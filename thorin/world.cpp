#include "thorin/world.h"

namespace thorin {

World::World()
    : root_page_(new Page)
    , cur_page_(root_page_.get())
    , star_({unify(alloc<Star>(0, *this, Qualifier::Unrestricted)),
             unify(alloc<Star>(0, *this, Qualifier::Affine)),
             unify(alloc<Star>(0, *this, Qualifier::Relevant)),
             unify(alloc<Star>(0, *this, Qualifier::Linear))})
    , error_(unify(alloc<Error>(0, *this)))
    , nat_({assume(star(Qualifier::Unrestricted), "Nat"),
            assume(star(Qualifier::Affine), "Nat"),
            assume(star(Qualifier::Relevant), "Nat"),
            assume(star(Qualifier::Linear), "Nat")})
{}

const Pi* World::pi(Defs domains, const Def* body, Qualifier::URAL q, const std::string& name) {
    if (domains.size() == 1 && domains.front()->type()) {
        if (auto sigma = domains.front()->type()->isa<Sigma>())
            return pi(sigma->ops(), body, q, name);
    }

    return unify(alloc<Pi>(domains.size() + 1, *this, domains, body, q, name));
}

const Def* World::tuple(const Def* type, Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify(alloc<Tuple>(defs.size(), *this, type->as<Sigma>(), defs, name));
}

const Def* World::sigma(Defs defs, Qualifier::URAL q, const std::string& name) {
    if (defs.size() == 1) {
        auto single = defs.front();
        assert(!single || single->qualifier() == q);
        return single;
    }

    return unify(alloc<Sigma>(defs.size(), *this, defs, name, q));
}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1) {
        if (auto tuple = args.front()->isa<Tuple>())
            return app(callee, tuple->ops(), name);
    }

    if (too_many_affine_uses({callee}) || too_many_affine_uses(args))
        return error();
    // TODO do this checking later during a separate type checking phase
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

    auto type = callee->type()->as<Quantifier>()->reduce(args);
    auto app = unify(alloc<App>(args.size() + 1, *this, type, callee, args, name));

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

}
