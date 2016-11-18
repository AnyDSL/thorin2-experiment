#include "thorin/world.h"

namespace thorin {

World::World()
    : root_page_(new Page)
    , cur_page_(root_page_.get())
    , star_(unify<Star>(0, *this))
    , nat_(assume(star(), "Nat"))
    , boolean_(assume(star(), "Boolean"))
{}

const Lambda* World::lambda(Defs domains, const Def* body, const std::string& name) {
    return pi_lambda(pi(domains, body->type()), body, name);
}

const Lambda* World::pi_lambda(const Pi* pi, const Def* body, const std::string& name) {
    assert(pi->body() == body->type());
    return unify<Lambda>(1, *this, pi, body, name);
}

const Pi* World::pi(Defs domains, const Def* body, const std::string& name) {
    if (domains.size() == 1 && domains.front()->type()) {
        if (auto sigma = domains.front()->type()->isa<Sigma>())
            return pi(sigma->ops(), body, name);
    }

    return unify<Pi>(domains.size() + 1, *this, domains, body, name);
}

const Def* World::sigma(Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify<Sigma>(defs.size(), *this, defs, name);
}

const Def* World::tuple(const Def* type, Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify<Tuple>(defs.size(), *this, type, defs, name);
}

const Def* build_extract_type(World& world, const Def* tuple, int index) {
    auto sigma = tuple->type()->as<Sigma>();

    auto type = sigma->op(index);
    Def2Def map;
    for (int delta = 1; type->maybe_dependent() && delta <= index; delta++) {
        auto prev_extract = world.extract(tuple, index - delta);
        type = type->substitute(map, delta - 1, {prev_extract});
    }
    return type;
}

const Def* World::extract(const Def* def, int index, const std::string& name) {
    if (!def->isa<Sigma>() && !def->type()->isa<Sigma>()) {
        assert(index == 0);
        return def;
    }

    auto type = build_extract_type(*this, def, index);

    return unify<Extract>(1, *this, type, def, index, name);
}

const Def* World::intersection(Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    // TODO check for disjunct defs?
    return unify<Intersection>(defs.size(), *this, defs, name);
}

const Def* World::all(Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    // TODO check for disjunct types(defs)?
    return unify<All>(defs.size(), *this, intersection(types(defs)), defs, name);
}

const Def* World::pick(const Def* type, const Def* def, const std::string& name) {
    if (!type->isa<Intersection>()) {
        assert(type == def->type());
        return def;
    }

    // TODO implement reduction and caching
    return unify<Pick>(1, *this, type, def, name);
}

const Def* World::variant(Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    return unify<Variant>(defs.size(), *this, defs, name);
}

const Def* World::any(const Def* type, int index, const Def* def, const std::string& name) {
    if (!type->isa<Variant>()) {
        assert(type == def->type());
        assert(index == 0);
        return def;
    }

    return unify<Any>(1, *this, type, index, def, name);
}

const Def* build_match_type(const Def* def, const Variant* type, Defs handlers) {
    // TODO check handler types
    return /*TODO*/nullptr;
}

const Def* World::match(const Def* def, Defs handlers, const std::string& name) {
    auto def_type = def->type();
    if (handlers.size() == 1) {
        assert(!def_type->isa<Variant>());
        return app(handlers.front(), def, name);
    }
    auto matched_type = def->type()->as<Variant>();
    assert(def_type->num_ops() == handlers.size());
    auto type = build_match_type(def, matched_type, handlers);
    // TODO implement reduction and caching
    return unify<Match>(1, *this, type, def, handlers, name);
}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1) {
        if (auto tuple = args.front()->isa<Tuple>())
            return app(callee, tuple->ops(), name);
    }

    auto type = callee->type()->as<Pi>()->reduce(args);
    auto app = unify<App>(args.size() + 1, *this, type, callee, args, name);

    if (auto cache = app->cache_)
        return cache;
    // Can only really reduce if it's not an Assume of Pi type
    if (auto lambda = app->destructee()->isa<Lambda>())
        return app->cache_ = lambda->reduce(args);
    else
        return app->cache_ = app;

    return app;
}

}
