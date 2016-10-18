#include "thorin/world.h"

namespace thorin {

World::World()
    : star_(unify(new Star(*this)))
    , nat_(assume(star(), "Nat"))
{}

const Lambda* World::lambda(Defs domain, const Def* body, const std::string& name) {
    if (domain.size() == 1 && domain.front()->type()) {
        if (auto sigma = domain.front()->type()->isa<Sigma>())
            return lambda(sigma->ops(), body, name);
    }

    return unify(new Lambda(*this, domain, body, name));
}

const Pi* World::pi(Defs domain, const Def* body, const std::string& name) {
    if (domain.size() == 1 && domain.front()->type()) {
        if (auto sigma = domain.front()->type()->isa<Sigma>())
            return pi(sigma->ops(), body, name);
    }

    return unify(new Pi(*this, domain, body, name));
}

const Def* World::tuple(const Def* type, Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify(new Tuple(*this, type, defs, name));
}

const Def* World::sigma(Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify(new Sigma(*this, defs, name));
}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1) {
        if (auto tuple = args.front()->isa<Tuple>())
            return app(callee, tuple->ops(), name);

        if (auto sigma = args.front()->type()->isa<Sigma>()) {
            assert(false && "TODO" && sigma);
        }

        if (auto tuple = callee->isa<Tuple>()) {
            assert(args.size() == 1);
            return tuple->op(std::stoi(args.front()->name()));
        }
    }

    auto app = unify(new App(*this, callee, args, name));

    if (auto cache = app->cache_)
        return cache;
    if (auto lambda = app->callee()->template isa<Lambda>())
        return app->cache_ = lambda->body()->reduce(lambda->var(), args);
    else
        return app->cache_ = app;

    return app;
}

const Def* World::extract(const Def* def, const Def* i) {
    if (!def->isa<Tuple>() && !def->isa<Sigma>()) {
        assert(i->name() == "0");
        return def;
    }

    return app(def, i);
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
