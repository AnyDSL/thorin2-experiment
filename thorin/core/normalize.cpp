#include "thorin/core/world.h"
#include "thorin/core/fold.h"

namespace thorin::core {

/*
 * helpers
 */

const Def* check_callee(Normalizer normalizer, thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto pi = callee->type()->isa<Pi>(); pi->body()->isa<Pi>())
        return world.curry(normalizer, callee, arg, dbg);
    return nullptr;
}

std::tuple<const Def*, const Def*> split(thorin::World& world, const Def* def) {
    auto a = world.extract(def, 0_u64);
    auto b = world.extract(def, 1_u64);
    return {a, b};
}

std::tuple<const Def*, const Def*, const Def*> msplit(thorin::World& world, const Def* def) {
    auto a = world.extract(def, 0_u64);
    auto b = world.extract(def, 1_u64);
    auto m = world.extract(def, 2_u64);
    return {a, b, m};
}

std::tuple<const Def*, const Def*> shrink_shape(thorin::World& world, const Def* def) {
    if (def->isa<Arity>())
        return {def, world.arity(1)};
    if (auto sigma = def->isa<Sigma>())
        return {sigma->op(0), world.sigma(sigma->ops().skip_front())->shift_free_vars(world, -1)};
    auto variadic = def->as<Variadic>();
    return {variadic->arity(world), world.variadic(variadic->arity(world)->as<Arity>()->value() - 1, variadic->body())};
}

const Def* normalize_tuple(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto ta = a->isa<Tuple>(), tb = b->isa<Tuple>();
    auto pa = a->isa<Pack>(),  pb = b->isa<Pack>();

    if ((ta || pa) && (tb || pb)) {
        auto [head, tail] = shrink_shape(world, app_arg(callee));
        auto new_callee = world.app(app_callee(callee), tail);

        if (ta && tb) return world.tuple(DefArray(ta->num_ops(), [&](auto i) { return world.app(new_callee, {ta->op(i), tb->op(i)}, dbg); }));
        if (ta && pb) return world.tuple(DefArray(ta->num_ops(), [&](auto i) { return world.app(new_callee, {ta->op(i), pb->body()}, dbg); }));
        if (pa && tb) return world.tuple(DefArray(tb->num_ops(), [&](auto i) { return world.app(new_callee, {pa->body(), tb->op(i)}, dbg); }));
        assert(pa && pb);
        return world.pack(head, world.app(new_callee, {pa->body(), pb->body()}, dbg), dbg);
    }

    return nullptr;
}

const Lit* const_to_left(const Def*& a, const Def*& b) {
    if (b->isa<Lit>()) {
        std::swap(a, b);
        return a->as<Lit>();
    }

    return a->isa<Lit>();
}

const Def* commute(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    if (a->gid() > b->gid() && !a->isa<Lit>())
        return world.raw_app(callee, {b, a}, dbg);
    return world.raw_app(callee, {a, b}, dbg);
}

/*
 * WArithop
 */

template<template<int, bool, bool> class F>
const Def* try_wfold(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = callee->type()->template as<Pi>()->body();
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
                        case  8: return world.lit(t, F< 8, true, true>::run(ba, bb));
                        case 16: return world.lit(t, F<16, true, true>::run(ba, bb));
                        case 32: return world.lit(t, F<32, true, true>::run(ba, bb));
                        case 64: return world.lit(t, F<64, true, true>::run(ba, bb));
                    }
            }
        } catch (ErrorException) {
            return world.error(t);
        }
    }

    return normalize_tuple(world, callee, a, b, dbg);
}

const Def* normalize_add(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_add, world, callee, arg, dbg)) return result;

    auto& w = static_cast<World&>(world);
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_add>(world, callee, a, b, dbg)) return result;

    if (auto la = const_to_left(a, b)) {
        if (get_u64(la) == 0_u64) return b;
    }

    if (a == b) return w.op<WOp::mul>(world.lit(a->type(), {2_u64}), a, dbg);

    return commute(world, callee, a, b, dbg);
}

const Def* normalize_sub(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_sub, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_sub>(world, callee, a, b, dbg)) return result;

    if (a == b) return world.lit(a->type(), {0_u64});

    return world.raw_app(callee, {a, b}, dbg);
}

const Def* normalize_mul(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_mul, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_mul>(world, callee, a, b, dbg)) return result;

    if (auto la = const_to_left(a, b)) {
        if (get_u64(la) == 0_u64) return la;
        if (get_u64(la) == 1_u64) return b;
    }

    return commute(world, callee, a, b, dbg);
}

const Def* normalize_shl(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_shl, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_shl>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

/*
 * MArithop
 */

const Def* normalize_sdiv(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_sdiv, world, callee, arg, dbg)) return result;

    auto [m, a, b] = msplit(world, arg);
    //if (auto result = try_wfold<Fold_sub>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {m, a, b}, dbg);
}

const Def* normalize_udiv(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_udiv, world, callee, arg, dbg)) return result;

    auto [m, a, b] = msplit(world, arg);
    //if (auto result = try_wfold<Fold_sub>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {m, a, b}, dbg);
}

const Def* normalize_smod(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_smod, world, callee, arg, dbg)) return result;

    auto [m, a, b] = msplit(world, arg);
    //if (auto result = try_wfold<Fold_sub>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {m, a, b}, dbg);
}

const Def* normalize_umod(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_umod, world, callee, arg, dbg)) return result;

    auto [m, a, b] = msplit(world, arg);
    //if (auto result = try_wfold<Fold_sub>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {m, a, b}, dbg);
}

/*
 * IArithop
 */

template<template<int> class F>
const Def* try_ifold(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = callee->type()->template as<Pi>()->body();
        auto w = get_nat(app_arg(app_callee(callee)));
        try {
            switch (w) {
                case  8: return world.lit(t, F< 8>::run(ba, bb));
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (ErrorException) {
            return world.error(t);
        }
    }

    return normalize_tuple(world, callee, a, b, dbg);
}

const Def* normalize_ashr(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_ashr, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_ashr>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

const Def* normalize_lshr(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_lshr, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_lshr>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

const Def* normalize_iand(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_iand, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_iand>(world, callee, a, b, dbg)) return result;

    return commute(world, callee, a, b, dbg);
}

const Def* normalize_ior(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_ior, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_ior>(world, callee, a, b, dbg)) return result;

    return commute(world, callee, a, b, dbg);
}

const Def* normalize_ixor(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_ixor, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_ixor>(world, callee, a, b, dbg)) return result;

    return commute(world, callee, a, b, dbg);
}

/*
 * RArithop
 */

template<template<int> class F>
const Def* try_rfold(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = callee->type()->template as<Pi>()->body();
        auto w = get_nat(app_arg(app_callee(callee)));
        try {
            switch (w) {
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (ErrorException) {
            return world.error(t);
        }
    }

    return normalize_tuple(world, callee, a, b, dbg);
}

const Def* normalize_radd(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_radd, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_radd>(world, callee, a, b, dbg)) return result;

    if (auto la = const_to_left(a, b)) {
        la->box();
    }

    return commute(world, callee, a, b, dbg);
}

const Def* normalize_rsub(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_rsub, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rsub>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

const Def* normalize_rmul(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_rmul, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rmul>(world, callee, a, b, dbg)) return result;

    if (auto la = const_to_left(a, b)) {
        la->box();
    }

    return commute(world, callee, a, b, dbg);
}

const Def* normalize_rdiv(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_rdiv, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rdiv>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

const Def* normalize_rmod(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_rmod, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rrem>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

/*
 * icmp
 */

template<ICmp op>
const Def* normalize_ICmp(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_ICmp<op>, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<FoldICmp<op>::template Fold>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}


template<RCmp op>
const Def* normalize_RCmp(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    if (auto result = check_callee(normalize_RCmp<op>, world, callee, arg, dbg)) return result;

    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<FoldRCmp<op>::template Fold>(world, callee, a, b, dbg)) return result;

    return world.raw_app(callee, {a, b}, dbg);
}

// instantiate templates
#define CODE(T, o) template const Def* normalize_ ## T<T::o>(thorin::World&, const Def*, const Def*, Debug);
    THORIN_I_CMP(CODE)
    THORIN_R_CMP(CODE)
#undef CODE

}
