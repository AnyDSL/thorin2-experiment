#include <algorithm>
#include <functional>

#include "thorin/world.h"
#include "thorin/reduce.h"
#include "thorin/matchers/types.h"

namespace thorin {

//------------------------------------------------------------------------------

/*
 * helpers
 */

static bool is_homogeneous(Defs defs) {
    return std::all_of(defs.begin() + 1, defs.end(), [&](auto def) { return def == defs.front(); });
}

static bool any_of(const Def* def, Defs defs) {
    return std::any_of(defs.begin(), defs.end(), [&](auto d){ return d == def; });
}

static bool is_qualifier(const Def* def) { return def->type() == def->world().qualifier_type(); }

template<bool use_glb>
const Def* WorldBase::bound(Defs defs, const Def* q, bool require_qualifier) {
    if (defs.empty())
        return star(q ? q : use_glb ? linear() : unlimited());

    auto inferred_q = defs.front()->qualifier();
    auto max_type = defs.front()->type();

    for (auto def : defs.skip_front()) {
        assertf(!def->is_value(), "can't have values as operands here");
        assertf(def->sort() != Def::Sort::Universe, "type universes must not be operands");

        if (use_glb)
            inferred_q = intersection(qualifier_type(), {inferred_q, def->qualifier()});
        else
            inferred_q = variant(qualifier_type(), {inferred_q, def->qualifier()});

        if (def->type() == universe() || max_type == universe()) {
            max_type = universe();
            break;
        }
        else if (def->type() == arities() && max_type == arities())
            // found at least two arities, must be a multi-arity
            max_type = multi_arities();
        else {
            max_type = star(inferred_q);
        }
    }

    if (max_type->isa<Star>()) {
        if (!require_qualifier)
            return star(q ? q : use_glb ? linear() : unlimited());
        if (q == nullptr) {
            // no provided qualifier, so we use the inferred one
            assert(!max_type || max_type->op(0) == inferred_q);
            return max_type;
        } else {
#ifndef NDEBUG
            if (auto qual_axiom = isa_const_qualifier(inferred_q)) {
                auto box_qual = qual_axiom->box().get_qualifier();
                if (auto q_axiom = isa_const_qualifier(q)) {
                    auto qual = q_axiom->box().get_qualifier();
                    auto test = use_glb ? qual <= box_qual : qual >= box_qual;
                    assertf(test, "qualifier must be {} than the {} of the operands' qualifiers",
                            use_glb ? "less" : "greater",
                            use_glb ? "greatest lower bound" : "least upper bound");
                }
            }
#endif
            return star(q);
        }
    }
    return max_type;
}

template<bool use_glb>
const Def* WorldBase::qualifier_bound(Defs defs, std::function<const Def*(Defs)> unify_fn) {
    auto const_elem = use_glb ? Qualifier::Unlimited : Qualifier::Linear;
    auto ident_elem = use_glb ? Qualifier::Linear : Qualifier::Unlimited;
    size_t num_defs = defs.size();
    DefArray reduced(num_defs);
    Qualifier accu = Qualifier::Unlimited;
    size_t num_const = 0;
    for (size_t i = 0, e = num_defs; i != e; ++i) {
        if (auto q = isa_const_qualifier(defs[i])) {
            auto qual = q->box().get_qualifier();
            accu = use_glb ? meet(accu, qual) : join(accu, qual);
            num_const++;
        } else {
            assert(is_qualifier(defs[i]));
            reduced[i - num_const] = defs[i];
        }
    }
    if (num_const == num_defs)
        return qualifier(accu);
    if (accu == const_elem) {
        // glb(U, x) = U/lub(L, x) = L
        return qualifier(const_elem);
    } else if (accu != ident_elem) {
        // glb(L, x) = x/lub(U, x) = x, so otherwise we need to add accu
        assert(num_const != 0);
        reduced[num_defs - num_const] = qualifier(accu);
        num_const--;
    }
    reduced.shrink(num_defs - num_const);
    if (reduced.size() == 1)
        return reduced[0];
    return unify_fn(reduced);
}

//------------------------------------------------------------------------------

/*
 * WorldBase
 */

bool WorldBase::alloc_guard_ = false;

WorldBase::WorldBase()
    : root_page_(new Zone)
    , cur_page_(root_page_.get())
{
    universe_ = insert<Universe>(0, *this);
    qualifier_kind_ = axiom(universe_, {"ℚₖ"});
    qualifier_type_ = axiom(qualifier_kind_, {"ℚ"});
    unit_kind_ = insert<Sigma>(0, *this, universe_, Defs(), Debug("Σ*"));
    for (size_t i = 0; i != 4; ++i) {
        auto q = Qualifier(i);
        qualifier_[i] = assume(qualifier_type(), {q}, {qualifier_cstr(q)});
        star_     [i] = insert<Star >(1, *this, qualifier_[i]);
        unit_     [i] = insert<Sigma>(0, *this, star_[i], Defs(), Debug("Σ()"));
        tuple0_   [i] = insert<Tuple>(0, *this, unit_[i], Defs(), Debug("()"));
    }
    arities_ = insert<Arities>(0, *this);
    multi_arities_ = insert<MultiArities>(0, *this);
}

WorldBase::~WorldBase() {
    for (auto def : defs_)
        def->~Def();
}

const Def* WorldBase::any(const Def* type, const Def* def, Debug dbg) {
    if (!type->isa<Variant>()) {
        assert(type == def->type());
        return def;
    }

    auto variants = type->ops();
    assert(any_of(def->type(), variants) && "type must be a part of the variant type");

    return unify<Any>(1, *this, type->as<Variant>(), def, dbg);
}

const Axiom* WorldBase::arity(size_t a, Location location) {
    auto cur = Def::gid_counter();
    auto result = assume(arities(), {u64(a)}, {location});

    if (result->gid() >= cur)
        result->debug().set(std::to_string(a) + "ₐ");

    return result;
}

const Def* WorldBase::app(const Def* callee, Defs args, Debug dbg) {
    if (args.size() == 1) {
        auto single = args.front();
        if (auto tuple = single->isa<Tuple>())
            return app(callee, tuple->ops(), dbg);

        if (auto sigma = single->type()->isa<Sigma>()) {
            auto extracts = DefArray(sigma->num_ops(), [&](auto i) { return this->extract(single, i); });
            return app(callee, extracts, dbg);
        }

        if (auto variadic = single->type()->isa<Variadic>()) {
            if (auto arity = variadic->arity()->isa<Axiom>()) {
                auto extracts = DefArray(arity->box().get_u64(), [&](auto i) { return this->extract(single, i); });
                return app(callee, extracts, dbg);
            }
        }
    }

    auto callee_type = callee->type()->as<Pi>();
    assertf(callee_type->domain()->assignable(args),
            "callee with domain {} cannot be called with arguments {}", callee_type->domain(), args);
    auto type = callee_type->reduce(args);
    auto app = unify<App>(args.size() + 1, *this, type, callee, args, dbg);
    assert(app->callee() == callee);

    return app->try_reduce();
}

const Def* WorldBase::extract(const Def* def, const Def* i, Debug dbg) {
    assert(!def->is_universe());
    auto a = def->arity();
    assertf(i->type() == a, "arity {} of value {} does not match type {} of index {} for extract",
            a, def, i->type(), i);
    if (auto assume = i->isa<Axiom>())
        return extract(def, assume->box().get_u64(), dbg);

    if (auto variadic = def->isa<Variadic>()) {
        if (!variadic->body()->free_vars().test(0))
            return variadic->body()->shift_free_vars(1);
    }

    if (auto pack = def->isa<Pack>()) {
        if (!pack->body()->free_vars().test(0))
            return pack->body()->shift_free_vars(1);
    }

    auto type = def->type();
    if (def->is_value())
        type = extract(def->type(), i);

    return unify<Extract>(2, *this, type, def, i, dbg);
}

const Def* WorldBase::extract(const Def* def, size_t i, Debug dbg) {
    if (def->isa<Tuple>())
        return def->op(i);

    if (auto sigma = def->isa<Sigma>()) {
        assert(!sigma->is_dependent());
        return sigma->op(i);
    }

    if (auto variadic = def->isa<Variadic>())
        return variadic->body()->reduce(index(variadic->arity()->as<Axiom>()->box().get_u64(), i));

    if (auto pack = def->isa<Pack>())
        return pack->body()->reduce(index(pack->arity()->as<Axiom>()->box().get_u64(), i));

    if (auto sigma = def->type()->isa<Sigma>()) {
        auto type = sigma->op(i);
        if (type->free_vars().any_end(i)) {
            size_t skipped_shifts = 0;
            for (size_t delta = 1; delta <= i; ++delta) {
                if (type->free_vars().none_begin(skipped_shifts)) {
                    ++skipped_shifts;
                    continue;
                }

                // this also shifts any Var with i > skipped_shifts by -1
                type = type->reduce(extract(def, i - delta), skipped_shifts);
            }
        }

        return unify<Extract>(2, *this, type, def, index(sigma->num_ops(), i, dbg), dbg);
    }

    if (auto v = def->type()->isa<Variadic>()) {
        auto a = v->arity()->as<Axiom>()->box().get_u64();
        assertf(i < a, "index {} not provably in arity {}", i, a);
        auto idx = index(a, i, dbg);
        return unify<Extract>(2, *this, v->body()->reduce(idx), def, idx, dbg);
    }

    assert(i == 0);
    return def;
}

const Def* WorldBase::index(size_t a, size_t i, Location location) {
    if (i < a) {
        auto cur = Def::gid_counter();
        auto result = assume(arity(a), {u64(i)}, {location});

        if (result->gid() >= cur) { // new assume -> build name
            std::string s = std::to_string(i);
            auto b = s.size();

            // append utf-8 subscripts in reverse order
            for (size_t aa = a; aa > 0; aa /= 10)
                ((s += char(char(0x80) + char(aa % 10))) += char(0x82)) += char(0xe2);

            std::reverse(s.begin() + b, s.end());
            result->debug().set(s);
        }

        return result;
    }

    return error(arity(a));
}

const Def* WorldBase::insert(const Def* def, const Def* i, const Def* value, Debug dbg) {
    // TODO type check insert node
    return unify<Insert>(2, *this, def->type(), def, i, value, dbg);
}

const Def* WorldBase::insert(const Def* def, size_t i, const Def* value, Debug dbg) {
    auto idx = index(def->arity()->as<Axiom>()->box().get_u64(), i);
    return insert(def, idx, value, dbg);
}

const Def* WorldBase::intersection(Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    return intersection(glb(defs, nullptr), defs, dbg);
}

const Def* WorldBase::intersection(const Def* type, Defs defs, Debug dbg) {
    assert(defs.size() > 0); // TODO empty intersection -> empty type/kind
    if (defs.size() == 1) {
        assert(defs.front()->type() == type);
        return defs.front();
    }
    // implements a least upper bound on qualifiers,
    // could possibly be replaced by something subtyping-generic
    if (is_qualifier(defs.front())) {
        assert(type == qualifier_type());
        return qualifier_glb(defs, [&] (Defs defs) {
            return unify<Intersection>(defs.size(), *this, qualifier_type(), defs, dbg);
        });
    }

    // TODO recognize some empty intersections? i.e. same sorted ops, intersection of types non-empty?
    return unify<Intersection>(defs.size(), *this, type, defs, dbg);
}

const Pi* WorldBase::pi(Defs domains, const Def* body, const Def* q, Debug dbg) {
    if (domains.size() == 1 && !domains.front()->is_nominal()) {
        auto domain = domains.front();

        if (auto sigma = domain->isa<Sigma>())
            return pi(sigma->ops(), flatten(body, sigma->ops()), q, dbg);

        if (auto v = domain->isa<Variadic>()) {
            if (auto arity = v->arity()->isa<Axiom>()) {
                auto a = arity->box().get_u64();
                assert(!v->body()->free_vars().test(0));
                DefArray args(a, [&] (auto i) { return v->body()->shift_free_vars(i); });
                return pi(args, flatten(body, args), q, dbg);
            }
        }
    }

    auto type = lub(concat(domains, body), q, false);

    return unify<Pi>(domains.size() + 1, *this, type, domains, body, dbg);
}

const Def* WorldBase::pick(const Def* type, const Def* def, Debug dbg) {
    if (auto def_type = def->type()->isa<Intersection>()) {
        assert(any_of(type, def_type->ops()) && "picked type must be a part of the intersection type");
        return unify<Pick>(1, *this, type, def, dbg);
    }

    assert(type == def->type());
    return def;
}

const Def* WorldBase::lambda(Defs domains, const Def* body, const Def* type_qualifier, Debug dbg) {
    auto p = pi(domains, body->type(), type_qualifier, dbg);
    if (p->domains().size() != domains.size())
        return lambda(p->domains(), flatten(body, p->domains()), type_qualifier, dbg);

    if (auto app = body->isa<App>()) {
        size_t n = app->num_args();

        auto eta_property = [&]() {
            for (size_t i = 0; i != n; ++i) {
                if (!app->arg(i)->isa<Var>() || n-1-app->arg(i)->as<Var>()->index() != i)
                    return false;
            }
            return true;
        };

        if (app->callee()->free_vars().none_range(0, n) && eta_property())
            return app->callee()->shift_free_vars(n);
    }

    return unify<Lambda>(1, *this, p, body, dbg);
}

Lambda* WorldBase::nominal_lambda(Defs domains, const Def* codomain, const Def* type_qualifier, Debug dbg) {
    auto l = insert<Lambda>(1, *this, pi(domains, codomain, type_qualifier, dbg), dbg);
    l->normalize_ = l->type()->domains().size() != domains.size();
    return l;
}

const Def* WorldBase::variadic(const Def* arity, const Def* body, Debug dbg) {
    if (auto sigma = arity->isa<Sigma>()) {
        assertf(!sigma->is_nominal(), "can't have nominal sigma arities");
        return variadic(sigma->ops(), flatten(body, sigma->ops()), dbg);
    }

    if (auto v = arity->isa<Variadic>()) {
        if (auto axiom = v->arity()->isa<Axiom>()) {
            assert(!v->body()->free_vars().test(0));
            auto a = axiom->box().get_u64();
            assert(a != 1);
            auto result = flatten(body, DefArray(a, v->body()->shift_free_vars(1-a)));
            for (size_t i = a; i-- != 0;)
                result = variadic(v->body()->shift_free_vars(-i+1), result, dbg);
            return result;
        }
    }

    if (auto axiom = arity->isa<Axiom>()) {
        auto a = axiom->box().get_u64();
        if (a == 0) {
            if (body->is_kind())
                return unit_kind();
            return unit(body->type()->qualifier());
        }
        if (a == 1) return body->reduce(this->index(1, 0));
        if (body->free_vars().test(0))
            return sigma(DefArray(a, [&](auto i) {
                        return body->reduce(this->index(a, i))->shift_free_vars(-i); }), dbg);
    }

    assert(body->type()->is_kind() || body->type()->is_universe());
    return unify<Variadic>(2, *this, body->type()->shift_free_vars(1), arity, body, dbg);
}

const Def* WorldBase::variadic(Defs arity, const Def* body, Debug dbg) {
    if (arity.empty())
        return body;
    return variadic(arity.skip_back(), variadic(arity.back(), body, dbg), dbg);
}

const Def* WorldBase::sigma(const Def* q, Defs defs, Debug dbg) {
    auto type = lub(defs, q);
    if (defs.size() == 0)
        return unit(type->qualifier());

    if (type == multi_arities()) {
        if (any_of(arity(0), defs))
            return arity(0);
    }

    if (defs.size() == 1) {
        assertf(defs.front()->type() == type, "type {} and inferred type {} don't match",
                defs.front()->type(), type);
        return defs.front();
    }

    if (defs.front()->free_vars().none_end(defs.size() - 1) && is_homogeneous(defs)) {
        assert(q == nullptr || defs.front()->qualifier() == q);
        return variadic(arity(defs.size(), dbg), defs.front()->shift_free_vars(-1), dbg);
    }

    return unify<Sigma>(defs.size(), *this, type, defs, dbg);
}

const Def* WorldBase::singleton(const Def* def, Debug dbg) {
    assert(def->type() && "can't create singletons of universes");

    if (def->type()->isa<Singleton>())
        return def->type();

    if (!def->is_nominal()) {
        if (def->isa<Variant>()) {
            auto ops = DefArray(def->num_ops(), [&](auto i) { return this->singleton(def->op(i)); });
            return variant(def->type()->type(), ops, dbg);
        }

        if (def->isa<Intersection>()) {
            // S(v : t ∩ u) : *
            // TODO Any normalization of a Singleton Intersection?
        }
    }

    if (auto sig = def->type()->isa<Sigma>()) {
        // See Harper PFPL 43.13b
        auto ops = DefArray(sig->num_ops(), [&](auto i) { return this->singleton(this->extract(def, i)); });
        return sigma(sig->qualifier(), ops, dbg);
    }

    if (auto pi_type = def->type()->isa<Pi>()) {
        // See Harper PFPL 43.13c
        auto domains = pi_type->domains();
        auto num_domains = pi_type->num_domains();
        auto new_pi_vars = DefArray(num_domains,
                [&](auto i) { return this->var(domains[i], num_domains - i - 1); });
        auto applied = app(def, new_pi_vars);
        return pi(domains, singleton(applied), pi_type->qualifier(), dbg);
    }

    return unify<Singleton>(1, *this, def, dbg);
}

const Def* WorldBase::pack_nominal_sigma(const Sigma* sigma, const Def* body, Debug dbg) {
    size_t arity = sigma->num_ops();
    if (body->free_vars().test(0))
        return tuple(sigma, DefArray(arity, [&](auto i) { return body->reduce(this->index(arity, i)); }), dbg);

    // TODO type checking
    return unify<Pack>(1, *this, sigma, body, dbg);
}

const Def* WorldBase::pack(const Def* arity, const Def* body, Debug dbg) {
    if (auto sigma = arity->isa<Sigma>())
        return pack(sigma->ops(), flatten(body, sigma->ops()), dbg);


    if (auto v = arity->isa<Variadic>()) {
        if (auto axiom = v->arity()->isa<Axiom>()) {
            assert(!v->body()->free_vars().test(0));
            auto a = axiom->box().get_u64();
            assert(a != 1);
            auto result = flatten(body, DefArray(a, v->body()->shift_free_vars(1-a)));
            for (size_t i = a; i-- != 0;)
                result = pack(v->body()->shift_free_vars(-i+1), result, dbg);
            return result;
        }
    }

    if (auto axiom = arity->isa<Axiom>()) {
        auto a = axiom->box().get_u64();
        if (a == 0) {
            if (body->is_type())
                return tuple(unit_kind(), {});
            return tuple0(body->type()->qualifier());
        }
        if (a == 1) return body->reduce(this->index(1, 0));
        if (body->free_vars().test(0))
            return tuple(DefArray(a, [&](auto i) { return body->reduce(this->index(a, i)); }), dbg);
    }

    if (auto extract = body->isa<Extract>()) {
        if (auto var = extract->index()->isa<Var>()) {
            if (var->index() == 0 && !extract->scrutinee()->free_vars().test(0))
                return extract->scrutinee()->shift_free_vars(1);
        }
    }

    assert(body->is_term() || body->is_type());
    return unify<Pack>(1, *this, variadic(arity, body->type()), body, dbg);
}

const Def* WorldBase::pack(Defs arity, const Def* body, Debug dbg) {
    if (arity.empty())
        return body;
    return pack(arity.skip_back(), pack(arity.back(), body, dbg), dbg);
}

const Def* WorldBase::tuple(const Def* type, Defs defs, Debug dbg) {
    assertf(type->assignable(defs),
            "can't assign type {} to tuple with type {}", type, sigma(types(defs)));
    size_t size = defs.size();

    if (size == 1 && (!type->is_nominal() || type == defs.front()->type()))
        return defs.front();

    auto eta_property = [&]() {
        const Def* same = nullptr;
        for (size_t i = 0; i != size; ++i) {
            if (auto extract = defs[i]->isa<Extract>()) {
                if (same == nullptr)
                    same = extract->scrutinee();

                if (same == extract->scrutinee()) {
                    if (auto index = extract->index()->isa<Axiom>()) {
                        if (index->box().get_u64() == i)
                            continue;
                    }
                }
            }
            return (const Def*)nullptr;
        }
        return same;
    };

    if (size != 0) {
        if (is_homogeneous(defs))
            return type->is_nominal()
                ? pack_nominal_sigma(type->as<Sigma>(), defs.front()->shift_free_vars(-1), dbg)
                : pack(arity(size, dbg), defs.front()->shift_free_vars(-1), dbg);
        else if (auto same = eta_property())
            return same;
    }

    return unify<Tuple>(size, *this, type->as<SigmaBase>(), defs, dbg);
}

const Def* WorldBase::variant(Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    return variant(lub(defs, nullptr), defs, dbg);
}

const Def* WorldBase::variant(const Def* type, Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    if (defs.size() == 1) {
        assert(defs.front()->type() == type);
        return defs.front();
    }
    // implements a least upper bound on qualifiers,
    // could possibly be replaced by something subtyping-generic
    if (is_qualifier(defs.front())) {
        assert(type == qualifier_type());
        return qualifier_lub(defs, [&] (Defs defs) {
            return unify<Variant>(defs.size(), *this, qualifier_type(), defs, dbg);
        });
    }

    return unify<Variant>(defs.size(), *this, type, defs, dbg);
}

static const Def* build_match_type(WorldBase& w, Defs handlers) {
    auto types = DefArray(handlers.size(),
            [&](auto i) { return handlers[i]->type()->template as<Pi>()->body(); });
    // We're not actually building a sum type here, we need uniqueness
    unique_gid_sort(&types);
    return w.variant(types);
}

const Def* WorldBase::match(const Def* def, Defs handlers, Debug dbg) {
    auto def_type = def->type();
    if (handlers.size() == 1) {
        assert(!def_type->isa<Variant>());
        return app(handlers.front(), def, dbg);
    }
    auto matched_type = def->type()->as<Variant>();
    assert(def_type->num_ops() == handlers.size() && "number of handlers does not match number of cases");

    DefArray sorted_handlers(handlers);
    std::sort(sorted_handlers.begin(), sorted_handlers.end(),
              [](const Def* a, const Def* b) {
                  auto a_dom = a->type()->as<Pi>()->domain();
                  auto b_dom = b->type()->as<Pi>()->domain();
                  return a_dom->gid() < b_dom->gid(); });
#ifndef NDEBUG
    for (size_t i = 0; i < sorted_handlers.size(); ++i) {
        auto domain = sorted_handlers[i]->type()->as<Pi>()->domain();
        assertf(domain == matched_type->op(i), "Handler {} with domain {} does not match type {}", i, domain,
                matched_type->op(i));
    }
#endif
    if (auto any = def->isa<Any>()) {
        auto any_def = any->def();
        return app(sorted_handlers[any->index()], any_def, dbg);
    }
    auto type = build_match_type(*this, sorted_handlers);
    return unify<Match>(1, *this, type, def, sorted_handlers, dbg);
}

//------------------------------------------------------------------------------

/*
 * World
 */

World::World() {
    auto Q = qualifier_type();
    auto U = qualifier(Qualifier::Unlimited);
    auto B = type_bool_ = axiom(star(), {"bool"});
    auto N = type_nat_  = axiom(star(), {"nat" });
    auto S = star();
    auto MA = multi_arities();

    type_i_ = axiom(pi({Q, N, N}, star(var(Q, 2))), {"int" });
    type_r_ = axiom(pi({Q, N, N}, star(var(Q, 2))), {"real"});

    val_bool_[0] = assume(B, {false}, {"⊥"});
    val_bool_[1] = assume(B, {true }, {"⊤"});
    val_nat_0_   = val_nat(0);
    for (size_t j = 0; j != val_nat_.size(); ++j)
        val_nat_[j] = val_nat(1 << int64_t(j));

    type_ptr_ = axiom(pi({S, N}, S), {"ptr"});
    auto M = type_mem_   = axiom(star(Qualifier::Linear), {"M"});
    auto F = type_frame_ = axiom(S, {"F"});

#define VAR(x, X, n)                 \
    auto T_CAT(v, x, n) = var(X, n); \
    (void)T_CAT(v, x, n);
#define CODE(x, X) \
    VAR(x, X, 0) VAR(x, X, 1) VAR(x, X, 2) VAR(x, X, 3) VAR(x, X, 4) VAR(x, X, 5) VAR(x, X, 6)
    CODE(a, MA) CODE(q, Q) CODE(n, N) CODE(s, S)
#undef CODE
#undef VAR

    // type_i
#define CODE(r, x) \
    T_CAT(type_, T_CAT(x), _) = \
        type_i(Qualifier::u, iflags::T_ELEM(0, x), T_ELEM(1, x));
    T_FOR_EACH_PRODUCT(CODE, (THORIN_I_FLAGS)(THORIN_I_WIDTH))
#undef CODE

    // type_r
#define CODE(r, x) \
    T_CAT(type_, T_CAT(x), _) = \
        type_r(Qualifier::u, rflags::T_ELEM(0, x), T_ELEM(1, x));
    T_FOR_EACH_PRODUCT(CODE, (THORIN_R_FLAGS)(THORIN_R_WIDTH))
#undef CODE

    auto i1 = variadic(va3, type_i(vq3, vn2, vn1)); auto r1 = variadic(va3, type_r(vq3, vn2, vn1));
    auto i2 = variadic(va4, type_i(vq4, vn3, vn2)); auto r2 = variadic(va4, type_r(vq4, vn3, vn2));
    auto i3 = variadic(va5, type_i(vq5, vn4, vn3)); auto r3 = variadic(va5, type_r(vq5, vn4, vn3));
    auto i_type_arithop = pi(MA, pi({Q, N, N}, pi({i1, i2}, i3)));
    auto r_type_arithop = pi(MA, pi({Q, N, N}, pi({r1, r2}, r3)));

    // arithop axioms
#define CODE(r, ir, x) \
    T_CAT(op_, x, _) = axiom(T_CAT(ir, _type_arithop), {T_STR(x)});
    T_FOR_EACH(CODE, i, THORIN_I_ARITHOP)
    T_FOR_EACH(CODE, r, THORIN_R_ARITHOP)
#undef CODE

    // arithop table
#define CODE(r, ir, x)                                                           \
    for (size_t f = 0; f != size_t(T_CAT(ir, flags)::Num); ++f) {                \
        for (size_t w = 0; w != size_t(T_CAT(ir, width)::Num); ++w) {            \
            auto flags = val_nat(f);                                             \
            auto width = val_nat(T_CAT(index2, ir, width)(w));                   \
            T_CAT(op_, x, s_)[f][w] = T_CAT(op_, x)(U, flags, width)->as<App>(); \
        }                                                                        \
    }
    T_FOR_EACH(CODE, i, THORIN_I_ARITHOP)
    T_FOR_EACH(CODE, r, THORIN_R_ARITHOP)
#undef CODE

    i1 = variadic(va4, type_i(vq3, vn2, vn1)); r1 = variadic(va4, type_r(vq3, vn2, vn1));
    i2 = variadic(va5, type_i(vq4, vn3, vn2)); r2 = variadic(va5, type_r(vq4, vn3, vn2));
    auto b = variadic(va6, type_i(vq5, val_nat(int64_t(iflags::uo)), val_nat_1()));
    auto i_type_cmp = pi(MA, pi(N, pi({Q, N, N}, pi({i1, i2}, b))));
    auto r_type_cmp = pi(MA, pi(N, pi({Q, N, N}, pi({r1, r2}, b))));
    op_icmp_ = axiom(i_type_cmp, {"icmp"});
    op_rcmp_ = axiom(r_type_cmp, {"rcmp"});

    // all cmp relations
#define CODE(r, ir, x) \
    T_CAT(op_, ir, cmp_, x, _) = app(app(T_CAT(op_, ir, cmp_), arity(1)), val_nat(int64_t(T_CAT(ir, rel)::x)), {T_STR(T_CAT(ir, cmp_, x))})->as<App>();
    T_FOR_EACH(CODE, i, THORIN_I_REL)
    T_FOR_EACH(CODE, r, THORIN_R_REL)
#undef CODE

    // cmp table
#define CODE(ir)                                                                                       \
    for (size_t r = 0; r != size_t(T_CAT(ir, rel)::Num); ++r) {                                        \
        for (size_t f = 0; f != size_t(T_CAT(ir, flags)::Num); ++f) {                                  \
            for (size_t w = 0; w != size_t(T_CAT(ir, width)::Num); ++w) {                              \
                auto rel = val_nat(r);                                                                 \
                auto flags = val_nat(f);                                                               \
                auto width = val_nat(T_CAT(index2, ir, width)(w));                                     \
                T_CAT(op_, ir, cmps_)[r][f][w] = T_CAT(op_, ir, cmp)(rel, U, flags, width)->as<App>(); \
            }                                                                                          \
        }                                                                                              \
    }
    CODE(i)
    CODE(r)
#undef CODE

    op_enter_ = axiom(pi(M, sigma({M, F})), {"enter"});
    {
        op_lea_ = axiom(pi({MA, variadic(va0, S), N},
                           pi({type_ptr(variadic(va2, extract(var(variadic(va3, S), 2), var(va3, 0))), vn0), va3},
                              type_ptr(extract(var(variadic(va4, S), 3), var(va4, 0)), vn2))), {"lea"});
    }
    {
        auto p = type_ptr(vs2, vn1);
        auto r = sigma({M, vs4});
        op_load_ = axiom(pi({S, N}, pi({M, p}, r)), {"load"});
    }
    {
        auto p = type_ptr(vs3, vn2);
        op_slot_ = axiom(pi({S, N}, pi({F, N}, p)), {"slot"});
    }
    {
        auto p = type_ptr(vs2, vn1);
        op_store_ = axiom(pi({S, N}, pi({M, p, vs3}, M)), {"store"});
    }
}

const Axiom* World::val_nat(int64_t val, Location location) {
    auto cur = Def::gid_counter();
    auto result = assume(type_nat(), {val}, {location});
    if (result->gid() >= cur)
        result->debug().set(std::to_string(val));
    return result;
}

#define CODE(r, ir, x)                                                    \
const App* World::T_CAT(op_, x)(const Def* a, const Def* b) {             \
    T_CAT(ir, Type) t(a->type());                                         \
    auto callee = t.is_const() && t.const_qualifier() == Qualifier::u     \
        ? T_CAT(op_, x)(             *t.const_flags(), *t.const_width())  \
        : T_CAT(op_, x)(t.qualifier(), t.     flags(), t.       width()); \
    return app(callee, {a, b})->as<App>();                                \
}
T_FOR_EACH(CODE, Int, THORIN_I_ARITHOP)
T_FOR_EACH(CODE, Real, THORIN_R_ARITHOP)
#undef CODE

const Def* World::op_enter(const Def* mem, Debug dbg) {
    return app(op_enter_, mem, dbg);
}

const Def* types_from_tuple_type(World& w, const Def* type) {
    if (auto sig = type->isa<Sigma>()) {
        return w.tuple(sig->ops());
    } else if (auto var = type->isa<Variadic>()) {
        return w.pack(var->arity(), var->body());
    }
    return type;
}

const Def* World::op_lea(const Def* ptr, const Def* index, Debug dbg) {
    PtrType ptr_type(ptr->type());
    auto types = types_from_tuple_type(*this, ptr_type.pointee());
    return app(app(op_lea_, {types->arity(), types, ptr_type.addr_space()}, dbg), {ptr, index}, dbg);
}

const Def* World::op_lea(const Def* ptr, size_t i, Debug dbg) {
    PtrType ptr_type(ptr->type());
    auto types = types_from_tuple_type(*this, ptr_type.pointee());
    auto idx = index(types->arity()->as<Axiom>()->box().get_u64(), i);
    return app(app(op_lea_, {types->arity(), types, ptr_type.addr_space()}, dbg), {ptr, idx}, dbg);
}

const Def* World::op_load(const Def* mem, const Def* ptr, Debug dbg) {
    PtrType ptr_type(ptr->type());
    return app(app(op_load_, {ptr_type.pointee(), ptr_type.addr_space()}, dbg), {mem, ptr}, dbg);
}

const Def* World::op_slot(const Def* type, const Def* frame, Debug dbg) {
    return app(app(op_slot_, {type, val_nat_0()}, dbg), {frame, val_nat(Def::gid_counter())}, dbg);
}

const Def* World::op_store(const Def* mem, const Def* ptr, const Def* val, Debug dbg) {
    PtrType ptr_type(ptr->type());
    return app(app(op_store_, {ptr_type.pointee(), ptr_type.addr_space()}, dbg), {mem, ptr, val}, dbg);
}

}
