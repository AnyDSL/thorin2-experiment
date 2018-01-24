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
    if (auto result = try_wfold<Fold_wadd>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_wsub(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_wsub>(world, callee, a, b, dbg)) return result;
    return nullptr;
}

const Def* normalize_wmul(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_wmul>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_wshl(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_wfold<Fold_wshl>(world, callee, a, b, dbg)) return result;

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
    if (auto result = try_ifold<Fold_ashr>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_lshr(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_lshr>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_iand(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_iand>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_ior(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_ior>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_ixor(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_ifold<Fold_ixor>(world, callee, a, b, dbg)) return result;

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
    if (auto result = try_rfold<Fold_radd>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rsub(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rsub>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rmul(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rmul>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rdiv(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rdiv>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

const Def* normalize_rmod(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    if (auto result = try_rfold<Fold_rrem>(world, callee, a, b, dbg)) return result;

    return nullptr;
}

/*
 * icmp
 */

template<ICmp op>
const Def* normalize_icmp(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    const Def* result = nullptr;
    switch (op) {
        case i_eq: result = try_ifold<Fold_i_eq>(world, callee, a, b, dbg); break;
        case i_ne: result = try_ifold<Fold_i_ne>(world, callee, a, b, dbg); break;
        case iugt: result = try_ifold<Fold_iugt>(world, callee, a, b, dbg); break;
        case iuge: result = try_ifold<Fold_iuge>(world, callee, a, b, dbg); break;
        case iult: result = try_ifold<Fold_iult>(world, callee, a, b, dbg); break;
        case iule: result = try_ifold<Fold_iule>(world, callee, a, b, dbg); break;
        case isgt: result = try_ifold<Fold_isgt>(world, callee, a, b, dbg); break;
        case isge: result = try_ifold<Fold_isge>(world, callee, a, b, dbg); break;
        case islt: result = try_ifold<Fold_islt>(world, callee, a, b, dbg); break;
        case isle: result = try_ifold<Fold_isle>(world, callee, a, b, dbg); break;
        default: THORIN_UNREACHABLE;
    }

    if (result)
        return result;

    return nullptr;
}

template<RCmp op>
const Def* normalize_rcmp(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto [a, b] = split(world, arg);
    const Def* result = nullptr;
    switch (op) {
        case rt  : result = try_rfold<Fold_rt  >(world, callee, a, b, dbg); break;
        case rult: result = try_rfold<Fold_rult>(world, callee, a, b, dbg); break;
        case rugt: result = try_rfold<Fold_rugt>(world, callee, a, b, dbg); break;
        case rune: result = try_rfold<Fold_rune>(world, callee, a, b, dbg); break;
        case rueq: result = try_rfold<Fold_rueq>(world, callee, a, b, dbg); break;
        case rule: result = try_rfold<Fold_rule>(world, callee, a, b, dbg); break;
        case ruge: result = try_rfold<Fold_ruge>(world, callee, a, b, dbg); break;
        case runo: result = try_rfold<Fold_runo>(world, callee, a, b, dbg); break;
        case rord: result = try_rfold<Fold_rord>(world, callee, a, b, dbg); break;
        case rolt: result = try_rfold<Fold_rolt>(world, callee, a, b, dbg); break;
        case rogt: result = try_rfold<Fold_rogt>(world, callee, a, b, dbg); break;
        case rone: result = try_rfold<Fold_rone>(world, callee, a, b, dbg); break;
        case roeq: result = try_rfold<Fold_roeq>(world, callee, a, b, dbg); break;
        case role: result = try_rfold<Fold_role>(world, callee, a, b, dbg); break;
        case roge: result = try_rfold<Fold_roge>(world, callee, a, b, dbg); break;
        case rf  : result = try_rfold<Fold_rf  >(world, callee, a, b, dbg); break;
        default: THORIN_UNREACHABLE;
    }

    if (result)
        return result;

    return nullptr;
}

/*
 * curry normalizers
 */

#define CODE(o) \
    const Def* normalize_ ## o ## _2(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o,       type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _2, type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _1, type, callee, arg, dbg); }
    THORIN_W_OP(CODE)
    THORIN_R_OP(CODE)
#undef CODE

#define CODE(o) \
    const Def* normalize_ ## o ## _1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o,       type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _1, type, callee, arg, dbg); }
    THORIN_M_OP(CODE)
    THORIN_I_OP(CODE)
#undef CODE

#define CODE(o) \
    const Def* normalize_ ## o ## _1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_icmp<o>,    type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _1, type, callee, arg, dbg); }
    THORIN_I_CMP(CODE)
#undef CODE

#define CODE(o) \
    const Def* normalize_ ## o ## _2(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_rcmp<o>,    type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _1(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _2, type, callee, arg, dbg); } \
    const Def* normalize_ ## o ## _0(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_## o ## _1, type, callee, arg, dbg); }
    THORIN_R_CMP(CODE)
#undef CODE

}
