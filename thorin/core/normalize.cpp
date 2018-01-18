#include "thorin/core/world.h"
#include "thorin/util/fold.h"

namespace thorin {
namespace core {

//------------------------------------------------------------------------------

std::tuple<const Def*, const Def*> shrink_shape(thorin::World& world, const Def* def) {
    if (def->isa<Arity>())
        return {def, world.arity(1)};
    if (auto sigma = def->isa<Sigma>())
        return {sigma->op(0), world.sigma(sigma->ops().skip_front())->shift_free_vars(-1)};
    auto variadic = def->as<Variadic>();
    return {variadic->arity(), world.variadic(variadic->arity()->as<Arity>()->value() - 1, variadic->body())};
}

/*
 * WArithop
 */

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

const Def* normalize_wadd(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto a = world.extract(arg, 0, dbg), b = world.extract(arg, 1, dbg);

    if (auto result = normalize_tuple(world, callee, a, b, dbg))
        return result;

    auto aa = a->isa<Axiom>(), ab = b->isa<Axiom>();
    if (aa && ab) {
        auto ba = aa->box(), bb = ab->box();
        auto t = a->type();
        auto w = get_nat(app_arg(app_callee(callee)));
        auto f = get_nat(app_arg(app_callee(app_callee(callee))));
        try {
            switch (f) {
                case int64_t(WFlags::none):
                    switch (w) {
                        case  8: return world.assume(t, {UInt< u8>(ba. get_u8(), false) + UInt< u8>(bb. get_u8(), false)}, dbg);
                        case 16: return world.assume(t, {UInt<u16>(ba.get_u16(), false) + UInt<u16>(bb.get_u16(), false)}, dbg);
                        case 32: return world.assume(t, {UInt<u32>(ba.get_u32(), false) + UInt<u32>(bb.get_u32(), false)}, dbg);
                        case 64: return world.assume(t, {UInt<u64>(ba.get_u64(), false) + UInt<u64>(bb.get_u64(), false)}, dbg);
                    }
                case int64_t(WFlags::nsw):
                    switch (w) {
                        case  8: return world.assume(t, {SInt< s8>(ba. get_s8(),  true) + SInt< s8>(bb. get_s8(),  true)}, dbg);
                        case 16: return world.assume(t, {SInt<s16>(ba.get_s16(),  true) + SInt<s16>(bb.get_s16(),  true)}, dbg);
                        case 32: return world.assume(t, {SInt<s32>(ba.get_s32(),  true) + SInt<s32>(bb.get_s32(),  true)}, dbg);
                        case 64: return world.assume(t, {SInt<s64>(ba.get_s64(),  true) + SInt<s64>(bb.get_s64(),  true)}, dbg);
                    }
                case int64_t(WFlags::nuw):
                    switch (w) {
                        case  8: return world.assume(t, {UInt< u8>(ba. get_u8(),  true) + UInt< u8>(bb. get_u8(),  true)}, dbg);
                        case 16: return world.assume(t, {UInt<u16>(ba.get_u16(),  true) + UInt<u16>(bb.get_u16(),  true)}, dbg);
                        case 32: return world.assume(t, {UInt<u32>(ba.get_u32(),  true) + UInt<u32>(bb.get_u32(),  true)}, dbg);
                        case 64: return world.assume(t, {UInt<u64>(ba.get_u64(),  true) + UInt<u64>(bb.get_u64(),  true)}, dbg);
                    }
                case int64_t(WFlags::nsw | WFlags::nuw):
                        case  8:                                 SInt< s8>(ba. get_s8(),  true) + SInt< s8>(bb. get_s8(),  true);
                                 return world.assume(t, {UInt< u8>(ba. get_u8(),  true) + UInt< u8>(bb. get_u8(),  true)}, dbg);
                        case 16:                         SInt<s16>(ba.get_s16(),  true) + SInt<s16>(bb.get_s16(),  true);
                                 return world.assume(t, {UInt<u16>(ba.get_u16(),  true) + UInt<u16>(bb.get_u16(),  true)}, dbg);
                        case 32:                         SInt<s32>(ba.get_s32(),  true) + SInt<s32>(bb.get_s32(),  true);
                                 return world.assume(t, {UInt<u32>(ba.get_u32(),  true) + UInt<u32>(bb.get_u32(),  true)}, dbg);
                        case 64:                         SInt<s64>(ba.get_s64(),  true) + SInt<s64>(bb.get_s64(),  true);
                                 return world.assume(t, {UInt<u64>(ba.get_u64(),  true) + UInt<u64>(bb.get_u64(),  true)}, dbg);
            }
        } catch (BottomException) {
        }
    }

    return nullptr;
}

const Def* normalize_wsub(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_wmul(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_wshl(thorin::World&, const Def*, const Def*, const Def*, Debug) {
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

const Def* normalize_ashr(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_lshr(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_iand(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_ior(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_ixor(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

/*
 * RArithop
 */

const Def* normalize_radd(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_rsub(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_rmul(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_rdiv(thorin::World&, const Def*, const Def*, const Def*, Debug) {
    return nullptr;
}

const Def* normalize_rmod(thorin::World&, const Def*, const Def*, const Def*, Debug) {
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

}
}
