#include "thorin/core/world.h"
#include "thorin/core/fold.h"
#include "thorin/transform/reduce.h"

namespace thorin::core {

/*
 * helpers
 */

static std::array<const Def*, 2> split(const Def* def) {
    auto& w = def->world();
    auto a = w.extract(def, 0_u64);
    auto b = w.extract(def, 1_u64);
    return {{a, b}};
}

static std::array<const Def*, 3> msplit(const Def* def) {
    auto& w = def->world();
    auto a = w.extract(def, 0_u64);
    auto b = w.extract(def, 1_u64);
    auto m = w.extract(def, 2_u64);
    return {{a, b, m}};
}

static std::array<const Def*, 2> shrink_shape(const Def* def) {
    auto& w = def->world();
    if (def->isa<Arity>())
        return {{def, w.arity(1)}};
    if (auto sigma = def->isa<Sigma>())
        return {{sigma->op(0), shift_free_vars(w.sigma(sigma->ops().skip_front()), -1)}};
    auto variadic = def->as<Variadic>();
    return {{variadic->arity(), w.variadic(variadic->arity()->as<Arity>()->value() - 1, variadic->body())}};
}

static const Def* normalize_tuple(const Def* callee, const Def* a, Debug dbg) {
    auto& w = callee->world();
    auto ta = a->isa<Tuple>();
    auto pa = a->isa<Pack>();

    if (ta || pa) {
        auto [head, tail] = shrink_shape(app_arg(callee));
        auto new_callee = w.app(app_callee(callee), tail);

        if (ta) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, ta->op(i), dbg); }));
        assert(pa);
        return w.pack(head, w.app(new_callee, pa->body(), dbg), dbg);
    }

    return nullptr;
}

static const Def* normalize_tuple(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    auto ta = a->isa<Tuple>(), tb = b->isa<Tuple>();
    auto pa = a->isa<Pack>(),  pb = b->isa<Pack>();

    if ((ta || pa) && (tb || pb)) {
        auto [head, tail] = shrink_shape(app_arg(callee));
        auto new_callee = w.app(app_callee(callee), tail);

        if (ta && tb) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, {ta->op(i),  tb->op(i)},  dbg); }));
        if (ta && pb) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, {ta->op(i),  pb->body()}, dbg); }));
        if (pa && tb) return w.tuple(DefArray(tb->num_ops(), [&](auto i) { return w.app(new_callee, {pa->body(), tb->op(i)},  dbg); }));
        assert(pa && pb);
        return w.pack(head, w.app(new_callee, {pa->body(), pb->body()}, dbg), dbg);
    }

    return nullptr;
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

static inline bool is_foldable(const Def* def) { return def->isa<Lit>() || def->isa<Tuple>() || def->isa<Pack>(); }

static const Lit* foldable_to_left(const Def*& a, const Def*& b) {
    if (is_foldable(b))
        std::swap(a, b);

    return a->isa<Lit>();
}

static const Def* commute(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    if (a->gid() > b->gid() && !a->isa<Lit>())
        return w.raw_app(callee, {b, a}, dbg);
    return w.raw_app(callee, {a, b}, dbg);
}

static const Def* reassociate(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    std::array<const Def*, 2> args{{a, b}};
    std::array<std::array<const Def*, 2>, 2> aa{{{{nullptr, nullptr}}, {{nullptr, nullptr}}}};
    std::array<bool, 2> foldable{{false, false}};

    for (size_t i = 0; i != 2; ++i) {
        if (auto app = args[i]->isa<App>()) {
            aa[i] = split(app->arg());
            foldable[i] = is_foldable(aa[i][0]);
        }
    }

    if (foldable[0] && foldable[1]) {
        auto f = w.app(callee, {aa[0][0], aa[1][0]}, dbg);
        return w.app(callee, {f, w.app(callee, {aa[0][1], aa[1][1]}, dbg)}, dbg);
    }

    for (size_t i = 0; i != 2; ++i) {
        if (foldable[i])
            return w.app(callee, {aa[i][0], w.app(callee, {args[1-i], aa[i][1]}, dbg)}, dbg);
    }

    return commute(callee, a, b, dbg);
}

bool is_commutative(ROp op) { return op == ROp::radd || op == ROp::rmul; }
bool is_commutative(WOp op) { return op == WOp:: add || op == WOp:: mul; }
bool is_commutative(IOp op) { return op == IOp::iand || op == IOp:: ior || op == IOp::ixor; }

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
        auto w = get_nat(app_arg(app_callee(callee)));
        auto f = get_nat(app_arg(app_callee(app_callee(callee))));
        try {
            switch (f) {
                case int64_t(WFlags::none):
                    switch (w) {
                        case  8: return world.lit(t, F< 8, false, false>::run(ba, bb));
                        case 16: return world.lit(t, F<16, false, false>::run(ba, bb));
                        case 32: return world.lit(t, F<32, false, false>::run(ba, bb));
                        case 64: return world.lit(t, F<64, false, false>::run(ba, bb));
                    }
                case int64_t(WFlags::nsw):
                    switch (w) {
                        case  8: return world.lit(t, F< 8, true, false>::run(ba, bb));
                        case 16: return world.lit(t, F<16, true, false>::run(ba, bb));
                        case 32: return world.lit(t, F<32, true, false>::run(ba, bb));
                        case 64: return world.lit(t, F<64, true, false>::run(ba, bb));
                    }
                case int64_t(WFlags::nuw):
                    switch (w) {
                        case  8: return world.lit(t, F< 8, false, true>::run(ba, bb));
                        case 16: return world.lit(t, F<16, false, true>::run(ba, bb));
                        case 32: return world.lit(t, F<32, false, true>::run(ba, bb));
                        case 64: return world.lit(t, F<64, false, true>::run(ba, bb));
                    }
                case int64_t(WFlags::nsw | WFlags::nuw):
                    switch (w) {
                        case  8: return world.lit(t, F< 8, true,  true>::run(ba, bb));
                        case 16: return world.lit(t, F<16, true,  true>::run(ba, bb));
                        case 32: return world.lit(t, F<32, true,  true>::run(ba, bb));
                        case 64: return world.lit(t, F<64, true,  true>::run(ba, bb));
                    }
            }
        } catch (BottomException) {
            return world.bottom(t);
        }
    }

    return normalize_tuple(callee, a, b, dbg);
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
            return world.bottom(t);
        }
    }

    return nullptr;
}

template<template<int> class F>
static const Def* try_ifold(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    if (auto result = just_try_ifold<F>(callee, a, b)) return result;
    return normalize_tuple(callee, a, b, dbg);
}

template<template<int> class F>
static const Def* try_mfold(const Def* callee, const Def* m, const Def* a, const Def* b, Debug dbg) {
    auto& w = static_cast<World&>(callee->world());
    if (auto result = just_try_ifold<F>(callee, a, b)) return w.tuple({m, result});
    return normalize_mtuple(callee, m, a, b, dbg);
}

template<MOp op>
const Def* normalize_MOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());
    auto [m, a, b] = msplit(arg);
    if (auto result = try_mfold<FoldMOp<op>::template Fold>(callee, m, a, b, dbg)) return result;

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
        auto w = get_nat(app_arg(app_callee(callee)));
        try {
            switch (w) {
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (BottomException) {
            return world.bottom(t);
        }
    }

    return normalize_tuple(callee, a, b, dbg);
}

template<ROp op>
const Def* normalize_ROp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());
    auto [a, b] = split(arg);
    if (auto result = try_rfold<FoldROp<op>::template Fold>(callee, a, b, dbg)) return result;

    auto f = RFlags(get_nat(app_arg(app_callee(app_callee(callee)))));
    auto w = get_nat(app_arg(app_callee(callee)));

    if (is_commutative(op)) {
        if (auto la = foldable_to_left(a, b)) {
            if (is_rzero(w, la)) {
                if (op == ROp::radd) return b;
                if (op == ROp::rmul && has_feature(f, RFlags::finite | RFlags::nsz)) return la;
            }
        }
    }

    if (is_commutative(op)) {
        if (has_feature(f, RFlags::reassoc))
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

template<RCmp op>
const Def* normalize_RCmp(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);
    if (auto result = try_rfold<FoldRCmp<op>::template Fold>(callee, a, b, dbg)) return result;

    return w.raw_app(callee, {a, b}, dbg);
}

/*
 * cast
 */

template<Cast op>
const Def* normalize_Cast(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());

    if (auto result =  normalize_tuple(callee, arg, dbg))
        return result;
    return world.raw_app(callee, arg, dbg);
}

// instantiate templates
#define CODE(T, o) template const Def* normalize_ ## T<T::o>(const Def*, const Def*, Debug);
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_R_OP (CODE)
    THORIN_I_CMP(CODE)
    THORIN_R_CMP(CODE)
    THORIN_CAST(CODE)
#undef CODE

}
