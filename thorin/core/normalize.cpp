#include "thorin/core/world.h"
#include "thorin/core/fold.h"

namespace thorin::core {

/*
 * helpers
 */

std::tuple<const Def*, const Def*> split(thorin::World& world, const Def* def) {
    auto a = world.extract(def, 0);
    auto b = world.extract(def, 1);
    return {a, b};
}

std::tuple<const Def*, const Def*> shrink_shape(thorin::World& world, const Def* def) {
    if (def->isa<Arity>())
        return {def, world.arity(1)};
    if (auto sigma = def->isa<Sigma>())
        return {sigma->op(0), world.sigma(sigma->ops().skip_front())->shift_free_vars(-1)};
    auto variadic = def->as<Variadic>();
    return {variadic->arity(), world.variadic(variadic->arity()->as<Arity>()->value() - 1, variadic->body())};
}

const Def* normalize_tuple(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto ta = a->isa<Tuple>(), tb = b->isa<Tuple>();
    auto pa = a->isa<Pack>(),  pb = b->isa<Pack>();

    if ((ta || pa) && (tb || pb)) {
        auto [head, tail] = shrink_shape(world, app_arg(callee));
        auto new_callee = world.app(app_callee(callee), tail);

        if (ta && tb)
            return world.tuple(DefArray(ta->num_ops(), [&](auto i) { return world.app(new_callee, {ta->op(i), tb->op(i)}, dbg); }));
        if (ta && pb)
            return world.tuple(DefArray(ta->num_ops(), [&](auto i) { return world.app(new_callee, {ta->op(i), pb->body()}, dbg); }));
        if (pa && tb)
            return world.tuple(DefArray(tb->num_ops(), [&](auto i) { return world.app(new_callee, {pa->body(), tb->op(i)}, dbg); }));
        assert(pa && pb);
        return world.pack(head, world.app(new_callee, {pa->body(), pb->body()}, dbg), dbg);
    }

    return nullptr;
}

/*
 * WArithop
 */

template<template<int, bool, bool> class F>
const Def* try_wfold(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = a->type();
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
        } catch (BottomException) {
        }
    }

    return normalize_tuple(world, callee, a, b, dbg);
}

const Def* normalize_wadd(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<FoldWAdd>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_wsub(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<FoldWSub>(world, callee, a, b, dbg)) return result;
    return nullptr;
}

const Def* normalize_wmul(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<FoldWMul>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_wshl(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<FoldWShl>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

/*
 * MArithop
 */

const Def* normalize_sdiv(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_udiv(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_smod(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_umod(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

/*
 * IArithop
 */

template<template<int> class F>
const Def* try_ifold(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = a->type();
        auto w = get_nat(app_arg(app_callee(callee)));
        try {
            switch (w) {
                case  8: return world.lit(t, F< 8>::run(ba, bb));
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (BottomException) {
        }
    }

    return normalize_tuple(world, callee, a, b, dbg);
}

const Def* normalize_ashr(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<FoldAShr>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_lshr(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<FoldLShr>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_iand(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<FoldIAnd>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_ior(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<FoldIOr>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_ixor(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<FoldIXor>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

/*
 * RArithop
 */

template<template<int> class F>
const Def* try_rfold(thorin::World& world, const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box(), bb = lb->box();
        auto t = a->type();
        auto w = get_nat(app_arg(app_callee(callee)));
        try {
            switch (w) {
                case 16: return world.lit(t, F<16>::run(ba, bb));
                case 32: return world.lit(t, F<32>::run(ba, bb));
                case 64: return world.lit(t, F<64>::run(ba, bb));
            }
        } catch (BottomException) {
        }
    }

    return normalize_tuple(world, callee, a, b, dbg);
}

const Def* normalize_radd(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<FoldRAdd>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rsub(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<FoldRSub>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rmul(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<FoldRMul>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rdiv(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<FoldRDiv>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rmod(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<FoldRRem>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

/*
 * icmp/rcmp
 */

const Def* normalize_icmp(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    return nullptr;
}

const Def* normalize_rcmp(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    return nullptr;
}

/*
 * curry normalizers
 */

#define CODE(o) \
    const Def* normalize_ ## o ## _2(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o,       type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _2, type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _1, type, callee, arg, dbg); }
    THORIN_W_ARITHOP(CODE)
    THORIN_R_ARITHOP(CODE)
#undef CODE

#define CODE(o) \
    const Def* normalize_ ## o ## _1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o,       type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _1, type, callee, arg, dbg); }
    THORIN_M_ARITHOP(CODE)
    THORIN_I_ARITHOP(CODE)
#undef CODE

const Def* normalize_icmp_2(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_icmp,   type, callee, arg, dbg); }
const Def* normalize_icmp_1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_icmp_2, type, callee, arg, dbg); }
const Def* normalize_icmp_0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_icmp_1, type, callee, arg, dbg); }

const Def* normalize_rcmp_2(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_rcmp,   type, callee, arg, dbg); }
const Def* normalize_rcmp_1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_rcmp_2, type, callee, arg, dbg); }
const Def* normalize_rcmp_0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_rcmp_1, type, callee, arg, dbg); }

}
