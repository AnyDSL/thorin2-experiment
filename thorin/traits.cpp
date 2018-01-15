#include "thorin/world.h"

namespace thorin {

const Axiom* isa_const_qualifier(const Def* def) {
    assert(def != nullptr);
    for (auto q : def->world().qualifiers())
        if (q == def) return q;
    return nullptr;
}

#if  0
bool is_primitive_type(const Def* type) {
    auto& world = static_cast<World&>(type->world());
    if (type == world.type_bool() || type == world.type_nat())
        return true;
    if (auto app = type->isa<App>())
        return is_primitive_type_constructor(app->callee());
    if (auto var = type->isa<Variadic>())
        return is_primitive_type(var->body());
    return false;
}

bool is_primitive_type_constructor(const Def* def) {
    auto& world = static_cast<World&>(def->world());
    if (!def->type()->isa<Pi>())
        return false;
    while (auto app = def->isa<App>())
        def = app->callee();
    return def && (def == world.type_i() || def == world.type_r());
}
#endif

}
