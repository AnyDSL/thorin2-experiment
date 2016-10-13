#include "thorin/world.h"

namespace thorin {

const Def* World::app(const Def* callee, const Def* arg, const std::string& name) {
    if (auto sigma = arg->type()->isa<Sigma>()) {
        Array<const Def*> args;
        for (size_t i = 0, e = sigma->num_ops(); i != e; ++i)
            args[i] = app(arg, arg, name);
        return app(callee, args, name);
    }
    return app(callee, {arg}, name);
}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1 && args.front()->type()->isa<Sigma>())
        return app(callee, args.front(), name);

    auto app = unify(new App(*this, callee, args, name));

    if (auto cache = app->cache_)
        return cache;
    if (auto lambda = app->callee()->template isa<Lambda>()) {
        Def2Def map;
        return app->cache_ = lambda->body()->reduce(1, args.front(), map); // TODO reduce args
    } else
        return app->cache_ = app;

    return app;
}

const Def* World::unify_base(const Def* def) {
    assert(!def->is_nominal());
    auto p = defs_.emplace(def);
    if (p.second)
        return def;

    --Def::gid_counter_;
    delete def;
    return *p.first;
}

}
