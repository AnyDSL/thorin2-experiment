#include "thorin/core/world.h"

namespace thorin {
namespace core {

//------------------------------------------------------------------------------

const Def* normalize_iadd(thorin::World& world, const Def*, const Def*, const Def* arg, Debug dbg) {
    auto a = world.extract(arg, 0, dbg);
    auto b = world.extract(arg, 1, dbg);

    auto aa = a->isa<Axiom>();
    auto ab = b->isa<Axiom>();

    if (aa && ab)
        return world.assume(a->type(), Box(aa->box().get_u64() + ab->box().get_u64()), dbg);

    return nullptr;
}

const Def* normalize_iadd_type (thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_iadd,       type, callee, arg, dbg); }
const Def* normalize_iadd_shape(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_iadd_type,  type, callee, arg, dbg); }
const Def* normalize_iadd_flags(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) { return world.curry(normalize_iadd_shape, type, callee, arg, dbg); }

}
}
