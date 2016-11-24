#include "thorin/world.h"

namespace thorin {

World::World()
    : root_page_(new Page)
    , cur_page_(root_page_.get())
    , error_(unify<Error>(0, *this))
    , universe_({insert<Universe>(0, *this, Qualifier::Unrestricted),
                 insert<Universe>(0, *this, Qualifier::Affine),
                 insert<Universe>(0, *this, Qualifier::Relevant),
                 insert<Universe>(0, *this, Qualifier::Linear)})
    , star_({unify<Star>(0, *this, Qualifier::Unrestricted),
             unify<Star>(0, *this, Qualifier::Affine),
             unify<Star>(0, *this, Qualifier::Relevant),
             unify<Star>(0, *this, Qualifier::Linear)})
    , nat_({assume(star(Qualifier::Unrestricted), "Nat"),
            assume(star(Qualifier::Affine), "Nat"),
            assume(star(Qualifier::Relevant), "Nat"),
            assume(star(Qualifier::Linear), "Nat")})
    , boolean_({assume(star(Qualifier::Unrestricted), "Boolean"),
                assume(star(Qualifier::Affine), "Boolean"),
                assume(star(Qualifier::Relevant), "Boolean"),
                assume(star(Qualifier::Linear), "Boolean")})
{}

const Pi* World::pi(Defs domains, const Def* body, Qualifier::URAL q, const std::string& name) {
    if (domains.size() == 1 && domains.front()->type()) {
        if (auto sigma = domains.front()->type()->isa<Sigma>())
            return pi(sigma->ops(), body, q, name);
    }

    return unify<Pi>(domains.size() + 1, *this, domains, body, q, name);
}

const Lambda* World::pi_lambda(const Pi* pi, const Def* body, const std::string& name) {
    assert(pi->body() == body->type());
    return unify<Lambda>(1, *this, pi, body, name);
}

const Def* single_qualified(Defs defs, Qualifier::URAL q) {
    assert(defs.size() == 1);
    auto single = defs.front();
    // TODO if we use automatic qualifier coercion/subtyping, need to allow it here as well
    assert(!single || single->qualifier() == q);
    return single;
}

const Def* World::sigma(Defs defs, Qualifier::URAL q, const std::string& name) {
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Sigma>(defs.size(), *this, defs, q, name);
}

const Def* World::tuple(const Def* type, Defs defs, const std::string& name) {
    if (defs.size() == 1) {
        return defs.front();
    }

    return unify<Tuple>(defs.size(), *this, type->as<Sigma>(), defs, name);
}

const Def* build_extract_type(World& world, const Def* tuple, size_t index) {
    auto sigma = tuple->type()->as<Sigma>();

    auto type = sigma->op(index);
    Def2Def map;
    for (size_t delta = 1; type->maybe_dependent() && delta <= index; delta++) {
        auto prev_extract = world.extract(tuple, index - delta);
        type = type->substitute(map, delta - 1, {prev_extract});
    }
    return type;
}

const Def* World::extract(const Def* def, size_t index, const std::string& name) {
    if (!def->type()->isa<Sigma>()) {
        assert(index == 0);
        return def;
    }
    if (auto tuple = def->isa<Tuple>()) {
        return tuple->op(index);
    }

    auto type = build_extract_type(*this, def, index);
    return unify<Extract>(1, *this, type, def, index, name);
}

const Def* World::intersection(Defs defs, Qualifier::URAL q, const std::string& name) {
    if (defs.size() == 1) { return single_qualified(defs, q); }

    // TODO check disjunct defs during a later type checking step or now?
    return unify<Intersection>(defs.size(), *this, defs, q, name);
}

const Def* World::all(Defs defs, const std::string& name) {
    if (defs.size() == 1)
        return defs.front();

    // TODO check disjunct types(defs) during a later type checking step or now?
    return unify<All>(defs.size(), *this, intersection(types(defs))->as<Intersection>(), defs, name);
}

const Def* World::pick(const Def* type, const Def* def, const std::string& name) {
    if (!type->isa<Intersection>()) {
        assert(type == def->type());
        return def;
    }

    // TODO implement reduction and caching
    return unify<Pick>(1, *this, type, def, name);
}

const Def* World::variant(Defs defs, Qualifier::URAL q, const std::string& name) {
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Variant>(defs.size(), *this, defs, q, name);
}

const Def* World::any(const Def* type, const Def* def, const std::string& name) {
    if (!type->isa<Variant>()) {
        assert(type == def->type());
        return def;
    }

    auto variants = type->ops();
    if (std::none_of(variants.begin(), variants.end(), [&](auto t){ return t == def->type(); })) {
        return error();
    }

    return unify<Any>(1, *this, type, def, name);
}

const Def* build_match_type(const Def* /*def*/, const Variant* /*type*/, Defs /*handlers*/) {
    // TODO check handler types in a later type checking step?
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
    if (auto any = def->isa<Any>()) {
        auto any_def = any->def();
        return app(handlers[any->index()], any_def, name);
    }
    auto type = build_match_type(def, matched_type, handlers);
    // TODO implement reduction and caching
    return unify<Match>(1, *this, type, def, handlers, name);
}

const Def* World::app(const Def* callee, Defs args, const std::string& name) {
    if (args.size() == 1) {
        auto single = args.front();
        if (auto tuple = single->isa<Tuple>())
            return app(callee, tuple->ops(), name);
        else if (auto sigma_type = single->type()->isa<Sigma>()) {
            Array<const Def*> extracts(sigma_type->num_ops());
            for (size_t i = 0; i < sigma_type->num_ops(); ++i) {
                extracts[i] = extract(single, i);
            }
            return app(callee, extracts, name);
        }
    }

    // TODO do this checking later during a separate type checking phase
    if (too_many_affine_uses({callee}) || too_many_affine_uses(args))
        return error();

    auto type = callee->type()->as<Pi>()->reduce(args);
    auto app = unify<App>(args.size() + 1, *this, type, callee, args, name);
    assert(app->destructee() == callee);

    if (auto cache = app->cache_)
        return cache;
    // Can only really reduce if it's not an Assume of Pi type
    if (auto lambda = callee->isa<Lambda>())
        return app->cache_ = lambda->reduce(args);
    else
        return app->cache_ = app;

    return app;
}

}
