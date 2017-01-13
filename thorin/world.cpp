#include "thorin/world.h"

namespace thorin {

bool World::alloc_guard_ = false;

World::World()
    : root_page_(new Page)
    , cur_page_(root_page_.get())
    , universe_({insert<Universe>(0, *this, Qualifier::Unrestricted),
                 insert<Universe>(0, *this, Qualifier::Affine),
                 insert<Universe>(0, *this, Qualifier::Relevant),
                 insert<Universe>(0, *this, Qualifier::Linear)})
    , star_({insert<Star>(0, *this, Qualifier::Unrestricted),
             insert<Star>(0, *this, Qualifier::Affine),
             insert<Star>(0, *this, Qualifier::Relevant),
             insert<Star>(0, *this, Qualifier::Linear)})
    , space_({insert<Space>(0, *this, Qualifier::Unrestricted),
              insert<Space>(0, *this, Qualifier::Affine),
              insert<Space>(0, *this, Qualifier::Relevant),
              insert<Space>(0, *this, Qualifier::Linear)})
    , nat_({axiom(star(Qualifier::Unrestricted),{"Nat"}),
            axiom(star(Qualifier::Affine),{"Nat"}),
            axiom(star(Qualifier::Relevant),{"Nat"}),
            axiom(star(Qualifier::Linear),{"Nat"})})
    , nats_({{{{nat( 0, Qualifier::Unrestricted), nat( 0, Qualifier::Affine), nat( 0, Qualifier::Relevant), nat( 0, Qualifier::Linear)}},
              {{nat( 1, Qualifier::Unrestricted), nat( 1, Qualifier::Affine), nat( 1, Qualifier::Relevant), nat( 1, Qualifier::Linear)}},
              {{nat( 2, Qualifier::Unrestricted), nat( 2, Qualifier::Affine), nat( 2, Qualifier::Relevant), nat( 2, Qualifier::Linear)}},
              {{nat( 4, Qualifier::Unrestricted), nat( 4, Qualifier::Affine), nat( 4, Qualifier::Relevant), nat( 4, Qualifier::Linear)}},
              {{nat( 8, Qualifier::Unrestricted), nat( 8, Qualifier::Affine), nat( 8, Qualifier::Relevant), nat( 8, Qualifier::Linear)}},
              {{nat(16, Qualifier::Unrestricted), nat(16, Qualifier::Affine), nat(16, Qualifier::Relevant), nat(16, Qualifier::Linear)}},
              {{nat(32, Qualifier::Unrestricted), nat(32, Qualifier::Affine), nat(32, Qualifier::Relevant), nat(32, Qualifier::Linear)}},
              {{nat(64, Qualifier::Unrestricted), nat(64, Qualifier::Affine), nat(64, Qualifier::Relevant), nat(64, Qualifier::Linear)}}}})
    , boolean_({axiom(star(Qualifier::Unrestricted),{"Boolean"}),
                axiom(star(Qualifier::Affine),{"Boolean"}),
                axiom(star(Qualifier::Relevant),{"Boolean"}),
                axiom(star(Qualifier::Linear),{"Boolean"})})
    , booleans_({{{{boolean(false, Qualifier::Unrestricted), boolean(false, Qualifier::Affine),
                    boolean(false, Qualifier::Relevant), boolean(false, Qualifier::Linear)}},
                  {{boolean( true, Qualifier::Unrestricted), boolean( true, Qualifier::Affine),
                    boolean( true, Qualifier::Relevant), boolean( true, Qualifier::Linear)}}}})
    , integer_({axiom(pi({nat(), nat()}, star(Qualifier::Unrestricted)), {"int"}),
                axiom(pi({nat(), nat()}, star(Qualifier::Affine      )), {"int"}),
                axiom(pi({nat(), nat()}, star(Qualifier::Relevant    )), {"int"}),
                axiom(pi({nat(), nat()}, star(Qualifier::Linear      )), {"int"})})
    , real_(axiom(pi({nat(), boolean()}, star()),{"real"}))
    , mem_(axiom(star(Qualifier::Linear),{"M"}))
    , frame_(axiom(star(Qualifier::Linear),{"F"}))
    , ptr_(axiom(pi({star(), nat()}, star(),{"ptr"})))
    , iarithop_type_(pi({nat(), nat()}, pi({
            integer(var(nat(), 1), var(nat(), 0)),
            integer(var(nat(), 2), var(nat(), 1))},
            integer(var(nat(), 3), var(nat(), 2)))))
    , rarithop_type_(pi({nat(), boolean()}, pi({
            real(var(nat(), 1), var(nat(), 0)),
            real(var(nat(), 2), var(nat(), 1))},
            real(var(nat(), 3), var(nat(), 2)))))
    //, icmpop_type_(pi({nat(), boolean(), boolean()}, pi(nat(), pi({
            //integer(var(nat(), 3), var(boolean(), 2), var(boolean(), 1)),
            //integer(var(nat(), 4), var(boolean(), 3), var(boolean(), 2))},
            //integer(var(nat(), 5), var(boolean(), 4), var(boolean(), 3))))))
    //, rcmpop_type_(pi({nat(), boolean()}, pi(nat(), pi({
            //real(var(nat(), 2), var(boolean(), 1)),
            //real(var(nat(), 3), var(boolean(), 2))}, type_uo1()))))
#if 0
#define CODE(x) \
    , x ## _(axiom(iarithop_type_, {# x}))
    THORIN_I_ARITHOP(CODE)
#undef CODE
#define CODE(x) \
    , x ## _(axiom(rarithop_type_, {# x}))
    THORIN_R_ARITHOP(CODE)
#undef CODE
#endif
{}

World::~World() {
    for (auto def : defs_)
        def->~Def();
}

const Def* World::proj(size_t index, size_t arity, Qualifier q, Debug dbg) {
    auto a = this->arity(arity, q);
    if (index < arity)
        return unify<Proj>(0, *this, a, index, dbg);
    return error(a);
}

const Def* World::variadic_sigma(const Def* body, Qualifier q, Debug dbg) {
    return unify<VariadicSigma>(1, *this, body, q, dbg);
}

const Def* World::variadic_tuple(const Def* body, Debug dbg) {
    assert(false && "TODO");
    //return unify<VariadicTuple>(1, *this, body, dbg);
}

const Pi* World::pi(Defs domains, const Def* body, Qualifier q, Debug dbg) {
    if (domains.size() == 1 && domains.front()->type()) {
        if (auto sigma = domains.front()->type()->isa<Sigma>())
            return pi(sigma->ops(), body, q, dbg);
    }

    return unify<Pi>(domains.size() + 1, *this, domains, body, q, dbg);
}

const Lambda* World::pi_lambda(const Pi* pi, const Def* body, Debug dbg) {
    assert(pi->body() == body->type());
    return unify<Lambda>(1, *this, pi, body, dbg);
}

const Def* single_qualified(Defs defs, Qualifier q) {
    assert(defs.size() == 1);
    auto single = defs.front();
    // TODO if we use automatic qualifier coercion/subtyping, need to allow it here as well
    assert(!single || single->qualifier() == q);
    return single;
}

const Def* World::sigma(Defs defs, Qualifier q, Debug dbg) {
    // TODO doing the following always loses Debug information
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Sigma>(defs.size(), *this, defs, q, dbg);
}

const Def* World::singleton(const Def* def, Debug dbg) {
    assert(def->type() && "Can't create singletons of universes.");
    // Normalize various cases
    if (def->type()->isa<Singleton>()) {
        return def->type();
    }
    if (!def->is_nominal()) {
        if (def->isa<Variant>()) {
            auto num_ops = def->num_ops();
            Array<const Def*> ops(num_ops);
            for (size_t i = 0; i < num_ops; ++i)
                ops[i] = singleton(def->op(i));
            return variant(ops, dbg);
        } else if (def->isa<Intersection>()) {
            // S(v : t ∩ u) : *
            // TODO Any normalization of a Singleton Intersection?
        }
    }
    if (auto sig = def->type()->isa<Sigma>()) {
        // See Harper PFPL 43.13b
        auto num_ops = sig->num_ops();
        Array<const Def*> ops(num_ops);
        for (size_t i = 0; i < num_ops; ++i)
            ops[i] = singleton(extract(def, i));
        return sigma(ops, sig->qualifier(), dbg);
    } else if (auto pi_type = def->type()->isa<Pi>()) {
        // See Harper PFPL 43.13c
        auto domains = pi_type->domains();
        auto num_domains = pi_type->num_domains();
        Array<const Def*> new_pi_vars(num_domains);
        for (size_t i = 0; i < num_domains; ++i)
            new_pi_vars[i] = var(domains[i], num_domains - i - 1);
        auto applied = app(def, new_pi_vars);
        return pi(domains, singleton(applied), pi_type->qualifier(), dbg);
    }
    // TODO other normalizations?
    return unify<Singleton>(1, *this, def, dbg);
}

const Def* World::tuple(const Def* type, Defs defs, Debug dbg) {
    auto expected_type = sigma(types(defs));
    const Def* found_structural_type = type;
    if (type->is_nominal())
        found_structural_type = sigma(type->ops());
    // TODO subtyping check instead of equality here
    // TODO error message with more precise information
    assert(found_structural_type == expected_type && "Can't give type to tuple.");
    if (defs.size() == 1) {
        return defs.front();
    }

    return unify<Tuple>(defs.size(), *this, type->as<Sigma>(), defs, dbg);
}

const Def* World::extract(const Def* def, size_t index, Debug dbg) {
    if (auto tuple_type = def->type()->isa<Sigma>()) {
        assert(index >= 0 && index < tuple_type->num_ops());
        if (auto tuple = def->isa<Tuple>()) {
            return tuple->op(index);
        }

        auto type = Tuple::extract_type(*this, def, index);
        return unify<Extract>(1, *this, type, def, index, dbg);
    } else {
        assert(index == 0);
        return def;
    }
}

const Def* World::intersection(Defs defs, Qualifier q, Debug dbg) {
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Intersection>(defs.size(), *this, defs, q, dbg);
}

const Def* World::pick(const Def* type, const Def* def, Debug dbg) {
    if (auto def_type = def->type()->isa<Intersection>()) {
        assert(std::any_of(def_type->ops().begin(), def_type->ops().end(), [&](auto t) { return t == type; })
               && "Picked type must be a part of the intersection type.");

        return unify<Pick>(1, *this, type, def, dbg);
    } else {
        assert(type == def->type());
        return def;
    }
}

const Def* World::variant(Defs defs, Qualifier q, Debug dbg) {
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Variant>(defs.size(), *this, defs, q, dbg);
}

const Def* World::any(const Def* type, const Def* def, Debug dbg) {
    if (!type->isa<Variant>()) {
        assert(type == def->type());
        return def;
    }

    auto variants = type->ops();
    assert(std::any_of(variants.begin(), variants.end(), [&](auto t){ return t == def->type(); })
           && "Type must be a part of the variant type.");

    return unify<Any>(1, *this, type->as<Variant>(), def, dbg);
}

const Def* build_match_type(World& w, const Def* /*def*/, const Variant* /*type*/, Defs handlers) {
    // TODO check handler types in a later type checking step?
    Array<const Def*> types(handlers.size());
    for (size_t i = 0; i < handlers.size(); ++i) {
        types[i] = handlers[i]->type()->as<Pi>()->body();
    }
    // We're not actually building a sum type here, we need uniqueness
    unique_gid_sort(&types);
    return w.variant(types);
}

const Def* World::match(const Def* def, Defs handlers, Debug dbg) {
    auto def_type = def->type();
    if (handlers.size() == 1) {
        assert(!def_type->isa<Variant>());
        return app(handlers.front(), def, dbg);
    }
    auto matched_type = def->type()->as<Variant>();
    assert(def_type->num_ops() == handlers.size() && "Not all Variant cases handled.");
#ifndef NDEBUG
    for (size_t i = 0; i < handlers.size(); ++i)
        assertmsg(handlers[i]->type()->as<Pi>()->domain() == matched_type->op(i),
                  "Handler % with type % does not match type %", i, handlers[i]->type(),
                  matched_type->op(i));
#endif
    if (auto any = def->isa<Any>()) {
        auto any_def = any->def();
        return app(handlers[any->index()], any_def, dbg);
    }
    auto type = build_match_type(*this, def, matched_type, handlers);
    return unify<Match>(1, *this, type, def, handlers, dbg);
}

const Def* World::app(const Def* callee, Defs args, Debug dbg) {
    if (args.size() == 1) {
        auto single = args.front();
        if (auto tuple = single->isa<Tuple>())
            return app(callee, tuple->ops(), dbg);
        else if (auto sigma_type = single->type()->isa<Sigma>()) {
            Array<const Def*> extracts(sigma_type->num_ops());
            for (size_t i = 0; i < sigma_type->num_ops(); ++i) {
                extracts[i] = extract(single, i);
            }
            return app(callee, extracts, dbg);
        }
    }

    // TODO do this checking later during a separate type checking phase
    if (too_many_affine_uses({callee}) || too_many_affine_uses(args))
        return error(callee->type()->as<Pi>()->body());

    // TODO what if args types don't match the domains? error?
    auto type = callee->type()->as<Pi>()->reduce(args);
    auto app = unify<App>(args.size() + 1, *this, type, callee, args, dbg);
    assert(app->destructee() == callee);

    return app->try_reduce();
}

}
