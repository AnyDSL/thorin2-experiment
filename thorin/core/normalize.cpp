#include "thorin/core/world.h"

namespace thorin {
namespace core {

//------------------------------------------------------------------------------

const Def* normalize_iadd(const App* app) {
    auto& w = (core::World&) app->world();

    auto a = app_arg(app, 0)->isa<Axiom>();
    auto b = app_arg(app, 0)->isa<Axiom>();

    if (a && b)
        return w.assume(a->type(), Box(a->box().get_u64() + b->box().get_u64()));

    return app;
}

const Def* normalize_iadd_flags(const App* app) { return app->set_normalizer(normalize_iadd); }
const Def* normalize_iadd_shape(const App* app) { return app->set_normalizer(normalize_iadd_flags); }

}
}
