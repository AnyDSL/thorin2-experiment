#include <functional>

#include <boost/preprocessor/stringize.hpp>

#include "thorin/world.h"

#include "thorin/reduce.h"
#include "thorin/type.h"

namespace thorin {

//------------------------------------------------------------------------------

/*
 * helpers
 */

const Def* infer_max_type(WorldBase& world, Defs ops, const Def* q, bool use_meet) {
    using Sort = Def::Sort;
    auto max_sort = Sort::Type;
    const Def* inferred = use_meet ? world.linear() : world.unlimited();
    for (auto op : ops) {
        assert(op->sort() >= Sort::Type && "Operands must be at least types.");
        max_sort = std::max(max_sort, op->sort());
        assert(max_sort != Sort::Universe && "Type universes shouldn't be operands.");
        if (max_sort == Sort::Type) {
            if (use_meet)
                inferred = world.intersection({inferred, op->qualifier()}, world.qualifier_kind());
            else
                inferred = world.variant({inferred, op->qualifier()}, world.qualifier_kind());
        } else
            inferred = nullptr;
    }
    if (max_sort == Sort::Type) {
        if (q != nullptr) {
#ifndef NDEBUG
            if (auto qual_axiom = inferred->isa<Axiom>()) {
                auto inferred_q = qual_axiom->box().get_qualifier();
                if (auto q_axiom = q->isa<Axiom>())
                    assert(q_axiom->box().get_qualifier() >= inferred_q &&
                          "Provided qualifier must be as restricted as the meet of the operands qualifiers.");
            }
#endif
            return world.star(q);
        } else
            // no provided qualifier, so we use the inferred one
            return world.star(inferred);
    }
    assert(max_sort == Sort::Kind);
    return world.universe();
}

const Def* single_qualified(Defs defs, const Def* q) {
    assert(defs.size() == 1);
    assert(defs.front()->qualifier() == q);
    return defs.front();
}

const Def* qualifier_glb_or_lub(WorldBase& w, Defs defs, bool use_meet,
                                std::function<const Def*(Defs)> unify_fn) {
    auto const_elem = use_meet ? Qualifier::Unlimited : Qualifier::Linear;
    auto ident_elem = use_meet ? Qualifier::Linear : Qualifier::Unlimited;
    size_t num_defs = defs.size();
    DefArray reduced(num_defs);
    Qualifier accu = Qualifier::Unlimited;
    size_t num_const = 0;
    for (size_t i = 0, e = num_defs; i != e; ++i) {
        if (auto q = w.isa_const_qualifier(defs[i])) {
            auto qual = q->box().get_qualifier();
            accu = use_meet ? meet(accu, qual) : join(accu, qual);
            num_const++;
        } else {
            assert(w.is_qualifier(defs[i]));
            reduced[i - num_const] = defs[i];
        }
    }
    if (num_const == num_defs)
        return w.qualifier(accu);
    if (accu == const_elem) {
        // glb(U, x) = U/lub(L, x) = L
        return w.qualifier(const_elem);
    } else if (accu != ident_elem) {
        // glb(L, x) = x/lub(U, x) = x, so otherwise we need to add accu
        assert(num_const != 0);
        reduced[num_defs - num_const] = w.qualifier(accu);
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
    : root_page_(new Page)
    , cur_page_(root_page_.get())
{
    universe_ = insert<Universe>(0, *this);
    qualifier_kind_ = axiom(universe_, {"ℚ"});
    for (size_t i = 0; i != 4; ++i) {
        auto q = Qualifier(i);
        qualifier_[i] = assume(qualifier_kind(), {q}, {qualifier_cstr(q)});
        star_     [i] = insert<Star >(1, *this, qualifier_[i]);
        unit_     [i] = insert<Sigma>(0, *this, Defs(), star_[i], Debug("Σ()"));
        tuple0_   [i] = insert<Tuple>(0, *this, unit_[i], Defs(), Debug("()"));
    }
    arity_kind_ = axiom(universe(), {"𝔸"});
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
    assert(std::any_of(variants.begin(), variants.end(), [&](auto t){ return t == def->type(); })
           && "type must be a part of the variant type");

    return unify<Any>(1, *this, type->as<Variant>(), def, dbg);
}

const Def* WorldBase::app(const Def* callee, Defs args, Debug dbg) {
    if (args.size() == 1) {
        auto single = args.front();
        if (auto tuple = single->isa<Tuple>())
            return app(callee, tuple->ops(), dbg);

        if (auto sigma_type = single->type()->isa<Sigma>()) {
            auto extracts = DefArray(sigma_type->num_ops(), [&](auto i) { return this->extract(single, i); });
            return app(callee, extracts, dbg);
        }
    }

    // TODO what if args types don't match the domains? error?
    auto type = callee->type()->as<Pi>()->reduce(args);
    auto app = unify<App>(args.size() + 1, *this, type, callee, args, dbg);
    assert(app->callee() == callee);

    return app->try_reduce();
}

const Def* WorldBase::dim(const Def* def, Debug dbg) {
    if (auto tuple = def->isa<Tuple>())
        return arity(tuple->num_ops(), dbg);
    if (auto sigma = def->isa<Sigma>())
        return arity(sigma->num_ops(), dbg);
    if (auto variadic = def->isa<Variadic>())
        return variadic->arity();
    if (def->isa<Var>())
        return unify<Dim>(1, *this, def, dbg);
    return arity(1, dbg);
}

const Def* WorldBase::extract(const Def* def, const Def* i, Debug dbg) {
    if (auto assume = i->isa<Axiom>())
        return extract(def, assume->box().get_u64(), dbg);
    return unify<Extract>(2, *this, i->type(), def, i, dbg);
}

const Def* WorldBase::extract(const Def* def, size_t i, Debug dbg) {
    if (def->isa<Tuple>() || def->isa<Sigma>())
        return def->op(i);

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
                type = type->reduce({extract(def, i - delta)}, skipped_shifts);
            }
        }

        return unify<Extract>(2, *this, type, def, index(i, sigma->num_ops(), dbg), dbg);
    }

    if (auto variadic = def->type()->isa<Variadic>()) {
        auto idx = index(i, /*TODO*/variadic->arity()->as<Axiom>()->box().get_u64(), dbg);
        return unify<Extract>(2, *this, variadic->body(), def, idx, dbg);
    }

    assert(i == 0);
    return def;
}

const Def* WorldBase::index(size_t i, size_t a, Debug dbg) {
    if (i < a)
        return assume(arity(a), {u64(i)}, dbg);
    return error(arity(a));
}

const Def* WorldBase::intersection(Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    return variant(defs, infer_max_type(*this, defs, nullptr, true), dbg);
}

const Def* WorldBase::intersection(Defs defs, const Def* type, Debug dbg) {
    assert(defs.size() > 0);
    if (defs.size() == 1) {
        assert(defs.front()->type() == type);
        return defs.front();
    }
    // implements a least upper bound on qualifiers,
    // could possibly be replaced by something subtyping-generic
    if (is_qualifier(defs.front())) {
        assert(type == qualifier_kind());
        return qualifier_glb_or_lub(*this, defs, true, [&] (Defs defs) {
            return unify<Intersection>(defs.size(), *this, qualifier_kind(), defs, dbg);
        });
    }

    // TODO recognize some empty intersections?
    return unify<Intersection>(defs.size(), *this, type, defs, dbg);
}

const Pi* WorldBase::pi(Defs domains, const Def* body, const Def* q, Debug dbg) {
    if (domains.size() == 1) {
        if (auto sigma = domains.front()->isa<Sigma>())
            return pi(sigma->ops(), body, q, dbg);
    }

    return unify<Pi>(domains.size() + 1, *this, domains, body, q, dbg);
}

const Def* WorldBase::pick(const Def* type, const Def* def, Debug dbg) {
    if (auto def_type = def->type()->isa<Intersection>()) {
        assert(std::any_of(def_type->ops().begin(), def_type->ops().end(), [&](auto t) { return t == type; })
               && "picked type must be a part of the intersection type");

        return unify<Pick>(1, *this, type, def, dbg);
    }

    assert(type == def->type());
    return def;
}

const Lambda* WorldBase::pi_lambda(const Pi* pi, const Def* body, Debug dbg) {
    assert(pi->body() == body->type());
    return unify<Lambda>(1, *this, pi, body, dbg);
}

const Def* WorldBase::variadic(const Def* a, const Def* body, Debug dbg) {
    if (auto arity = a->isa<Axiom>()) {
        if (body->free_vars().test(0)) {
            return sigma(DefArray(arity->box().get_u64(),
                        [&](auto i) { return reduce(body, {this->index(i, arity->box().get_u64())}); }), dbg);
        }
    }

    return unify<Variadic>(2, *this, a, body, dbg);
}

const Def* WorldBase::sigma(Defs defs, const Def* q, Debug dbg) {
    auto inferred_type = infer_max_type(*this, defs, q, false);
    switch (defs.size()) {
        case 0:
            return unit(inferred_type->qualifier());
        case 1:
            return defs.front();
        default:
            // TODO variadic needs qualifiers, but need to pull rebase first
            if (std::equal(defs.begin() + 1, defs.end(), &defs.front()))
                return variadic(arity(defs.size(), dbg), defs.front(), dbg);
    }

    return unify<Sigma>(defs.size(), *this, defs, inferred_type, dbg);
}

const Def* WorldBase::singleton(const Def* def, Debug dbg) {
    assert(def->type() && "can't create singletons of universes");

    if (def->type()->isa<Singleton>())
        return def->type();

    if (!def->is_nominal()) {
        if (def->isa<Variant>()) {
            auto ops = DefArray(def->num_ops(), [&](auto i) { return this->singleton(def->op(i)); });
            return variant(ops, def->type()->type(),dbg);
        }

        if (def->isa<Intersection>()) {
            // S(v : t ∩ u) : *
            // TODO Any normalization of a Singleton Intersection?
        }
    }

    if (auto sig = def->type()->isa<Sigma>()) {
        // See Harper PFPL 43.13b
        auto ops = DefArray(sig->num_ops(), [&](auto i) { return this->singleton(this->extract(def, i)); });
        return sigma(ops, sig->qualifier(), dbg);
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

const Def* WorldBase::tuple(const Def* type, Defs defs, Debug dbg) {
    // will return the single type in case of a single def
    auto expected_type = sigma(types(defs));
    const Def* found_structural_type = type;
    // if defs is just one, we may not have a sigma to unpack
    if (type->is_nominal() && type->isa<Sigma>())
        found_structural_type = sigma(type->ops());
    // TODO error message with more precise information
    assert(found_structural_type == expected_type && "can't give type to tuple");

    if (!type->is_nominal() && defs.size() == 1)
        return defs.front();

    return unify<Tuple>(defs.size(), *this, type->as<SigmaBase>(), defs, dbg);
}

const Def* WorldBase::variant(Defs defs, Debug dbg) {
    assert(defs.size() > 0);
    return variant(defs, infer_max_type(*this, defs, nullptr, false), dbg);
}

const Def* WorldBase::variant(Defs defs, const Def* type, Debug dbg) {
    assert(defs.size() > 0);
    if (defs.size() == 1) {
        assert(defs.front()->type() == type);
        return defs.front();
    }
    // implements a least upper bound on qualifiers,
    // could possibly be replaced by something subtyping-generic
    if (is_qualifier(defs.front())) {
        assert(type == qualifier_kind());
        return qualifier_glb_or_lub(*this, defs, false, [&] (Defs defs) {
            return unify<Variant>(defs.size(), *this, qualifier_kind(), defs, dbg);
        });
    }

    return unify<Variant>(defs.size(), *this, type, defs, dbg);
}

const Def* build_match_type(WorldBase& w, Defs handlers) {
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
    auto Q = qualifier_kind();
    auto U = qualifier(Qualifier::Unlimited);
    auto B = type_bool_ = axiom(star(), {"bool"});
    auto N = type_nat_  = axiom(star(), {"nat" });

    type_i_ = axiom(pi({Q, N, N}, star(var(Q, 2))), {"int" });
    type_r_ = axiom(pi({Q, N, N}, star(var(Q, 2))), {"real"});

    val_bool_[0] = assume(B, {false}, {"⊥"});
    val_bool_[1] = assume(B, {true }, {"⊤"});
    val_nat_0_   = val_nat(0);
    for (size_t j = 0; j != val_nat_.size(); ++j)
        val_nat_[j] = val_nat(1 << int64_t(j));

    type_mem_   = axiom(star(Qualifier::Linear), {"M"});
    type_frame_ = axiom(star(Qualifier::Linear), {"F"});
    type_ptr_   = axiom(pi({star(), N}, star()), {"ptr"});

    /*auto vq0 = var(Q, 0)*/; auto vn0 = var(N, 0);
    /*auto vq1 = var(Q, 1)*/; auto vn1 = var(N, 1);
    auto vq2 = var(Q, 2);     auto vn2 = var(N, 2);
    auto vq3 = var(Q, 3);     auto vn3 = var(N, 3);
    auto vq4 = var(Q, 4);

    // type_i
#define CODE(r, x) \
    T_CAT(type_, T_CAT(x), _) = \
        type_i(Qualifier::u, iflags::T_ELEM(0, x), T_ELEM(1, x));
    T_FOR_EACH_PRODUCT(CODE, (THORIN_I_FLAGS)(THORIN_I_WIDTH))
#undef CODE

    // type_i
#define CODE(r, x) \
    T_CAT(type_, T_CAT(x), _) = \
        type_r(Qualifier::u, rflags::T_ELEM(0, x), T_ELEM(1, x));
    T_FOR_EACH_PRODUCT(CODE, (THORIN_R_FLAGS)(THORIN_R_WIDTH))
#undef CODE

    auto i1 = type_i(vq2, vn1, vn0); auto r1 = type_r(vq2, vn1, vn0);
    auto i2 = type_i(vq3, vn2, vn1); auto r2 = type_r(vq3, vn2, vn1);
    auto i3 = type_i(vq4, vn3, vn2); auto r3 = type_r(vq4, vn3, vn2);
    auto i_type_arithop = pi({Q, N, N}, pi({i1, i2}, i3));
    auto r_type_arithop = pi({Q, N, N}, pi({r1, r2}, r3));

    // arithop axioms
#define CODE(r, ir, x) \
    T_CAT(op_, x, _) = axiom(T_CAT(ir, _type_arithop), {T_STR(x)});
    T_FOR_EACH(CODE, i, THORIN_I_ARITHOP)
    T_FOR_EACH(CODE, r, THORIN_R_ARITHOP)
#undef CODE

    // arithop table
    for (size_t q = 0; q != 4; ++q) {
        auto qq = qualifier(Qualifier(q));
#define CODE(r, ir, x)                                                                   \
        for (size_t f = 0; f != size_t(T_CAT(ir, flags)::Num); ++f) {                    \
            for (size_t w = 0; w != size_t(T_CAT(ir, width)::Num); ++w) {                \
                auto flags = val_nat(f);                                                 \
                auto width = val_nat(T_CAT(index2, ir, width)(w));                       \
                T_CAT(op_, x, s_)[q][f][w] = T_CAT(op_, x)(qq, flags, width)->as<App>(); \
            }                                                                            \
        }
        T_FOR_EACH(CODE, i, THORIN_I_ARITHOP)
        T_FOR_EACH(CODE, r, THORIN_R_ARITHOP)
#undef CODE
    }

    auto b = type_i(vq4, val_nat(int64_t(iflags::uo)), val_nat_1());
    auto i_type_cmp = pi(N, pi({Q, N, N}, pi({i1, i2}, b)));
    auto r_type_cmp = pi(N, pi({Q, N, N}, pi({r1, r2}, b)));
    op_icmp_ = axiom(i_type_cmp, {"icmp"});
    op_rcmp_ = axiom(r_type_cmp, {"rcmp"});

    // all cmp relations
#define CODE(r, ir, x) \
    T_CAT(op_, ir, cmp_, x, _) = app(T_CAT(op_, ir, cmp_), val_nat(int64_t(T_CAT(ir, rel)::x)), {T_STR(T_CAT(ir, cmp_, x))})->as<App>();
    T_FOR_EACH(CODE, i, THORIN_I_REL)
    T_FOR_EACH(CODE, r, THORIN_R_REL)
#undef CODE

    // cmp table
    for (size_t q = 0; q != 4; ++q) {
        auto qq = qualifier(Qualifier(q));
#define CODE(ir)                                                                                               \
        for (size_t r = 0; r != size_t(T_CAT(ir, rel)::Num); ++r) {                                            \
            for (size_t f = 0; f != size_t(T_CAT(ir, flags)::Num); ++f) {                                      \
                for (size_t w = 0; w != size_t(T_CAT(ir, width)::Num); ++w) {                                  \
                    auto rel = val_nat(r);                                                                     \
                    auto flags = val_nat(f);                                                                   \
                    auto width = val_nat(T_CAT(index2, ir, width)(w));                                         \
                    T_CAT(op_, ir, cmps_)[r][q][f][w] = T_CAT(op_, ir, cmp)(rel, qq, flags, width)->as<App>(); \
                }                                                                                              \
            }                                                                                                  \
        }
        CODE(i)
        CODE(r)
#undef CODE
    }

    op_insert_ = axiom(pi(star(),
            pi({var(star(), 0), dim(var(star(), 1)), extract(var(star(), 2), var(dim(var(star(), 2)), 0))},
            var(star(), 3))), {"insert"});

    op_lea_ = axiom(pi({star(), N},
                pi({type_ptr(var(star(), 1), var(N, 0)), dim(var(star(), 2))},
                type_ptr(extract(var(star(), 3), var(dim(var(star(), 3)), 0)), var(N, 2)))), {"lea"});
}

#define CODE(r, ir, x)                                                          \
const App* World::T_CAT(op_, x)(const Def* a, const Def* b) {                   \
    T_CAT(ir, Type) t(a->type());                                               \
    auto callee = t.is_const()                                                  \
        ? T_CAT(op_, x)(t.const_qualifier(), t.const_flags(), t.const_width())  \
        : T_CAT(op_, x)(t.      qualifier(), t.      flags(), t.      width()); \
    return app(callee, {a, b})->as<App>();                                      \
}
T_FOR_EACH(CODE, i, THORIN_I_ARITHOP)
T_FOR_EACH(CODE, r, THORIN_R_ARITHOP)
#undef CODE

const Def* World::op_insert(const Def* def, const Def* index, const Def* val, Debug dbg) {
    return app(app(op_insert_, def->type(), dbg), {def, index, val}, dbg);
}

const Def* World::op_insert(const Def* def, size_t i, const Def* val, Debug dbg) {
    auto idx = index(i, dim(def->type())->as<Axiom>()->box().get_u64());
    return app(app(op_insert_, def->type(), dbg), {def, idx, val}, dbg);
}

const Def* World::op_lea(const Def* ptr, const Def* index, Debug dbg) {
    PtrType ptr_type(ptr->type());
    return app(app(op_lea_, {ptr_type.pointee(), ptr_type.addr_space()}, dbg), {ptr, index}, dbg);
}

const Def* World::op_lea(const Def* ptr, size_t i, Debug dbg) {
    PtrType ptr_type(ptr->type());
    auto idx = index(i, dim(ptr_type.pointee())->as<Axiom>()->box().get_u64());
    idx->dump();
    return app(app(op_lea_, {ptr_type.pointee(), ptr_type.addr_space()}, dbg), {ptr, idx}, dbg);
}

}
