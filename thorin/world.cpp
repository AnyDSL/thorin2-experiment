#include "thorin/world.h"

namespace thorin {

World::World()
    : star_(unify(new Star(*this)))
    , nat_(assume(star(), "Nat"))
{}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1) {
        if (auto tuple = args.front()->isa<Tuple>())
            return app(callee, tuple->ops(), name);

        if (auto sigma = args.front()->type()->isa<Sigma>()) {
            assert(false && "TODO" && sigma);
        }
    }

    auto app = unify(new App(*this, callee, args, name));

    if (auto cache = app->cache_)
        return cache;
    if (auto lambda = app->callee()->template isa<Lambda>())
        return app->cache_ = lambda->body()->reduce(1, args);
    else
        return app->cache_ = app;

    return app;
}

const Def* World::tuple(const Def* type, Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify(new Tuple(*this, type, defs, name));
}

const Def* World::unify_base(const Def* def) {
    assert(!def->is_nominal());
    auto p = defs_.emplace(def);
    if (p.second)
        return def;

    def->unregister_uses();
    --Def::gid_counter_;
    delete def;
    return *p.first;
}

}
