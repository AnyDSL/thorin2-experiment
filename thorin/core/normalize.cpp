#include "thorin/core/world.h"

namespace thorin {
namespace core {

//------------------------------------------------------------------------------

const Def* shrink_shape(thorin::World& world, const Def* def) {
    if (def->isa<Arity>())
        return world.arity(1);
    if (auto sigma = def->isa<Sigma>())
        return world.sigma(sigma->ops().skip_front());
    auto variadic = def->as<Variadic>();
    return world.variadic(variadic->arity()->as<Arity>()->value() - 1, variadic->body());
}

//auto v = app_arg(callee)->as<Variadic>();

const Def* normalize_tuple(thorin::World& world, const Def* callee, const Def* arg, Debug dbg) {
    auto a = world.extract(arg, 0, dbg);
    auto b = world.extract(arg, 1, dbg);

    auto ta = a->isa<Tuple>();
    auto tb = b->isa<Tuple>();

    if (ta && tb) {
        auto new_shape = shrink_shape(world, app_arg(callee));
        auto new_callee = world.app(app_callee(callee), new_shape);

        return world.tuple(DefArray(ta->num_ops(), [&](auto i) { return world.app(new_callee, {ta->op(i), tb->op(i)}); }));
    }

    return nullptr;
}

const Def* normalize_wadd(thorin::World& world, const Def*, const Def* callee, const Def* arg, Debug dbg) {
    auto a = world.extract(arg, 0, dbg);
    auto b = world.extract(arg, 1, dbg);

    auto aa = a->isa<Axiom>();
    auto ab = b->isa<Axiom>();

    auto ta = a->isa<Tuple>();
    auto tb = b->isa<Tuple>();

    if (ta && tb)
        return normalize_tuple(world, callee, arg, dbg);

    if (aa && ab)
        return world.assume(a->type(), Box(aa->box().get_u64() + ab->box().get_u64()), dbg);

    return nullptr;
}

const Def* normalize_wadd_type (thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_wadd,       type, callee, arg, dbg); }
const Def* normalize_wadd_shape(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_wadd_type,  type, callee, arg, dbg); }
const Def* normalize_wadd_flags(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_wadd_shape, type, callee, arg, dbg); }

}
}
