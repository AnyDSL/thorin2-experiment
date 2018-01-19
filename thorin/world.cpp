#include <algorithm>
#include <functional>

#include "thorin/world.h"
#include "thorin/reduce.h"
#include "thorin/frontend/parser.h"
#include "thorin/core/normalize.h"

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

template<bool use_glb, class I>
const Def* World::bound(Range<I> defs, const Def* q, bool require_qualifier) {
    if (defs.distance() == 0)
        return star(q ? q : use_glb ? linear() : unlimited());

    auto first = *defs.begin();
    auto inferred_q = first->qualifier();
    auto max = first;

    auto iter = defs.begin();
    iter++;
    for (size_t i = 1, e = defs.distance(); i != e; ++i, ++iter) {
        auto def = *iter;
        assertf(!def->is_value(), "can't have value {} as operand of bound operator", def);

        if (def->qualifier()->free_vars().any_range(0, i)) {
            // qualifier is dependent within this type/kind, go to top directly
            // TODO might want to assert that this will always be a kind?
            inferred_q = use_glb ? linear() : unlimited();
        } else {
            auto qualifier = def->qualifier()->shift_free_vars(-i);

            inferred_q = use_glb ? intersection(qualifier_type(), {inferred_q, qualifier})
                : variant(qualifier_type(), {inferred_q, qualifier});
        }

        // TODO somehow build into a def->is_subtype_of(other)/similar
        bool is_arity = def->template isa<ArityKind>();
        bool is_star = def->template isa<Star>();
        bool is_marity = is_arity || def->template isa<MultiArityKind>();
        bool max_is_marity = max->template isa<ArityKind>() || max->template isa<MultiArityKind>();
        bool max_is_star = max->template isa<Star>();
        if (is_arity && max_is_marity)
            // found at least two arities, must be a multi-arity
            max = multi_arity_kind(inferred_q);
        else if ((is_star || is_marity) && (max_is_star || max_is_marity))
            max = star(inferred_q);
        else {
            max = universe();
            break;
        }
    }

    if (max->template isa<Star>()) {
        if (!require_qualifier)
            return star(q ? q : use_glb ? linear() : unlimited());
        if (q == nullptr) {
            // no provided qualifier, so we use the inferred one
            assert(!max || max->op(0) == inferred_q);
            return max;
        } else {
#ifndef NDEBUG
            if (auto i_qual = inferred_q->template isa<Qualifier>()) {
                auto iq = i_qual->qualifier_tag();
                if (auto q_qual = q->template isa<Qualifier>()) {
                    auto qual = q_qual->qualifier_tag();
                    auto test = use_glb ? qual <= iq : qual >= iq;
                    assertf(test, "qualifier must be {} than the {} of the operands' qualifiers",
                            use_glb ? "less" : "greater",
                            use_glb ? "greatest lower bound" : "least upper bound");
                }
            }
#endif
            return star(q);
        }
    }
    return max;
}

template<bool use_glb, class I>
const Def* World::qualifier_bound(Range<I> defs, std::function<const Def*(const SortedDefSet&)> unify_fn) {
    auto const_elem = use_glb ? QualifierTag::Unlimited : QualifierTag::Linear;
    auto ident_elem = use_glb ? QualifierTag::Linear : QualifierTag::Unlimited;
    size_t num_defs = defs.distance();
    DefArray reduced(num_defs);
    QualifierTag accu = QualifierTag::Unlimited;
    size_t num_const = 0;
    I iter = defs.begin();
    for (size_t i = 0, e = num_defs; i != e; ++i, ++iter) {
        if (auto q = (*iter)->template isa<Qualifier>()) {
            auto qual = q->qualifier_tag();
            accu = use_glb ? thorin::glb(accu, qual) : thorin::lub(accu, qual);
            num_const++;
        } else {
            assert(is_qualifier(*iter));
            reduced[i - num_const] = *iter;
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
    SortedDefSet set(reduced.begin(), reduced.end());
    return unify_fn(set);
}

const Def* normalize_arity_eliminator(thorin::World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg) {
    if (callee->type()->as<Pi>()->body()->isa<Pi>())
        return world.curry(normalize_arity_eliminator, type, callee, arg, dbg);
    arg->dump();
    const Def* pred = nullptr;
    if (auto arity = arg->isa<Arity>()) {
        auto arity_val = arity->value();
        if (arity_val == 0) {
            // callee = E P base f
            return callee->op(0)->op(1);
        }
        pred = world.arity(arity_val - 1);
    } else if (auto arity_app = arg->isa<App>(); arity_app->callee() == world.arity_succ()) {
        pred = arity_app->arg();
    }
    if (pred != nullptr)
        return world.app(world.app(callee->op(1), pred), world.app(callee, pred));
    return nullptr;
}

//------------------------------------------------------------------------------

bool World::alloc_guard_ = false;

World::World()
    : root_page_(new Zone)
    , cur_page_(root_page_.get())
{
    universe_ = insert<Universe>(0, *this);
    qualifier_type_ = insert<QualifierType>(0, *this);
    for (size_t i = 0; i != 4; ++i) {
        auto q = QualifierTag(i);
        qualifier_[i] = insert<Qualifier>(1, *this, q);
        star_[i] = insert<Star>(1, *this, qualifier_[i]);
        arity_kind_[i] = insert<ArityKind>(1, *this, qualifier_[i]);
        multi_arity_kind_[i] = insert<MultiArityKind>(1, *this, qualifier_[i]);
        unit_[i] = arity(1, qualifier_[i]);
        unit_val_[i] = index_zero(unit_[i]);
    }

    type_bool_ = arity(2);
    type_nat_  = axiom(star(), {"nat"});

    val_bool_[0] = index(2, 0);
    val_bool_[1] = index(2, 1);

    val_nat_0_   = val_nat(0);
    for (size_t j = 0; j != val_nat_.size(); ++j)
        val_nat_[j] = val_nat(1 << int64_t(j));

    Env env;
    arity_succ_ = axiom(parse(*this, "Π[q: ℚ, a: 𝔸(q)].𝔸(q)", env), {"Sₐ"});
    env["ASucc"] = arity_succ_;
    index_zero_ = axiom(parse(*this, "Πp:[q: ℚ, 𝔸(q)].ASucc p", env), {"0ⁱ"});
    env["I0"] = index_zero_;
    index_succ_ = axiom(parse(*this, "Πp:[q: ℚ, a: 𝔸(q)].Πa.ASucc p", env), {"Sⁱ"});
    env["IS"] = index_succ_;

    arity_eliminator_ = axiom(parse(*this, "Πq: ℚ.ΠP: [Π𝔸(q).*(q)].ΠP(0ₐ(q)).Π[Πa:𝔸(q).ΠP(a).P(ASucc (q,a))].Πa: 𝔸(q).P a", env), {"Eₐ"});
    arity_eliminator_->set_normalizer(normalize_arity_eliminator);
    arity_eliminator_arity_ = axiom(parse(*this, "Πq: ℚ.Π𝔸q.Π[Π𝔸q.Π𝔸q.𝔸q].Π𝔸q.𝔸q", env), {"R𝔸ₐ"});
    arity_eliminator_multi_ = axiom(parse(*this, "Πq: ℚ.Π𝕄q.Π[Π𝔸q.Π𝕄q.𝕄q].Π𝔸q.𝕄q", env), {"R𝕄ₐ"});
    arity_eliminator_star_ = axiom(parse(*this, "Πq: ℚ.Π*q.Π[Π𝔸q.Π*q.*q].Π𝔸q.*q", env), {"R*ₐ"});
    // index_eliminator_ = axiom(parse(*this, "Πq: ℚ.ΠP:[Πa:𝔸(q).Πa.*(q)].ΠP(0ₐ(q)).Π[Πa:𝔸(q).ΠP(a).P(ASucc (q,a))].Πa:𝔸(q).P a", env));
}

World::~World() {
    for (auto def : defs_)
        def->~Def();
}

const Def* World::any(const Def* type, const Def* def, Debug dbg) {
    if (!type->isa<Variant>()) {
        assert(type == def->type());
        return def;
    }

    auto variants = type->ops();
    assert(any_of(def->type(), variants) && "type must be a part of the variant type");

    return unify<Any>(1, *this, type->as<Variant>(), def, dbg);
}

const Arity* World::arity(size_t a, const Def* q, Location location) {
    assert(q->type() == qualifier_type());
    auto cur = Def::gid_counter();
    auto result = unify<Arity>(3, *this, arity_kind(q), a, location);

    if (result->gid() >= cur)
        result->debug().set(std::to_string(a) + "ₐ");

    return result;
}

const Def* World::arity_succ(const Def* a, Debug dbg) {
    if (auto a_lit = a->isa<Arity>()) {
        return arity(a_lit->value() + 1, a->qualifier(), dbg);
    }
    return app(arity_succ_, tuple({a->qualifier(), a}), dbg);
}

const Def* World::app(const Def* callee, const Def* arg, Debug dbg) {
    auto callee_type = callee->type()->as<Pi>();
    assertf(callee_type->domain()->assignable(arg),
            "callee {} with domain {} cannot be called with argument {} : {}", callee, callee_type->domain(), arg, arg->type());

    auto type = callee_type->apply(arg);

    if (auto normalizer = callee->normalizer()) {
        if (auto result = normalizer(*this, type, callee, arg, dbg))
            return result;
    }

    auto app = unify<App>(2, *this, type, callee, arg, dbg);
    assert(app->callee() == callee);

    return app->try_reduce();
}

const Def* World::extract(const Def* def, const Def* index, Debug dbg) {
    if (index->type() == arity(1))
        return def;
    // need to allow the above, as types are also a 1-tuple of a type
    assertf(def->is_value(), "can only build extracts of values, {} is not a value", def);
    auto arity = def->arity();
    auto type = def->type();
    assertf(arity, "arity unknown for {} of type {}, can only extract when arity is known", def, type);
    if (arity->assignable(index)) {
        if (auto idx = index->isa<Index>()) {
            auto i = idx->value();
            if (def->isa<Tuple>()) {
                return def->op(i);
            }

            if (auto sigma = type->isa<Sigma>()) {
                auto type = sigma->op(i);
                //size_t skipped_shifts = 0;
                for (size_t delta = 1; delta <= i; ++delta) {
                    //if (type->free_vars().none_begin(skipped_shifts)) {
                    //    ++skipped_shifts;
                    //    continue;
                    //}

                    // this also shifts any Var with i > skipped_shifts by -1
                    type = type->reduce(extract(def, i - delta));
                }
                return unify<Extract>(2, *this, type, def, index, dbg);
            }
        }
        // homogeneous tuples <v,...,v> are normalized to packs, so this also optimizes to v
        if (auto pack = def->isa<Pack>()) {
            return pack->body()->reduce(index);
        }
        // here: index is const => type is variadic, index is var => type may be variadic/sigma, must not be dependent sigma
        assert(!index->isa<Index>() || type->isa<Variadic>()); // just a sanity check for implementation errors above
        const Def* result_type = nullptr;
        if (auto sigma = type->isa<Sigma>()) {
            assertf(!sigma->is_dependent(), "can't extract at {} from {} : {}, type is dependent", index, def, sigma);
            if (sigma->type() == universe()) {
                // can only type those, that we can bound usefully
                auto bound = lub(sigma->ops(), nullptr, true);
                // universe may be a wrong bound, e.g. for (poly_identity, Nat) : [t:*->t->t, *] : □, but * and t:*->t-> not subtypes, thus can't derive a bound for this
                // TODO maybe infer variant? { t:*->t->t, * }?
                assertf(bound != universe(), "can't extract at {} from {} : {}, type may be □ (not reflectable)", index, def, sigma);
                result_type = bound;
            } else
                result_type = extract(tuple(sigma->ops(), dbg), index);
        } else
            result_type = type->as<Variadic>()->body()->reduce(index);

        return unify<Extract>(2, *this, result_type, def, index, dbg);
    }
    // not the same exact arity, but as long as it types, we can use indices from constant arity tuples, even of non-index type
    // can only extract if we can iteratively extract with each index in the multi-index
    // can only do that if we know how many elements there are
    if (auto i_arity = index->arity()->isa<Arity>()) {
        auto a = i_arity->value();
        if (a > 1) {
            auto extracted = def;
            for (size_t i = 0; i < a; ++i) {
                auto idx = extract(index, i, dbg);
                extracted = extract(extracted, idx, dbg);
            }
            return extracted;
        }
    }
    assertf(false, "can't extract at {} from {} : {}, index type {} not compatible",
            index, index->type(), def, type);
}

const Def* World::extract(const Def* def, size_t i, Debug dbg) {
    assertf(def->arity()->isa<Arity>(), "can only extract by size_t on constant arities");
    return extract(def, index(def->arity()->as<Arity>(), i, dbg), dbg);
}

const Index* World::index(const Arity* a, size_t i, Location location) {
    auto arity_val = a->value();
    assertf(i < arity_val, "Index literal {} does not fit within arity {}", i, a);
    auto cur = Def::gid_counter();
    auto result = unify<Index>(3, *this, a, i, location);

    if (result->gid() >= cur) { // new iassume -> build name
        std::string s = std::to_string(i);
        auto b = s.size();

        // append utf-8 subscripts in reverse order
        for (size_t aa = arity_val; aa > 0; aa /= 10)
            ((s += char(char(0x80) + char(aa % 10))) += char(0x82)) += char(0xe2);

        std::reverse(s.begin() + b, s.end());
        result->debug().set(s);
    }

    return result;
}

const Def* World::index_zero(const Def* arity, Location location) {
    assert(arity->type()->isa<ArityKind>());
    if (auto a = arity->isa<Arity>())
        return index(a->value() + 1, 0, location);

    return app(index_zero_, arity, {location});
}

const Def* World::index_succ(const Def* index, Debug dbg) {
    assert(index->type()->type()->isa<ArityKind>());
    if (auto idx = index->isa<Index>())
        return this->index(idx->type(), idx->value() + 1, dbg);

    return app(app(index_succ_, index->type(), dbg), index, dbg);
}

const Def* World::insert(const Def* def, const Def* i, const Def* value, Debug dbg) {
    // TODO type check insert node
    return unify<Insert>(2, *this, def->type(), def, i, value, dbg);
}

const Def* World::insert(const Def* def, size_t i, const Def* value, Debug dbg) {
    auto idx = index(def->arity()->as<Arity>()->value(), i);
    return insert(def, idx, value, dbg);
}

const Def* World::intersection(Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    return intersection(type_glb(defs, nullptr), defs, dbg);
}

const Def* World::intersection(const Def* type, Defs ops, Debug dbg) {
    assert(ops.size() > 0); // TODO empty intersection -> empty type/kind
    auto defs = set_flatten<Intersection>(ops);
    auto first = *defs.begin();
    if (defs.size() == 1) {
        assert(first->type() == type);
        return first;
    }
    // implements a least upper bound on qualifiers,
    // could possibly be replaced by something subtyping-generic
    if (is_qualifier(first)) {
        assert(type == qualifier_type());
        return qualifier_glb(range(defs), [&] (const SortedDefSet& defs) {
                return unify<Intersection>(defs.size(), *this, qualifier_type(), defs, dbg);
        });
    }

    // TODO recognize some empty intersections? i.e. same sorted ops, intersection of types non-empty?
    return unify<Intersection>(defs.size(), *this, type, defs, dbg);
}

const Pi* World::pi(const Def* domain, const Def* body, const Def* q, Debug dbg) {
    assertf(!body->is_value(), "body {} : {} of function type cannot be a value", body, body->type());
    auto type = type_lub({domain, body}, q, false);
    return unify<Pi>(2, *this, type, domain, body, dbg);
}

const Def* World::pick(const Def* type, const Def* def, Debug dbg) {
    if (auto def_type = def->type()->isa<Intersection>()) {
        assert(any_of(type, def_type->ops()) && "picked type must be a part of the intersection type");
        return unify<Pick>(1, *this, type, def, dbg);
    }

    assert(type == def->type());
    return def;
}

const Def* World::lambda(const Def* domain, const Def* body, const Def* type_qualifier, Debug dbg) {
    auto p = pi(domain, body->type(), type_qualifier, dbg);

    if (auto app = body->isa<App>()) {
        auto eta_property = [&]() {
            return app->arg()->isa<Var>() && app->arg()->as<Var>()->index() == 0;
        };

        if (!app->callee()->free_vars().test(0) && eta_property())
            return app->callee()->shift_free_vars(-1);
    }

    return unify<Lambda>(1, *this, p, body, dbg);
}

const Def* World::variadic(const Def* arity, const Def* body, Debug dbg) {
    assertf(multi_arity_kind()->assignable(arity), "({} : {}) provided to variadic constructor is not a (multi-) arity",
            arity, arity-> type());
    if (auto sigma = arity->isa<Sigma>()) {
        assertf(!sigma->is_nominal(), "can't have nominal sigma arities");
        return variadic(sigma->ops(), flatten(body, sigma->ops()), dbg);
    }

    if (auto v = arity->isa<Variadic>()) {
        if (auto a_literal = v->arity()->isa<Arity>()) {
            assert(!v->body()->free_vars().test(0));
            auto a = a_literal->value();
            assert(a != 1);
            auto result = flatten(body, DefArray(a, v->body()->shift_free_vars(a-1)));
            for (size_t i = a; i-- != 0;)
                result = variadic(v->body()->shift_free_vars(i-1), result, dbg);
            return result;
        }
    }

    if (auto a_literal = arity->isa<Arity>()) {
        auto a = a_literal->value();
        if (a == 0) {
            if (body->is_kind())
                return unify<Variadic>(2, *this, universe(), this->arity(0), star(body->qualifier()), dbg);
            return unit(body->type()->qualifier());
        }
        if (a == 1) return body->reduce(this->index(1, 0));
        if (body->free_vars().test(0))
            return sigma(DefArray(a, [&](auto i) {
                        return body->reduce(this->index(a, i))->shift_free_vars(i); }), dbg);
    }

    assert(body->type()->is_kind() || body->type()->is_universe());
    return unify<Variadic>(2, *this, body->type()->shift_free_vars(-1), arity, body, dbg);
}

const Def* World::variadic(Defs arity, const Def* body, Debug dbg) {
    if (arity.empty())
        return body;
    return variadic(arity.skip_back(), variadic(arity.back(), body, dbg), dbg);
}

const Def* World::sigma(const Def* q, Defs defs, Debug dbg) {
    auto type = type_lub(defs, q);
    if (defs.size() == 0)
        return unit(type->qualifier());

    if (type == multi_arity_kind()) {
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
        return variadic(arity(defs.size(), QualifierTag::Unlimited, dbg), defs.front()->shift_free_vars(-1), dbg);
    }

    return unify<Sigma>(defs.size(), *this, type, defs, dbg);
}

const Def* World::singleton(const Def* def, Debug dbg) {
    assert(def->sort() != Def::Sort::Universe && "can't create singletons of universes");

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
        auto domain = pi_type->domain();
        auto applied = app(def, var(domain, 0));
        return pi(domain, singleton(applied), pi_type->qualifier(), dbg);
    }

    return unify<Singleton>(1, *this, def, dbg);
}

const Def* World::pack(const Def* arity, const Def* body, Debug dbg) {
    if (auto sigma = arity->isa<Sigma>())
        return pack(sigma->ops(), flatten(body, sigma->ops()), dbg);

    if (auto v = arity->isa<Variadic>()) {
        if (auto a_literal = v->arity()->isa<Arity>()) {
            assert(!v->body()->free_vars().test(0));
            auto a = a_literal->value();
            assert(a != 1);
            auto result = flatten(body, DefArray(a, v->body()->shift_free_vars(a-1)));
            for (size_t i = a; i-- != 0;)
                result = pack(v->body()->shift_free_vars(i-1), result, dbg);
            return result;
        }
    }

    if (auto a_literal = arity->isa<Arity>()) {
        auto a = a_literal->value();
        if (a == 0) {
            if (body->is_type())
                return unify<Pack>(1, *this, unit_kind(body->qualifier()), unit(body->qualifier()), dbg);
            return val_unit(body->type()->qualifier());
        }
        if (a == 1) return body->reduce(this->index(1, 0));
        if (body->free_vars().test(0))
            return tuple(DefArray(a, [&](auto i) { return body->reduce(this->index(a, i)); }), dbg);
    }

    if (auto extract = body->isa<Extract>()) {
        if (auto var = extract->index()->isa<Var>()) {
            if (var->index() == 0 && !extract->scrutinee()->free_vars().test(0))
                return extract->scrutinee()->shift_free_vars(-1);
        }
    }

    assert(body->is_term() || body->is_type());
    return unify<Pack>(1, *this, variadic(arity, body->type()), body, dbg);
}

const Def* World::pack(Defs arity, const Def* body, Debug dbg) {
    if (arity.empty())
        return body;
    return pack(arity.skip_back(), pack(arity.back(), body, dbg), dbg);
}

const Def* World::tuple(Defs defs, Debug dbg) {
    size_t size = defs.size();
    if (size == 0)
        return val_unit();
    if (size == 1)
        return defs.front();
    auto type = sigma(DefArray(defs.size(),
                               [&](auto i) { return defs[i]->type()->shift_free_vars(i); }), dbg);

    auto eta_property = [&]() {
        const Def* same = nullptr;
        for (size_t i = 0; i != size; ++i) {
            if (auto extract = defs[i]->isa<Extract>()) {
                if (same == nullptr) {
                    same = extract->scrutinee();
                    if (same->arity() != arity(size))
                        return (const Def*)nullptr;
                }

                if (same == extract->scrutinee()) {
                    if (auto index = extract->index()->isa<Index>()) {
                        if (index->value() == i)
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
            return pack(arity(size, QualifierTag::Unlimited, dbg), defs.front()->shift_free_vars(1), dbg);
        else if (auto same = eta_property())
            return same;
    }

    return unify<Tuple>(size, *this, type->as<SigmaBase>(), defs, dbg);
}

const Def* World::variant(Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    return variant(type_lub(defs, nullptr), defs, dbg);
}

const Def* World::variant(const Def* type, Defs ops, Debug dbg) {
    assert(ops.size() > 0);
    auto defs = set_flatten<Variant>(ops);
    auto first = *defs.begin();
    if (defs.size() == 1) {
        assert(first->type() == type);
        return first;
    }
    // implements a least upper bound on qualifiers,
    // could possibly be replaced by something subtyping-generic
    if (is_qualifier(first)) {
        assert(type == qualifier_type());
        return qualifier_lub(range(defs), [&] (const SortedDefSet& defs) {
            return unify<Variant>(defs.size(), *this, qualifier_type(), defs, dbg);
        });
    }

    return unify<Variant>(defs.size(), *this, type, defs, dbg);
}

static const Def* build_match_type(World& w, Defs handlers) {
    auto types = DefArray(handlers.size(),
            [&](auto i) { return handlers[i]->type()->template as<Pi>()->body(); });
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

const Axiom* World::val_nat(int64_t val, Location location) {
    auto cur = Def::gid_counter();
    auto result = assume(type_nat(), {val}, {location});
    if (result->gid() >= cur)
        result->debug().set(std::to_string(val));
    return result;
}

//------------------------------------------------------------------------------

}
