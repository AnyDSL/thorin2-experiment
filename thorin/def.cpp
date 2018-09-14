#include "thorin/world.h"
#include "thorin/transform/reduce.h"
#include "thorin/transform/mangle.h"
#include "thorin/util/log.h"

namespace thorin {

//------------------------------------------------------------------------------

/*
 * helpers
 */

uint64_t DefIndexHash::hash(DefIndex s) {
    return murmur3(uint64_t(s->gid()) << 32_u64 | uint64_t(s.index()));
}

void gid_sort(DefArray* defs) {
    std::sort(defs->begin(), defs->end(), DefLt());
}

DefArray gid_sorted(Defs defs) {
    DefArray result(defs);
    gid_sort(&result);
    return result;
}

void unique_gid_sort(DefArray* defs) {
    gid_sort(defs);
    auto first_non_unique = std::unique(defs->begin(), defs->end());
    defs->shrink(std::distance(defs->begin(), first_non_unique));
}

const Axiom* get_axiom(const Def* def) {
    if (auto axiom = def->isa<Axiom>())
        return axiom;
    if (auto app = def->isa<App>(); app != nullptr && app->has_axiom())
        return app->axiom();
    return nullptr;
}

//------------------------------------------------------------------------------

/*
 * misc
 */

uint32_t Def::gid_counter_ = 1;

Debug Def::debug_history() const {
#ifndef NDEBUG
    auto& w = world();
    if (!isa<Axiom>() && !w.is_external(this))
        return w.track_history() ? Debug(loc(), unique_name()) : debug();
#endif
    return debug();
}

const Def* App::unfold() const {
    if (has_axiom()) return this;
    if (cache()) return cache();

    const Def* callee = this->callee();
    if (auto app = callee->isa<App>())
        callee = app->unfold();

    if (auto lambda = callee->isa_lambda()) {
        assert(lambda->is_nominal());
        auto res = drop(lambda, {arg()})->body();
        assert(cache() == nullptr);
        extra().cache_.set_ptr(res);
        return res;
    }

    return this;
}

const Def* Def::destructing_type() const {
    if (auto app = type()->isa<App>())
        return app->unfold();
    return type();
}

Def::Sort Def::sort() const {
    if (tag()                 == Tag::Universe) return Sort::Universe;
    if (type()->tag()         == Tag::Universe) return Sort::Kind;
    if (type()->type()->tag() == Tag::Universe) return Sort::Type;
    assert(type()->type()->type()->tag() == Tag::Universe);
    return Sort::Term;
}

bool Def::maybe_affine() const {
    if (!is_value())
        return false;
    if (type()->isa<QualifierType>())
        return false;
    const Def* q = qualifier();
    assert(q != nullptr);
    if (auto qu = get_qualifier(q)) {
        return qu >= Qualifier::a;
    }
    return true;
}

Def* Def::set(size_t i, const Def* def) {
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_ptr()[i] = def;
    if (i == num_ops() - 1)
        finalize();
    return this;
}

Def* Def::set(Defs defs) {
    assertf(std::all_of(ops().begin(), ops().end(), [](auto op) { return op == nullptr; }), "all ops must be unset");
    assertf(std::all_of(defs.begin(), defs.end(), [](auto def) { return def != nullptr; }), "all new ops must be non-null");
    std::copy(defs.begin(), defs.end(), ops_ptr());
    finalize();
    return this;
}

void Def::finalize() {
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        assert(op(i) != nullptr);
        checked_emplace(op(i)->uses_, Use{this, i});
        free_vars_    |= op(i)->free_vars() >> shift(i);
        contains_lambda_ |= op(i)->tag() == Tag::Lambda || op(i)->contains_lambda();
        is_dependent_ |= is_dependent_ || op(i)->free_vars().any_end(i);
    }

    if (!(isa<Pi>() || isa<Sigma>() || isa<Variadic>()))
        is_dependent_ = false;

    if (type() != nullptr)
        free_vars_ |= type()->free_vars_;

    assert((!is_nominal() || free_vars().none()) && "nominals must not have free vars");

    if (world().expensive_checks_enabled() && free_vars().none())
        check();
}

void Def::unset(size_t i) {
    assert(ops_ptr()[i] && "must be set");
    unregister_use(i);
    ops_ptr()[i] = nullptr;
}

void Def::unregister_uses() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        unregister_use(i);
}

void Def::unregister_use(size_t i) const {
    auto def = ops_ptr()[i];
    assert(def->uses_.contains(Use(this, i)));
    def->uses_.erase(Use(this, i));
    assert(!def->uses_.contains(Use(this, i)));
}

bool Def::is_value() const {
    switch (sort()) {
        case Sort::Universe:
        case Sort::Kind: return false;
        case Sort::Type: return destructing_type()->has_values();
        case Sort::Term: return destructing_type()->has_values();
    }
    THORIN_UNREACHABLE;
}

void Def::replace(Tracker with) const {
    DLOG("replace: {} -> {}", this, with.def());
    assert(type() == with->type());
    assert(!is_replaced());

    if (this != with) {
        for (auto& use : copy_uses()) {
            auto def = const_cast<Def*>(use.def());
            auto index = use.index();
            def->unset(index);
            def->set(index, with);
        }

        uses_.clear();
        substitute_ = with;
    }
}

std::string Def::unique_name() const { return name().str() + '_' + std::to_string(gid()); }

Lambda* Def::as_lambda() const { if (is_nominal()) return const_cast<Lambda*>(as<Lambda>()); return nullptr; }
Lambda* Def::isa_lambda() const { if (is_nominal()) return const_cast<Lambda*>(isa<Lambda>()); return nullptr; }

//------------------------------------------------------------------------------

/*
 * constructors
 */

static inline const char* kind2str(Def::Tag tag) {
    switch (tag) {
        case Def::Tag::ArityKind:      return "𝔸";
        case Def::Tag::MultiArityKind: return "𝕄";
        case Def::Tag::Star:           return "*";
        default: THORIN_UNREACHABLE;
    }
}

Kind::Kind(World& world, Tag tag, const Def* qualifier)
    : Def(tag, world.universe(), {qualifier}, {kind2str(tag)})
{}

QualifierType::QualifierType(World& world)
    : Def(Tag::QualifierType, world.universe(), 0, {"ℚ"})
{}

Sigma::Sigma(World& world, size_t num_ops, Debug dbg)
    : Sigma(world.universe(), num_ops, dbg)
{}

//------------------------------------------------------------------------------

/*
 * has_values
 */

bool is_type_with_values(const Def* def) {
    return def->is_type() && !def->type()->has_values();
}

bool Arity::has_values() const { return true; }
bool App::has_values() const { return is_type_with_values(this); }
bool Axiom::has_values() const { return is_type_with_values(this); }
bool Intersection::has_values() const {
    return std::all_of(ops().begin(), ops().end(), [&](auto op){ return op->has_values(); });
}
bool Lit::has_values() const { return is_type_with_values(this); }
bool Pi::has_values() const { return true; }
bool QualifierType::has_values() const { return true; }
bool Sigma::has_values() const { return true; }
bool Singleton::has_values() const { return op(0)->is_value(); }
bool Var::has_values() const { return is_type_with_values(this); }
bool Variadic::has_values() const { return true; }
bool Variant::has_values() const {
    return std::any_of(ops().begin(), ops().end(), [&](auto op){ return op->has_values(); });
}

//------------------------------------------------------------------------------

/*
 * qualifier/kind_qualifier
 */

const Def* Def::qualifier() const {
    switch (sort()) {
        case Sort::Term:     return type()->type()->kind_qualifier();
        case Sort::Type:     return type()->kind_qualifier();
        case Sort::Kind:     return kind_qualifier();
        case Sort::Universe: return world().qualifier_u();
    }
    THORIN_UNREACHABLE;
}

const Def* Def::kind_qualifier() const {
    assert(is_kind() || is_universe());
    return world().qualifier_u();
}

const Def* Kind::kind_qualifier() const { return op(0); }

const Def* Intersection::kind_qualifier() const {
    auto& w = world();
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return w.intersection(w.qualifier_type(), qualifiers);
}

const Def* Pi::kind_qualifier() const {
    assert(is_kind());
    // TODO ensure that no Pi kinds with substructural qualifiers can exist -> polymorphic/dependent functions must always be unlimited
    return world().qualifier_u();
}

const Def* QualifierType::kind_qualifier() const { return world().qualifier_u(); }

const Def* Sigma::kind_qualifier() const {
    assert(is_kind());
    auto& w = world();
    auto unlimited = w.qualifier_u();
    if (num_ops() == 0)
        return unlimited;
    auto qualifiers = DefArray(num_ops(), [&](auto i) {
       // XXX qualifiers should/must not be dependent within a sigma, as we couldn't express the qualifier of the sigma itself then
       return this->op(i)->has_values() ? shift_free_vars(this->op(i)->qualifier(), -i) : unlimited; });
    return w.variant(w.qualifier_type(), qualifiers);
}

const Def* Singleton::kind_qualifier() const {
    assert(is_kind() && op(0)->type() != this);
    return op(0)->qualifier();
}

const Def* Variadic::kind_qualifier() const {
    assert(is_kind());
    return body()->has_values() ? shift_free_vars(body()->qualifier(), 1) : world().qualifier_u();
}

const Def* Variant::kind_qualifier() const {
    auto& w = world();
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return w.variant(w.qualifier_type(), qualifiers);
}

bool Def::is_substructural() const {
    auto q = qualifier();
    return q != world().qualifier_u();
}

//------------------------------------------------------------------------------

/*
 * arity
 */

// TODO assumption: every axiom that is not a value has arity 1
// TODO assumption: all callees of non-folded apps that yield a type are (originally) axioms

const Def* Def           ::arity() const { return is_value() ? destructing_type()->arity() : nullptr; }
const Def* Arity         ::arity() const { return world().arity(1); }
const Def* App           ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* Axiom         ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* Bottom        ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
// const Def* Intersection::arity() const { return TODO; }
const Def* Kind          ::arity() const { return world().arity(1); }
const Def* Lit           ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* Param         ::arity() const { return destructing_type()->arity(); }
const Def* Pi            ::arity() const { return world().arity(1); }
const Def* QualifierType ::arity() const { return world().arity(1); }
const Def* Sigma         ::arity() const { return world().arity(num_ops()); }
const Def* Singleton     ::arity() const { return op(0)->arity(); }
const Def* Top           ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* Universe      ::arity() const { THORIN_UNREACHABLE; }
const Def* Unknown       ::arity() const { return world().arity(1); } // TODO is this correct?
const Def* Var           ::arity() const { return is_value() ? destructing_type()->arity() : nullptr; }
const Def* Variant       ::arity() const { return world().variant(DefArray(num_ops(), [&](auto i) { return op(i)->arity(); })); }

std::optional<size_t> Def::has_constant_arity() const {
    if (auto a = arity()->isa<Arity>())
        return a->value();
    else return {};
}
//------------------------------------------------------------------------------

/*
 * shift
 */

size_t Def     ::shift(size_t  ) const { return 0; }
size_t Lambda  ::shift(size_t i) const { assert_unused(i == 0 || i == 1); return is_nominal() ? 0 : 1; }
size_t Pack    ::shift(size_t i) const { assert_unused(i == 0); return 1; }
size_t Pi      ::shift(size_t i) const { return i; }
size_t Sigma   ::shift(size_t i) const { return i; }
size_t Variadic::shift(size_t i) const { return i; }

//------------------------------------------------------------------------------

/*
 * hash
 */

uint64_t Def::vhash() const {
    if (is_nominal())
        return murmur3(gid());

    uint64_t seed = hash_combine(hash_begin(fields()), type()->gid());
    for (auto op : ops())
        seed = hash_combine(seed, op->gid());
    return seed;
}

uint64_t Arity::vhash() const { return hash_combine(Def::vhash(), value()); }
uint64_t Lit  ::vhash() const { return hash_combine(Def::vhash(), box().get_u64()); }
uint64_t Var  ::vhash() const { return hash_combine(Def::vhash(), index()); }

//------------------------------------------------------------------------------

/*
 * equal
 */

bool Def::equal(const Def* other) const {
    if (this->is_nominal() || other->is_nominal())
        return this == other;

    bool result = this->fields() == other->fields() && this->type() == other->type();
    if (result) {
        for (size_t i = 0, e = num_ops(); result && i != e; ++i)
            result &= this->op(i) == other->op(i);
    }

    return result;
}

bool Arity::equal(const Def* other) const { return Def::equal(other) && this->value() == other->as<Arity>()->value(); }
bool Lit  ::equal(const Def* other) const { return Def::equal(other) && this->box().get_u64() == other->as<Lit>()->box().get_u64(); }
bool Var  ::equal(const Def* other) const { return Def::equal(other) && this->index() == other->as<Var>()->index(); }

//------------------------------------------------------------------------------

/*
 * rebuild
 */

// TODO rebuild ts
const Def* App           ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops[1], debug()); }
const Def* Arity         ::rebuild(World& to, const Def* t, Defs    ) const { return to.arity(t->op(0), value(), debug()); }
const Def* Axiom         ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Bottom        ::rebuild(World& to, const Def* t, Defs    ) const { return to.bottom(t); }
const Def* Extract       ::rebuild(World& to, const Def*  , Defs ops) const { return to.extract(ops[0], ops[1], debug()); }
const Def* Insert        ::rebuild(World& to, const Def*  , Defs ops) const { return to.insert(ops[0], ops[1], ops[2], debug()); }
const Def* Intersection  ::rebuild(World& to, const Def* t, Defs ops) const { return to.intersection(t, ops, debug()); }
const Def* Kind          ::rebuild(World& to, const Def*  , Defs ops) const { return to.kind(tag(), ops[0]); }
const Def* Lambda        ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.lambda(t->qualifier(), t->as<Pi>()->domain(), ops[0], ops[1], debug());
}
const Def* Lit           ::rebuild(World& to, const Def* t, Defs    ) const { return to.lit(t, box(), debug()); }
const Def* Match         ::rebuild(World& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* Pack          ::rebuild(World& to, const Def* t, Defs ops) const { return to.pack(t->arity(), ops[0], debug()); }
const Def* Param         ::rebuild(World& to, const Def*  , Defs ops) const { return to.param(ops[0]->as<Lambda>(), debug()); }
const Def* Pi            ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi(ops[0], ops[1], debug()); } // TODO deal with qualifier
const Def* Pick          ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(ops.size() == 1);
    return to.pick(ops.front(), t, debug());
}
const Def* QualifierType ::rebuild(World& to, const Def*  , Defs    ) const { return to.qualifier_type(); }
const Def* Sigma         ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.sigma(t->qualifier(), ops, debug());
}
const Def* Singleton     ::rebuild(World& to, const Def*  , Defs ops) const { return to.singleton(ops[0]); }
const Def* Top           ::rebuild(World& to, const Def* t, Defs    ) const { return to.top(t); }
const Def* Tuple         ::rebuild(World& to, const Def*  , Defs ops) const { return to.tuple(ops, debug()); }
const Def* Universe      ::rebuild(World& to, const Def*  , Defs    ) const { return to.universe(); }
const Def* Unknown       ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Var           ::rebuild(World& to, const Def* t, Defs    ) const { return to.var(t, index(), debug()); }
const Def* Variant       ::rebuild(World& to, const Def* t, Defs ops) const { return to.variant(t, ops, debug()); }
const Def* Variadic      ::rebuild(World& to, const Def*  , Defs ops) const { return to.variadic(ops[0], ops[1], debug()); }

//------------------------------------------------------------------------------

/*
 * vstub
 */

Axiom*   Axiom  ::vstub(World& to, const Def* type, Debug dbg) const { return to.axiom(type, normalizer(), dbg); }
Lambda*  Lambda ::vstub(World& to, const Def* type, Debug dbg) const { assert(is_nominal()); return to.lambda (type->as<Pi>(),  dbg); }
Sigma*   Sigma  ::vstub(World& to, const Def* type, Debug dbg) const { assert(is_nominal()); return to.sigma  (type, num_ops(), dbg); }
Unknown* Unknown::vstub(World& to, const Def*     , Debug dbg) const { return to.unknown(dbg); }
Variant* Variant::vstub(World& to, const Def* type, Debug dbg) const { assert(is_nominal()); return to.variant(type, num_ops(), dbg); }

//------------------------------------------------------------------------------

/*
 * reduce/apply
 */

const Def* Pi::apply(const Def* arg) const {
    assert(domain()->assignable(arg));
    return reduce(codomain(), arg);
}

const Def* Lambda::apply(const Def* arg) const {
    assert(domain()->assignable(arg));
    return world().app(this, arg);
}

//------------------------------------------------------------------------------

/*
 * assignable/subtype_of
 * TODO: introduce subtyping by qualifiers? i.e. x:*u -> x:*a
 */

bool Def::subtype_of(const Def* def) const {
    if (this == def)
        return true;
    auto s = sort();
    if (s == Sort::Term || is_value() || def->is_value() || s != def->sort())
        return false;
    if (auto variant = def->isa<Variant>())
        if (!this->isa<Variant>())
            return variant->contains(this);
    return vsubtype_of(def);
}

bool Kind::vsubtype_of(const Def* def) const {
    return (is_arity_kind(this)       && (is_multi_arity_kind(def) || is_star(def)) && op(0) == def->op(0))
        || (is_multi_arity_kind(this) && (is_star(def) && op(0) == def->op(0)));
}

bool Pi::vsubtype_of(const Def* def) const {
    if (type()->subtype_of(def->type())) {
        if (auto other_pi = def->isa<Pi>()) {
            if (other_pi->domain() == domain() && codomain()->subtype_of(other_pi->codomain()))
                return true;
        }
    }
    return false;
}

bool Sigma::vsubtype_of(const Def* def) const {
    if (type()->subtype_of(def->type())) {
        if (auto other = def->isa<Sigma>()) {
            if (num_ops() == other->num_ops()) {
                for (size_t i = 0, e = num_ops(); i != e; ++i) {
                    if (!op(i)->subtype_of(other->op(i))) {
                        return false;
                    }
                }
                return true;
            }
        } else if (auto other = def->isa<Variadic>()) {
            if (auto size = other->has_constant_arity(); num_ops() == size) {
                for (u64 i = 0; i != size; ++i) {
                    // variadic body must not depend on index
                    auto body = shift_free_vars(other->body(), -1);
                    return std::all_of(ops().begin(), ops().end(), [&] (auto op) { return op->subtype_of(body); });
                }
            }
        }
    }
    return false;
}

// bool Variadic::vsubtype_of(const Def* def) const {
// }

bool Def::assignable(const Def* def) const { return def->destructing_type()->subtype_of(this); }

bool Sigma::assignable(const Def* def) const {
    auto& w = world();
    auto type = def->destructing_type();
    if (type == this)
        return true;
    if (is_nominal() && num_ops() == 1 && type == op(0))
        return true;
    if (!type->type()->subtype_of(this->type()))
        return false;
    if (type->subtype_of(this))
        return true;
    Defs defs = def->ops(); // only correct when def is a tuple
    if (auto pack = def->isa<Pack>()) {
        if (auto arity = pack->has_constant_arity()) {
            if (num_ops() != arity)
                return false;
            defs = DefArray(num_ops(), [&](auto i) { return w.extract(def, i); });
        } else
            return false;
    } else if (!def->isa<Tuple>())
        return false;
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        auto reduced_type = reduce(op(i), defs.get_front(i));
        // TODO allow conversion from nominal -> structural, instead of simple comparison
        if (!reduced_type->assignable(defs[i]))
            return false;
    }
    return true;
}

bool Kind::assignable(const Def* def) const {
    return def->type()->subtype_of(this);
    // TODO
#if 0
    auto type = def->type();
    return this == type || ((type->isa<MultiArityKind>() || type->isa<ArityKind>()) && op(0) == type->op(0));
#endif
}

bool Variadic::assignable(const Def* def) const {
    auto type = def->destructing_type();
    if (type == this)
        return true;
    if (auto pack = def->isa<Pack>()) {
        if (arity() != pack->arity())
            return false;
        return body()->assignable(pack->body());
    }
    // only need this because we don't normalize every variadic of constant arity to a sigma
    if (def->isa<Tuple>()) {
        if (auto size = has_constant_arity()) {
            if (size != def->num_ops())
                return false;
            for (size_t i = 0; i != size; ++i) {
                // body should actually not depend on index, but implemented for completeness
                auto reduced_type = reduce(body(), world().index(*size, i));
                if (!reduced_type->assignable(def->op(i)))
                    return false;
            }
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------

/*
 * misc
 */

Lambda* Lambda::set(const Def* body) { return set(world().lit_false(), body); }
const Param* Lambda::param(Debug dbg) const { return world().param(this, dbg); }
const Def* Lambda::param(u64 i, Debug dbg) const { return world().extract(param(), i, dbg); }

Lambda* Lambda::jump(const Def* callee, const Def* arg, Debug dbg) { return set(world().app(callee, arg, dbg)); }
Lambda* Lambda::jump(const Def* callee, Defs args, Debug dbg ) { return jump(callee, world().tuple(args), dbg); }

Lambda* Lambda::br(const Def* cond, const Def* t, const Def* f, Debug dbg) { return jump(world().cn_br(), {cond, t, f}, dbg); }

bool Variant::contains(const Def* def) const {
    return std::binary_search(ops().begin(), ops().end(), def, [&] (auto a, auto b) { return DefLt()(a, b); });
}

//------------------------------------------------------------------------------

}
