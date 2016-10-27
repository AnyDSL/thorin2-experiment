#include "thorin/world.h"

namespace thorin {

World::World()
    : star_(unify(new Star(*this)))
    , error_(unify(new Error(*this)))
    , nat_({assume(star(), Qualifier::Unrestricted, "Nat"),
            assume(star(), Qualifier::Affine, "Nat"),
            assume(star(), Qualifier::Relevant, "Nat"),
            assume(star(), Qualifier::Linear, "Nat")})
{}

const Pi* World::pi(Defs domain, const Def* body, Qualifier::URAL q, const std::string& name) {
    if (domain.size() == 1 && domain.front()->type()) {
        if (auto sigma = domain.front()->type()->isa<Sigma>())
            return pi(sigma->ops(), body, q, name);
    }

    return unify(new Pi(*this, domain, body, q, name));
}

const Def* World::tuple(const Def* type, Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify(new Tuple(*this, type, defs, name));
}

const Def* World::sigma(Defs defs, Qualifier::URAL q, const std::string& name) {
    if (defs.size() == 1) {
        auto single = defs.front();
        assert(!single || single->qualifier() == q);
        return single;
    }

    return unify(new Sigma(*this, defs, name, q));
}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1) {
        if (auto tuple = args.front()->isa<Tuple>())
            return app(callee, tuple->ops(), name);
    }

    if (too_many_affine_uses(args))
        return error();
    if (callee->type()->isa<Sigma>()) {
        assert(args.size() == 1);
        auto assume = args.front()->as<Assume>();
        if (assume->type() != nat())
            return error();
        auto index = args.front()->name();
        if (callee->type()->is_affine()) {
            // check all uses for same index
            for (auto use : callee->uses()) {
                auto use_index = use->as<App>()->op(1)->as<Assume>()->name();
                if (index == use_index)
                    return error();
            }
        }
    }

    auto app = unify(new App(*this, callee, args, name));

    if (auto cache = app->cache_)
        return cache;
    if (auto abs = app->callee()->isa<Abs>())
        return app->cache_ = abs->reduce(args);
    else
        return app->cache_ = app;

    return app;
}

const Def* World::extract(const Def* def, const Def* i) {
    if (!def->isa<Sigma>() && !def->type()->isa<Sigma>()) {
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
