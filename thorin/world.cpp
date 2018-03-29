#include <algorithm>
#include <functional>

#include "thorin/world.h"
#include "thorin/normalize.h"
#include "thorin/fe/parser.h"
#include "thorin/transform/reduce.h"

namespace thorin {

//------------------------------------------------------------------------------

/*
 * helpers
 */

template<class T>
static const SortedDefSet set_flatten(Defs defs) {
    SortedDefSet flat_defs;
    for (auto def : defs) {
        if (def->template isa<Bottom>())
            continue;
        if (def->isa<T>())
            for (auto inner : def->ops())
                flat_defs.insert(inner);
        else
            flat_defs.insert(def);
    }
    return flat_defs;
}

const Def* infer_shape(World& world, const Def* def) {
    if (auto variadic = def->type()->isa<Variadic>()) {
        if (!variadic->body()->isa<Variadic>())
            return variadic->arity();
        std::vector<const Def*> arities;
        const Def* cur = variadic;
        for (; cur->isa<Variadic>(); cur = cur->as<Variadic>()->body())
            arities.emplace_back(cur->as<Variadic>()->arity());
        return world.sigma(arities);
    }
    return world.arity(1);
}

static bool all_of(Defs defs) {
    return std::all_of(defs.begin() + 1, defs.end(), [&](auto def) { return def == defs.front(); });
}

static bool any_of(const Def* def, Defs defs) {
    return std::any_of(defs.begin(), defs.end(), [&](auto d){ return d == def; });
}

static inline bool same_sort(Defs ops) {
    auto sort = ops.front()->sort();
    return std::all_of(ops.begin()+1, ops.end(), [&](auto op) { return sort == op->sort(); });
}

static bool is_qualifier(const Def* def) { return def->type() == def->world().qualifier_type(); }

template<class T, class I>
const Def* World::bound(const Def* q, Range<I> defs, bool require_qualifier) {
    if (defs.distance() == 0)
        return star(q ? q : qualifier(T::Qualifier::min));

    auto first = *defs.begin();
    auto inferred_q = first->qualifier();
    auto max = first;

    auto iter = defs.begin();
    iter++;
    for (size_t i = 1, e = defs.distance(); i != e; ++i, ++iter) {
        auto def = *iter;
        if (def->is_value())
            errorf("can't have value '{}' as operand of bound operator", def);

        if (def->qualifier()->free_vars().any_range(0, i)) {
            // qualifier is dependent within this type/kind, go to top directly
            // TODO might want to assert that this will always be a kind?
            inferred_q = qualifier(T::Qualifier::max);
        } else {
            auto qualifier = shift_free_vars(def->qualifier(), -i);
            inferred_q = join<T>(qualifier_type(), {inferred_q, qualifier}, {});
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
            return star(q ? q : qualifier(T::Qualifier::min));
        if (q == nullptr) {
            // no provided qualifier, so we use the inferred one
            assert(!max || max->op(0) == inferred_q);
            return max;
        } else {
#if 0
            if (expensive_checks_enabled()) {
                if (auto i_qual = inferred_q->template isa<Qualifier>()) {
                    auto iq = i_qual->qualifier_tag();
                    if (auto q_qual = q->template isa<Qualifier>()) {
                        auto qual = q_qual->qualifier_tag();
                        if (l.q_less(iq, qual))
                            errorf("qualifier must be '{}' than the '{}' of the operands' qualifiers", l.short_name, l.full_name);
                    }
                }
            }
#endif
            return star(q);
        }
    }
    return max;
}

//------------------------------------------------------------------------------

#ifndef NDEBUG
bool World::Lock::alloc_guard_ = false;
#endif

World::World(Debug dbg)
    : debug_(dbg)
    , root_page_(new Zone)
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
        unit_[i] = arity(qualifier_[i], 1);
        unit_val_[i] = index_zero(unit_[i]);
    }

    type_bool_ = axiom(star(), {"bool"});
    type_nat_  = axiom(star(), {"nat"});

    lit_bool_[0] = lit(type_bool(), {false});
    lit_bool_[1] = lit(type_bool(), {true});

    lit_nat_0_   = lit_nat(0);
    for (size_t j = 0; j != lit_nat_.size(); ++j)
        lit_nat_[j] = lit_nat(1 << int64_t(j));

    auto type_BOp  = fe::parse(*this, "Πs: 𝕄. Π[[s; bool], [s; bool]]. [s; bool]");
    auto type_NOp  = fe::parse(*this, "Πs: 𝕄. Π[[s;  nat], [s;  nat]]. [s;  nat]");

#define CODE(T, o) \
    T ## _[size_t(T::o)] = axiom(type_ ## T, normalize_ ## T<T::o>, {op2str(T::o)});
    THORIN_B_OP(CODE)
    THORIN_N_OP(CODE)
#undef CODE

    arity_succ_ = axiom("ASucc", "Π[q: ℚ, a: 𝔸q].𝔸q");         // {"Sₐ"}
    index_zero_ = axiom("I0",    "Πp:[q: ℚ, 𝔸q].ASucc p");       // {"0ⁱ"}
    index_succ_ = axiom("IS",    "Πp:[q: ℚ, a: 𝔸q].Πa.ASucc p"); // {"Sⁱ"}

    arity_eliminator_       = axiom("Eₐ",  "Πq: ℚ.ΠP: [Π𝔸q.*q].ΠP(0ₐq).Π[Πa:𝔸q.ΠP a.P(ASucc (q,a))].Πa: 𝔸q.P a", normalize_arity_eliminator);
    arity_eliminator_arity_ = axiom("R𝔸ₐ", "Πq: ℚ.Π𝔸q.Π[Π𝔸q.Π𝔸q.𝔸q].Π𝔸q.𝔸q");
    arity_eliminator_multi_ = axiom("R𝕄ₐ", "Πq: ℚ.Π𝕄q.Π[Π𝔸q.Π𝕄q.𝕄q].Π𝔸q.𝕄q");
    arity_eliminator_star_  = axiom("R*ₐ",  "Πq: ℚ.Π*q.Π[Π𝔸q.Π*q.*q].Π𝔸q.*q");
    // index_eliminator_ = axiom(fe::parse(*this, "Πq: ℚ.ΠP:[Πa:𝔸(q).Πa.*(q)].ΠP(0ₐ(q)).Π[Πa:𝔸(q).ΠP(a).P(ASucc (q,a))].Πa:𝔸(q).P a"));

    cn_br_      = axiom("br",      "cn[bool, cn[], cn[]]");
    cn_end_     = lambda(cn(unit()), {"end"});
}

World::~World() {
    for (auto def : defs_)
        def->~Def();
}

const Arity* World::arity(const Def* q, size_t a, Location location) {
    assert(q->type() == qualifier_type());
    auto cur = Def::gid_counter();
    auto result = unify<Arity>(3, arity_kind(q), a, location);

    if (result->gid() >= cur)
        result->debug().set(std::to_string(a) + "ₐ");

    return result;
}

const Def* World::arity_succ(const Def* a, Debug dbg) {
    if (auto a_lit = a->isa<Arity>()) {
        return arity(a->qualifier(), a_lit->value() + 1, dbg);
    }
    return app(arity_succ_, tuple({a->qualifier(), a}), dbg);
}

const Def* World::app(const Def* callee, const Def* arg, Debug dbg) {
    auto callee_type = callee->type()->as<Pi>();

    if (!expensive_checks_enabled() || callee_type->domain()->assignable(arg)) {
        if (auto axiom = get_axiom(callee); axiom && !callee_type->codomain()->isa<Pi>()) {
            if (auto normalizer = axiom->normalizer()) {
                if (auto result = normalizer(callee, arg, dbg))
                    return result;
            }
        }

        auto type = callee_type->apply(arg);
        auto app = unify<App>(2, type, callee, arg, dbg);
        assert(app->callee() == callee);

        if (callee->is_nominal())
            return app;

        if (auto lambda = app->callee()->isa<Lambda>()) {
            if (auto cache = app->cache())
                return cache;

            // TODO could reduce those with only affine return type, but requires always rebuilding the reduced body?
            if (!lambda->maybe_affine() && !lambda->codomain()->maybe_affine()) {
                assert(app->cache() == nullptr);
                auto res = reduce(lambda->body(), app->arg());
                app->extra().cache_.set_ptr(res);
                return res;
            }
        }
        return app;
    } else {
        errorf("callee '{}' with domain '{}' cannot be called with argument '{}' of type '{}'", callee, callee_type->domain(), arg, arg->type());
    }
}

Axiom* World::axiom(const Def* type, Normalizer normalizer, Debug dbg) {
    auto a = insert<Axiom>(0, type, normalizer, dbg);
    auto s = dbg.name().c_str();
    if (s[0] != '\0') {
        assert(!axioms_.contains(s));
        axioms_[s] = a;
    }
    return a;
}

Axiom* World::axiom(Symbol name, const char* s, Normalizer normalizer) {
    return axiom(fe::parse(*this, s), normalizer, name);
}

const Def* World::extract(const Def* def, const Def* index, Debug dbg) {
    if (index->type() == arity(1)) return def;
    if (def->is_value()) {
        auto type = def->destructing_type();
        auto arity = type->arity();
        if (arity == nullptr)
            errorf("arity unknown for '{}' of type '{}', can only extract when arity is known", def, type);
        if (arity->assignable(index)) {
            if (auto idx = index->isa<Lit>()) {
                auto i = get_index(idx);
                if (def->isa<Tuple>()) {
                    return def->op(i);
                }

                if (auto sigma = type->isa<Sigma>()) {
                    auto type = sigma->op(i);
                    if (!sigma->is_dependent())
                        type = shift_free_vars(type, -i);
                    else {
                        for (ptrdiff_t def_idx = i - 1; def_idx >= 0; --def_idx) {
                            // this also shifts any Var with i > skipped_shifts by -1
                            auto prev_def = shift_free_vars(extract(def, def_idx), def_idx);
                            type = reduce(type, prev_def);
                        }
                    }
                    return unify<Extract>(2, type, def, index, dbg);
                }
            }
            // homogeneous tuples <v,...,v> are normalized to packs, so this also optimizes to v
            if (auto pack = def->isa<Pack>()) {
                return reduce(pack->body(), index);
            }
            // here: index is const => type is variadic, index is var => type may be variadic/sigma, must not be dependent sigma
            assert(!index->isa<Lit>() || type->isa<Variadic>()); // just a sanity check for implementation errors above
            const Def* result_type = nullptr;
            if (auto sigma = type->isa<Sigma>()) {
                if (sigma->is_dependent())
                    errorf("can't extract at {} from '{}' of type '{}', type is dependent", index, def, sigma);
                if (sigma->type() == universe()) {
                    // can only type those, that we can bound usefully
                    auto bnd = bound<Variant>(nullptr, sigma->ops(), true);
                    // universe may be a wrong bound, e.g. for (poly_identity, Nat) : [t:*->t->t, *] : □, but * and t:*->t-> not subtypes, thus can't derive a bound for this
                    // TODO maybe infer variant? { t:*->t->t, * }?
                    if (bnd == universe())
                        errorf("can't extract at '{}' from '{}' of type '{}'  type may be □ (not reflectable)", index, def, sigma);
                    result_type = bnd;
                } else
                    result_type = extract(tuple(sigma->ops(), dbg), index);
            } else
                result_type = reduce(type->as<Variadic>()->body(), index);

            return unify<Extract>(2, result_type, def, index, dbg);
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

        errorf("can't extract at '{}' from '{}' of type '{}' because index type '{}' is not compatible",
                index, index->type(), def, type);
    } else {
        errorf("can only extract from values, but '{}' is not a value", def);
    }
}

const Def* World::extract(const Def* def, size_t i, Debug dbg) {
    if (auto arity = def->arity()->isa<Arity>())
        return extract(def, index(arity, i, dbg), dbg);
    else
        errorf("can only extract with constant on constant arities");
}

const Lit* World::index(const Arity* a, u64 i, Location location) {
    auto arity_val = a->value();
    if (i < arity_val) {
        auto cur = Def::gid_counter();
        auto result = lit(a, i, location);

        if (result->gid() >= cur) { // new literal -> build name
            std::string s = std::to_string(i);
            auto b = s.size();

            // append utf-8 subscripts in reverse order
            for (size_t aa = arity_val; aa > 0; aa /= 10)
                ((s += char(char(0x80) + char(aa % 10))) += char(0x82)) += char(0xe2);

            std::reverse(s.begin() + b, s.end());
            result->debug().set(s);
        }

        return result;
    } else {
        errorf("index literal '{}' does not fit within arity '{}'", i, a);
    }
}

const Def* World::index_zero(const Def* arity, Location location) {
    if (arity->type()->isa<ArityKind>()) {
        if (auto a = arity->isa<Arity>())
            return index(a->value() + 1, 0, location);
        return app(index_zero_, arity, {location});
    } else {
        errorf("expected '{}' to have an 𝔸 type", arity);
    }
}

const Def* World::index_succ(const Def* index, Debug dbg) {
    assert(index->type()->type()->isa<ArityKind>());
    if (auto idx = index->isa<Lit>())
        return this->index(idx->type()->as<Arity>(), get_index(idx) + 1_u64, dbg);

    return app(app(index_succ_, index->type(), dbg), index, dbg);
}

const Def* World::insert(const Def* def, const Def* i, const Def* value, Debug dbg) {
    // TODO type check insert node
    return unify<Insert>(2, def->type(), def, i, value, dbg);
}

const Def* World::insert(const Def* def, size_t i, const Def* value, Debug dbg) {
    auto idx = index(def->arity()->as<Arity>()->value(), i);
    return insert(def, idx, value, dbg);
}

template<class T>
const Def* World::join(const Def* type, Defs ops, Debug dbg) {
    auto defs = set_flatten<T>(ops);
    if (defs.empty()) return bottom(type);
    if (same_sort(ops)) {
        auto first = *defs.begin();
        if (defs.size() == 1) {
            if (first->type() == type) {
                return first;
            } else {
                errorf("provided type '{}' for '{}' must match the type '{}' of the sole operand '{}'",
                        type, T::op_name, first->type(), type);
            }
        }

        // implements a least upper bound on qualifiers,
        // could possibly be replaced by something subtyping-generic
        if (is_qualifier(first)) {
            assert(type == qualifier_type());

            size_t num_defs = std::distance(defs.begin(), defs.end());
            DefArray reduced(num_defs);
            auto accu = T::Qualifier::min;
            size_t num_const = 0;
            auto iter = defs.begin();
            for (size_t i = 0, e = num_defs; i != e; ++i, ++iter) {
                if (auto q = (*iter)->template isa<Qualifier>()) {
                    auto qual = q->qualifier_tag();
                    accu = T::Qualifier::join(accu, qual);
                    num_const++;
                } else {
                    assert(is_qualifier(*iter));
                    reduced[i - num_const] = *iter;
                }
            }
            if (num_const == num_defs)
                return qualifier(accu);
            if (accu == T::Qualifier::max) {
                // glb(U, x) = U/lub(L, x) = L
                return qualifier(T::Qualifier::max);
            } else if (accu != T::Qualifier::min) {
                // glb(L, x) = x/lub(U, x) = x, so otherwise we need to add accu
                assert(num_const != 0);
                reduced[num_defs - num_const] = qualifier(accu);
                num_const--;
            }
            reduced.shrink(num_defs - num_const);
            if (reduced.size() == 1)
                return reduced[0];

            SortedDefSet set(reduced.begin(), reduced.end());
            return unify<T>(set.size(), qualifier_type(), set, dbg);
        }

        return unify<T>(defs.size(), type, defs, dbg);
    } else {
        errorf("all operands must be of the same sort");
    }
}

template const Def* World::join<Intersection>(const Def*, Defs, Debug);
template const Def* World::join<Variant     >(const Def*, Defs, Debug);

const Pi* World::pi(const Def* q, const Def* domain, const Def* codomain, Debug dbg) {
    if (!codomain->is_value()) {
        auto type = type_bound<Variant>(q, {domain, codomain}, false);
        return unify<Pi>(2, type, domain, codomain, dbg);
    } else {
        errorf("codomain '{}' of type '{}' of function type cannot be a value", codomain, codomain->type());
    }
}

const Def* World::pick(const Def* type, const Def* def, Debug dbg) {
    if (auto intersection = def->type()->isa<Intersection>()) {
        assert_unused(any_of(type, intersection->ops()) && "picked type must be a part of the intersection type");
        return unify<Pick>(1, type, def, dbg);
    }

    assert(type == def->type());
    return def;
}

const Def* World::lambda(const Def* type_qualifier, const Def* domain, const Def* filter, const Def* body, Debug dbg) {
    auto p = pi(type_qualifier, domain, body->type(), dbg);

    if (auto app = body->isa<App>()) {
        bool eta_property = app->arg()->isa<Var>() && app->arg()->as<Var>()->index() == 0;

        if (!app->callee()->free_vars().test(0) && eta_property)
            return shift_free_vars(app->callee(), -1);
    }

    return unify<Lambda>(2, p, filter, body, dbg);
}

const Def* World::variadic(const Def* arity, const Def* body, Debug dbg) {
    if (!expensive_checks_enabled() || multi_arity_kind()->assignable(arity)) {
        if (auto s = arity->isa<Sigma>()) {
            if (!s->is_nominal())
                return variadic(s->ops(), flatten(body, s->ops()), dbg);
            else
                errorf("can't have nominal sigma arities");
        } else if (auto v = arity->isa<Variadic>()) {
            if (auto a_literal = v->arity()->isa<Arity>()) {
                assert(!v->body()->free_vars().test(0));
                auto a = a_literal->value();
                assert(a != 1);
                auto result = flatten(body, DefArray(a, shift_free_vars(v->body(), a-1)));
                for (size_t i = a; i-- != 0;)
                    result = variadic(shift_free_vars(v->body(), i-1), result, dbg);
                return result;
            }
        } else if (auto a_literal = arity->isa<Arity>()) {
            auto a = a_literal->value();
            switch (a) {
                case 0:
                    if (body->is_kind())
                        return unify<Variadic>(2, universe(), this->arity(0), star(body->qualifier()), dbg);
                    return unit(body->type()->qualifier());
                case 1:
                    return reduce(body, index(1, 0));
                default:
                    if (body->free_vars().test(0))
                        return sigma(DefArray(a, [&](auto i) { return shift_free_vars(reduce(body, index(a, i)), i); }), dbg);
            }
        }

        assert(body->type()->is_kind() || body->type()->is_universe());
        return unify<Variadic>(2, shift_free_vars(body->type(), -1), arity, body, dbg);
    } else {
        errorf("'{}' of type '{}' provided to variadic constructor is not a (multi-) arity", arity, arity->type());
    }
}

const Def* World::variadic(Defs arity, const Def* body, Debug dbg) {
    if (arity.empty())
        return body;
    return variadic(arity.skip_back(), variadic(arity.back(), body, dbg), dbg);
}

const Def* World::sigma(const Def* q, Defs defs, Debug dbg) {
    auto type = type_bound<Variant>(q, defs);
    if (defs.size() == 0)
        return unit(type->qualifier());

    if (type == multi_arity_kind()) {
        if (any_of(arity(0), defs))
            return arity(0);
    }

    if (defs.size() == 1) {
        if (defs.front()->type() == type)
            return defs.front();
        else
            errorf("type '{}' and inferred type '{}' don't match", defs.front()->type(), type);
    }

    if (defs.front()->free_vars().none_end(defs.size() - 1) && all_of(defs)) {
        assert(q == nullptr || defs.front()->qualifier() == q);
        return variadic(arity(QualifierTag::u, defs.size(), dbg), shift_free_vars(defs.front(), -1), dbg);
    }

    return unify<Sigma>(defs.size(), type, defs, dbg);
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
        return pi(pi_type->qualifier(), domain, singleton(applied), dbg);
    }

    return unify<Singleton>(1, def, dbg);
}

const Def* World::pack(const Def* arity, const Def* body, Debug dbg) {
    if (auto sigma = arity->isa<Sigma>())
        return pack(sigma->ops(), flatten(body, sigma->ops()), dbg);

    if (auto v = arity->isa<Variadic>()) {
        if (auto a_literal = v->arity()->isa<Arity>()) {
            assert(!v->body()->free_vars().test(0));
            auto a = a_literal->value();
            assert(a != 1);
            auto result = flatten(body, DefArray(a, shift_free_vars(v->body(), a-1)));
            for (size_t i = a; i-- != 0;)
                result = pack(shift_free_vars(v->body(), i-1), result, dbg);
            return result;
        }
    }

    if (auto a_literal = arity->isa<Arity>()) {
        auto a = a_literal->value();
        if (a == 0) {
            if (body->is_type())
                return unify<Pack>(1, unit_kind(body->qualifier()), unit(body->qualifier()), dbg);
            return val_unit(body->type()->qualifier());
        }
        if (a == 1) return reduce(body, index(1, 0));
        if (body->free_vars().test(0))
            return tuple(DefArray(a, [&](auto i) { return reduce(body, this->index(a, i)); }), dbg);
    }

    if (auto extract = body->isa<Extract>()) {
        if (auto var = extract->index()->isa<Var>()) {
            if (var->index() == 0 && !extract->scrutinee()->free_vars().test(0))
                return shift_free_vars(extract->scrutinee(), -1);
        }
    }

    assert(body->is_term() || body->is_type());
    return unify<Pack>(1, variadic(arity, body->type()), body, dbg);
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
                               [&](auto i) { return shift_free_vars(defs[i]->type(), i); }), dbg);

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
                    if (auto index = extract->index()->isa<Lit>()) {
                        if (get_index(index) == i)
                            continue;
                    }
                }
            }
            return (const Def*)nullptr;
        }
        return same;
    };

    if (size != 0) {
        if (all_of(defs))
            return pack(arity(QualifierTag::u, size, dbg), shift_free_vars(defs.front(), 1), dbg);
        else if (auto same = eta_property())
            return same;
    }

    return unify<Tuple>(size, type->as<SigmaBase>(), defs, dbg);
}

const Def* World::match(const Def* def, Defs handlers, Debug dbg) {
    auto type = def->destructing_type();

    if (handlers.size() == 1) {
        if (!type->isa<Variant>())
            return app(handlers.front(), def, dbg);
        else
            errorf("matching with one handler implies matching a non-variant-typed value");
    }

    if (auto matched_type = type->as<Variant>()) {
        if (type->num_ops() == handlers.size()) {
            DefArray sorted_handlers(handlers);
            std::sort(sorted_handlers.begin(), sorted_handlers.end(), [](const Def* a, const Def* b) {
                    auto a_dom = a->type()->as<Pi>()->domain();
                    auto b_dom = b->type()->as<Pi>()->domain();
                    return a_dom->gid() < b_dom->gid(); });

            if (expensive_checks_enabled()) {
                for (size_t i = 0; i < sorted_handlers.size(); ++i) {
                    auto domain = sorted_handlers[i]->type()->as<Pi>()->domain();
                    if (domain != matched_type->op(i))
                        errorf("handler '{}' with domain '{}' does not match type '{}'", i, domain, matched_type->op(i));
                }
            }
            auto types = DefArray(sorted_handlers.size(), [&](size_t i) { return handlers[i]->type()->as<Pi>()->codomain(); });
            unique_gid_sort(&types); // we're not actually building a sum type here, we need uniqueness
            return unify<Match>(1+sorted_handlers.size(), variant(types), def, sorted_handlers, dbg);
        } else {
            errorf("number of handlers does not match number of cases");
        }
    } else {
        errorf("matched type must be a variant");
    }
}

const Lit* World::lit_nat(int64_t val, Location location) {
    auto cur = Def::gid_counter();
    auto result = lit(type_nat(), {val}, {location});
    if (result->gid() >= cur)
        result->debug().set(std::to_string(val));
    return result;
}

#ifndef NDEBUG

void World::breakpoint(size_t number) { breakpoints_.insert(number); }
const World::Breakpoints& World::breakpoints() const { return breakpoints_; }
void World::swap_breakpoints(World& other) { swap(this->breakpoints_, other.breakpoints_); }
bool World::track_history() const { return track_history_; }
void World::enable_history(bool flag) { track_history_ = flag; }

#endif

//------------------------------------------------------------------------------

}
