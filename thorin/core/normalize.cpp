#include "thorin/core/world.h"

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
    if (aa && ab)
        return world.assume(a->type(), Box(aa->box().get_u64() + ab->box().get_u64()), dbg);

    return nullptr;
}

const Def* normalize_wadd_type (thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_wadd,       type, callee, arg, dbg); }
const Def* normalize_wadd_shape(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_wadd_type,  type, callee, arg, dbg); }
const Def* normalize_wadd_flags(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_wadd_shape, type, callee, arg, dbg); }

}
}
