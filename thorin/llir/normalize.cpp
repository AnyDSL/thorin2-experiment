#include "thorin/normalize.h"
#include "thorin/llir/world.h"
#include "thorin/llir/fold.h"
#include "thorin/transform/reduce.h"

namespace thorin::llir {

/*
 * helpers
 */

static std::array<const Def*, 3> msplit(const Def* def) {
    auto& w = def->world();
    auto a = w.extract(def, 0_u64);
    auto b = w.extract(def, 1_u64);
    auto m = w.extract(def, 2_u64);
    return {{a, b, m}};
}

static const Def* normalize_mtuple(const Def* callee, const Def* m, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    // TODO
    auto ta = a->isa<Tuple>(), tb = b->isa<Tuple>();
    auto pa = a->isa<Pack>(),  pb = b->isa<Pack>();

    if ((ta || pa) && (tb || pb)) {
        auto [head, tail] = shrink_shape(app_arg(callee));
        auto new_callee = w.app(app_callee(callee), tail);

        if (ta && tb) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, {m, ta->op(i),  tb->op(i)},  dbg); }));
        if (ta && pb) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, {m, ta->op(i),  pb->body()}, dbg); }));
        if (pa && tb) return w.tuple(DefArray(tb->num_ops(), [&](auto i) { return w.app(new_callee, {m, pa->body(), tb->op(i)},  dbg); }));
        assert(pa && pb);
        return w.pack(head, w.app(new_callee, {m, pa->body(), pb->body()}, dbg), dbg);
    }

    return nullptr;
}

bool is_commutative(WOp op) { return op == WOp:: add || op == WOp:: mul; }
bool is_commutative(IOp op) { return op == IOp::iand || op == IOp:: ior || op == IOp::ixor; }
bool is_commutative(FOp op) { return op == FOp::fadd || op == FOp::fmul; }

/*
 * WArithop
 */

template<template<int, bool, bool> class F>
static const Def* try_wfold(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = callee->type()->template as<Pi>()->codomain();
        auto fw = app_arg(app_callee(callee));
        auto f = get_nat(world.extract(fw, 0_u64));
        auto w = get_nat(world.extract(fw, 1_u64));
        try {
            switch (f) {
                case int64_t(WFlags::none):
                    switch (w) {
                        case  8: return world.lit(t, F< 8, false, false>::run(ba, bb));
                        case 16: return world.lit(t, F<16, false, false>::run(ba, bb));
                        case 32: return world.lit(t, F<32, false, false>::run(ba, bb));
                        case 64: return world.lit(t, F<64, false, false>::run(ba, bb));
                        default: THORIN_UNREACHABLE;
                    }
                case int64_t(WFlags::nsw):
                    switch (w) {
                        case  8: return world.lit(t, F< 8,  true, false>::run(ba, bb));
                        case 16: return world.lit(t, F<16,  true, false>::run(ba, bb));
                        case 32: return world.lit(t, F<32,  true, false>::run(ba, bb));
                        case 64: return world.lit(t, F<64,  true, false>::run(ba, bb));
                        default: THORIN_UNREACHABLE;
                    }
                case int64_t(WFlags::nuw):
                    switch (w) {
                        case  8: return world.lit(t, F< 8, false,  true>::run(ba, bb));
                        case 16: return world.lit(t, F<16, false,  true>::run(ba, bb));
                        case 32: return world.lit(t, F<32, false,  true>::run(ba, bb));
                        case 64: return world.lit(t, F<64, false,  true>::run(ba, bb));
                        default: THORIN_UNREACHABLE;
                    }
                case int64_t(WFlags::nsw | WFlags::nuw):
                    switch (w) {
                        case  8: return world.lit(t, F< 8,  true,  true>::run(ba, bb));
                        case 16: return world.lit(t, F<16,  true,  true>::run(ba, bb));
                        case 32: return world.lit(t, F<32,  true,  true>::run(ba, bb));
                        case 64: return world.lit(t, F<64,  true,  true>::run(ba, bb));
                        default: THORIN_UNREACHABLE;
                    }
            }
        } catch (BottomException) {
            return world.bot(t);
        }
    }

    return normalize_tuple(callee, {a, b}, dbg);
}

template<WOp op>
const Def* normalize_WOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);
    if (auto result = try_wfold<FoldWOp<op>::template Fold>(callee, a, b, dbg)) return result;

    if (is_commutative(op)) {
        if (auto la = foldable_to_left(a, b)) {
            if (is_zero(la)) {
                switch (op) {
                    case WOp::add: return b;
                    case WOp::mul: return la;
                    default: THORIN_UNREACHABLE;
                }
            }
            if (is_one(la)) {
                switch (op) {
                    case WOp::add: break;
                    case WOp::mul: return b;
                    default: THORIN_UNREACHABLE;
                }
            }
        }
    }

    if (a == b) {
        switch (op) {
            case WOp::add: return world.op<WOp::mul>(world.lit(a->type(), {2_u64}), a, dbg);
            case WOp::sub: return world.lit(a->type(), {0_u64});
            case WOp::mul: break;
            case WOp::shl: break;
        }
    }

    if (is_commutative(op))
        return reassociate(callee, a, b, dbg);
    return world.raw_app(callee, {a, b}, dbg);
}

/*
 * MArithop
 */

template<template<int> class F>
static const Def* just_try_ifold(const Def* callee, const Def* a, const Def* b) {
    auto& world = static_cast<World&>(callee->world());
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = callee->type()->template as<Pi>()->codomain();
        auto w = get_nat(app_arg(app_callee(callee)));
        try {
            switch (w) {
                case  8: return world.lit(t, F< 8>::run(ba, bb));
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (BottomException) {
            return world.bot(t);
        }
    }

    return nullptr;
}

template<template<int> class F>
static const Def* try_ifold(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    if (auto result = just_try_ifold<F>(callee, a, b)) return result;
    return normalize_tuple(callee, {a, b}, dbg);
}

template<template<int> class F>
static const Def* try_mfold(const Def* callee, const Def* m, const Def* a, const Def* b, Debug dbg) {
    auto& w = static_cast<World&>(callee->world());
    if (auto result = just_try_ifold<F>(callee, a, b)) return w.tuple({m, result});
    return normalize_mtuple(callee, m, a, b, dbg);
}

template<ZOp op>
const Def* normalize_ZOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());
    auto [m, a, b] = msplit(arg);
    if (auto result = try_mfold<FoldZOp<op>::template Fold>(callee, m, a, b, dbg)) return result;

    return world.raw_app(callee, {m, a, b}, dbg);
}

/*
 * IArithop
 */

template<IOp op>
const Def* normalize_IOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);
    if (auto result = try_ifold<FoldIOp<op>::template Fold>(callee, a, b, dbg)) return result;

    if (is_commutative(op)) {
        if (auto la = foldable_to_left(a, b)) {
            if (is_zero(la)) {
                switch (op) {
                    case IOp::iand: return la;
                    case IOp::ior:  return b;
                    default: THORIN_UNREACHABLE;
                }
            }

            if (is_allset(la)) {
                switch (op) {
                    case IOp::iand: return b;
                    case IOp::ior:  return la;
                    default: THORIN_UNREACHABLE;
                }
            }
        }
    }

    if (is_commutative(op))
        return reassociate(callee, a, b, dbg);
    return world.raw_app(callee, {a, b}, dbg);
}

/*
 * RArithop
 */

template<template<int> class F>
static const Def* try_rfold(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = callee->type()->template as<Pi>()->codomain();
        auto w = get_nat(world.extract(app_arg(app_callee(callee)), 1));
        try {
            switch (w) {
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (BottomException) {
            return world.bot(t);
        }
    }

    return normalize_tuple(callee, {a, b}, dbg);
}

template<FOp op>
const Def* normalize_FOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());
    auto [a, b] = split(arg);
    if (auto result = try_rfold<FoldFOp<op>::template Fold>(callee, a, b, dbg)) return result;

    auto fw = app_arg(app_callee(callee));
    auto f = FFlags(get_nat(world.extract(fw, 0_u64)));
    auto w = get_nat(world.extract(fw, 1_u64));

    if (is_commutative(op)) {
        if (auto la = foldable_to_left(a, b)) {
            if (is_fzero(w, la)) {
                if (op == FOp::fadd) return b;
                if (op == FOp::fmul && has_feature(f, FFlags::finite | FFlags::nsz)) return la;
            }
        }
    }

    if (is_commutative(op)) {
        if (has_feature(f, FFlags::reassoc))
            return reassociate(callee, a, b, dbg);
        return commute(callee, a, b, dbg);
    }
    return world.raw_app(callee, {a, b}, dbg);
}

/*
 * icmp
 */

template<ICmp op>
const Def* normalize_ICmp(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);
    if (auto result = try_ifold<FoldICmp<op>::template Fold>(callee, a, b, dbg)) return result;

    return w.raw_app(callee, {a, b}, dbg);
}

template<FCmp op>
const Def* normalize_FCmp(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);
    if (auto result = try_rfold<FoldFCmp<op>::template Fold>(callee, a, b, dbg)) return result;

    return w.raw_app(callee, {a, b}, dbg);
}

/*
 * cast
 */

template<Cast op>
const Def* normalize_Cast(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());

    if (auto result = normalize_tuple(callee, {arg}, dbg))
        return result;
    return world.raw_app(callee, arg, dbg);
}

// instantiate templates
#define CODE(T, o) template const Def* normalize_ ## T<T::o>(const Def*, const Def*, Debug);
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_F_OP (CODE)
    THORIN_I_CMP(CODE)
    THORIN_F_CMP(CODE)
    THORIN_CAST(CODE)
#undef CODE

}
