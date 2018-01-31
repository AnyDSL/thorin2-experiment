#include <sstream>
#include <stack>

#include "thorin/world.h"
#include "thorin/transform/reduce.h"

namespace thorin {

//------------------------------------------------------------------------------

/*
 * helpers
 */

DefArray qualifiers(Defs defs) {
    DefArray result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->qualifier();
    return result;
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

DefArray unique_gid_sorted(Defs defs) {
    DefArray result(defs);
    unique_gid_sort(&result);
    return result;
}

static bool check_same_sorted_ops(Def::Sort sort, Defs ops) {
#ifndef NDEBUG
    auto all = std::all_of(ops.begin(), ops.end(), [&](auto op) { return sort == op->sort(); });
    assertf(all, "operands must be of the same sort");
#endif
    return true;
}

//------------------------------------------------------------------------------

/*
 * misc
 */

uint32_t Def::gid_counter_ = 1;

const Def* Def::destructing_type() const {
    if (auto app = type()->isa<App>()) {
        if (auto cache = app->cache_) {
            assert(app->callee()->is_nominal());
            return cache;
        }
        if (auto lambda = app->callee()->isa<Lambda>(); lambda != nullptr && lambda->is_nominal()) {
            auto res = thorin::reduce(lambda->body(), app->arg());
            app->cache_ = res;
            return res;
        }
    }

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
    if (auto qu = q->isa<Qualifier>()) {
        return qu->qualifier_tag() >= QualifierTag::Affine;
    }
    return true;
}

Def* Def::set(size_t i, const Def* def) {
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    if (i == num_ops() - 1)
        finalize();
    return this;
}

void Def::finalize() {
    // TODO move this to Error constructor
    has_error_   |= this->tag() == Tag::Error;

    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        assert(op(i) != nullptr);
        const auto& p = op(i)->uses_.emplace(this, i);
        assert_unused(p.second);
        free_vars_   |= op(i)->free_vars() >> shift(i);
        has_error_   |= op(i)->has_error();
        contains_cn_ |= op(i)->tag() == Tag::Cn || op(i)->contains_cn();
    }

    if (type() != nullptr) {
        free_vars_ |= type()->free_vars_;
        has_error_ |= type()->has_error();
    }

#ifndef NDEBUG
    if (free_vars().none())
        typecheck();
#endif
}

void Def::unset(size_t i) {
    assert(ops_[i] && "must be set");
    unregister_use(i);
    ops_[i] = nullptr;
}

void Def::unregister_uses() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        unregister_use(i);
}

void Def::unregister_use(size_t i) const {
    auto def = ops_[i];
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

std::string Def::unique_name() const { return name() + '_' + std::to_string(gid()); }

Cn* Def::as_cn() const { return const_cast<Cn*>(as<Cn>()); }
Cn* Def::isa_cn() const { return const_cast<Cn*>(isa<Cn>()); }

//------------------------------------------------------------------------------

/*
 * constructors
 */

ArityKind::ArityKind(World& world, const Def* qualifier)
    : Def(Tag::ArityKind, world.universe(), {qualifier}, THORIN_OPS_PTR, {"𝔸"})
{}

Intersection::Intersection(const Def* type, const SortedDefSet& ops, Debug dbg)
    : Def(Tag::Intersection, type, range(ops), THORIN_OPS_PTR, dbg)
{
    assert(check_same_sorted_ops(sort(), this->ops()));
}

MultiArityKind::MultiArityKind(World& world, const Def* qualifier)
    : Def(Tag::MultiArityKind, world.universe(), {qualifier}, THORIN_OPS_PTR, {"𝕄"})
{}

Qualifier::Qualifier(World& world, QualifierTag q)
    : Def(Tag::Qualifier, world.qualifier_type(), 0, THORIN_OPS_PTR, {qualifier2str(q)})
    , qualifier_tag_(q)
{}

QualifierType::QualifierType(World& world)
    : Def(Tag::QualifierType, world.universe(), 0, THORIN_OPS_PTR, {"ℚ"})
{}

Sigma::Sigma(World& world, size_t num_ops, Debug dbg)
    : Sigma(world.universe(), num_ops, dbg)
{}

Star::Star(World& world, const Def* qualifier)
    : Def(Tag::Star, world.universe(), {qualifier}, THORIN_OPS_PTR, {"*"})
{}

Variant::Variant(const Def* type, const SortedDefSet& ops, Debug dbg)
    : Def(Tag::Variant, type, range(ops), THORIN_OPS_PTR, dbg)
{
    // TODO does same sorted ops really hold? ex: matches that return different sorted stuff? allowed?
    assert(check_same_sorted_ops(sort(), this->ops()));
}

//------------------------------------------------------------------------------

/*
 * has_values
 */

bool Arity::has_values() const { return true; }
bool Axiom::has_values() const { return sort() == Sort::Type && !type()->has_values(); }
bool CnType::has_values() const { return true; }
bool Intersection::has_values() const {
    return std::all_of(ops().begin(), ops().end(), [&](auto op){ return op->has_values(); });
}
bool Lit::has_values() const { return sort() == Sort::Type && !type()->has_values(); }
bool Pi::has_values() const { return true; }
bool QualifierType::has_values() const { return true; }
bool Sigma::has_values() const { return true; }
bool Singleton::has_values() const { return op(0)->is_value(); }
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
        case Sort::Universe: return world().unlimited();
    }
    THORIN_UNREACHABLE;
}

const Def* Def::kind_qualifier() const {
    assert(is_kind() || is_universe());
    return world().unlimited();
}

const Def* ArityKind::kind_qualifier() const { return op(0); }

const Def* CnType::kind_qualifier() const {
    // TODO
    return world().unlimited();
}

const Def* MultiArityKind::kind_qualifier() const { return op(0); }

const Def* Intersection::kind_qualifier() const {
    auto& w = world();
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return w.intersection(w.qualifier_type(), qualifiers);
}

const Def* Pi::kind_qualifier() const {
    assert(is_kind());
    // TODO the qualifier of a Pi is upper bounded by the qualifiers of the free variables within it
    return world().unlimited();
}

const Def* QualifierType::kind_qualifier() const { return world().unlimited(); }

const Def* Sigma::kind_qualifier() const {
    assert(is_kind());
    auto& w = world();
    auto unlimited = w.unlimited();
    if (num_ops() == 0)
        return unlimited;
    auto qualifiers = DefArray(num_ops(), [&](auto i) {
       // XXX qualifiers should/must not be dependent within a sigma, as we couldn't express the qualifier of the sigma itself then
       return this->op(i)->has_values() ? this->op(i)->qualifier()->shift_free_vars(-i) : unlimited; });
    return w.variant(w.qualifier_type(), qualifiers);
}

const Def* Singleton::kind_qualifier() const {
    assert(is_kind() && op(0)->type() != this);
    return op(0)->qualifier();
}

const Def* Star::kind_qualifier() const { return op(0); }

const Def* Variadic::kind_qualifier() const {
    assert(is_kind());
    return body()->has_values() ? body()->qualifier()->shift_free_vars(1) : world().unlimited();
}

const Def* Variant::kind_qualifier() const {
    auto& w = world();
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return w.variant(w.qualifier_type(), qualifiers);
}

//------------------------------------------------------------------------------

/*
 * arity
 */
//
// TODO assumption: every axiom that is not a value has arity 1
// TODO assumption: all callees of non-folded apps that yield a type are (originally) axioms

const Def* Def           ::arity() const { return is_value() ? destructing_type()->arity() : nullptr; }
const Def* Arity         ::arity() const { return world().arity(1); }
const Def* ArityKind     ::arity() const { return world().arity(1); }
// const Def* All::arity() const { return TODO; }
// const Def* Any::arity() const { return TODO; }
const Def* App           ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* Axiom         ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* Cn            ::arity() const { return world().arity(1); }
const Def* CnType        ::arity() const { return world().arity(1); }
const Def* Error         ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
// const Def* Intersection::arity() const { return TODO; }
const Def* Lit           ::arity() const { return is_value() ? destructing_type()->arity() : world().arity(1); }
const Def* MultiArityKind::arity() const { return world().arity(1); }
const Def* Param         ::arity() const { return destructing_type()->arity(); }
const Def* Pi            ::arity() const { return world().arity(1); }
const Def* Qualifier     ::arity() const { return world().arity(1); }
const Def* QualifierType ::arity() const { return world().arity(1); }
const Def* Sigma         ::arity() const { return world().arity(num_ops()); }
const Def* Singleton     ::arity() const { return op(0)->arity(); }
const Def* Star          ::arity() const { return world().arity(1); }
const Def* Universe      ::arity() const { THORIN_UNREACHABLE; }
const Def* Var           ::arity() const { return is_value() ? destructing_type()->arity() : nullptr; }
const Def* Variant       ::arity() const { return world().variant(DefArray(num_ops(), [&](auto i) { return op(i)->arity(); })); }

//------------------------------------------------------------------------------

/*
 * shift
 */

size_t Def     ::shift(size_t  ) const { return 0; }
size_t Lambda  ::shift(size_t i) const { assert_unused(i == 0); return 1; }
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

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(fields()), type()->gid());
    for (auto op : ops())
        seed = thorin::hash_combine(seed, op->gid());
    return seed;
}

uint64_t Arity::vhash() const { return thorin::hash_combine(Def::vhash(), value()); }
uint64_t Lit  ::vhash() const { return thorin::hash_combine(Def::vhash(), box_.get_u64()); }
uint64_t Var  ::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }

//------------------------------------------------------------------------------

/*
 * equal
 */

bool Def::equal(const Def* other) const {
    if (is_nominal())
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

const Def* Any           ::rebuild(World& to, const Def* t, Defs ops) const { return to.any(t, ops[0], debug()); }
const Def* App           ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops[1], debug()); }
const Def* Arity         ::rebuild(World& to, const Def* t, Defs    ) const { return to.arity(value(), t->op(0), debug()); }
const Def* ArityKind     ::rebuild(World& to, const Def*  , Defs ops) const { return to.arity_kind(ops[0]); }
const Def* Axiom         ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Cn            ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* CnType        ::rebuild(World& to, const Def*  , Defs ops) const { return to.cn_type(ops[0], debug()); }
const Def* Error         ::rebuild(World& to, const Def* t, Defs    ) const { return to.error(t); }
const Def* Extract       ::rebuild(World& to, const Def*  , Defs ops) const { return to.extract(ops[0], ops[1], debug()); }
const Def* Insert        ::rebuild(World& to, const Def*  , Defs ops) const { return to.insert(ops[0], ops[1], ops[2], debug()); }
const Def* Intersection  ::rebuild(World& to, const Def* t, Defs ops) const { return to.intersection(t, ops, debug()); }
const Def* Lambda        ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.lambda(t->as<Pi>()->domain(), ops.front(), debug());
}
const Def* Lit           ::rebuild(World& to, const Def* t, Defs    ) const { return to.lit(t, box(), debug()); }
const Def* Match         ::rebuild(World& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* MultiArityKind::rebuild(World& to, const Def*  , Defs ops) const { return to.multi_arity_kind(ops[0]); }
const Def* Pack          ::rebuild(World& to, const Def* t, Defs ops) const { return to.pack(t->arity(), ops[0], debug()); }
const Def* Param         ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Pi            ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi(ops[0], ops[1], debug()); }
const Def* Pick          ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(ops.size() == 1);
    return to.pick(ops.front(), t, debug());
}
const Def* Qualifier     ::rebuild(World& to, const Def*  , Defs    ) const { return to.qualifier(qualifier_tag_); }
const Def* QualifierType ::rebuild(World& to, const Def*  , Defs    ) const { return to.qualifier_type(); }
const Def* Sigma         ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.sigma(t->qualifier(), ops, debug());
}
const Def* Singleton     ::rebuild(World& to, const Def*  , Defs ops) const { return to.singleton(ops[0]); }
const Def* Star          ::rebuild(World& to, const Def*  , Defs ops) const { return to.star(ops[0]); }
const Def* Tuple         ::rebuild(World& to, const Def*  , Defs ops) const { return to.tuple(ops, debug()); }
const Def* Universe      ::rebuild(World& to, const Def*  , Defs    ) const { return to.universe(); }
const Def* Var           ::rebuild(World& to, const Def* t, Defs    ) const { return to.var(t, index(), debug()); }
const Def* Variant       ::rebuild(World& to, const Def* t, Defs ops) const { return to.variant(t, ops, debug()); }
const Def* Variadic      ::rebuild(World& to, const Def*  , Defs ops) const { return to.variadic(ops[0], ops[1], debug()); }

//------------------------------------------------------------------------------

/*
 * stub
 */

Axiom*   Axiom  ::stub(World& to, const Def* type, Debug dbg) const { return const_cast<Axiom*>(to.axiom(type, dbg)); }
Cn*      Cn     ::stub(World&   , const Def*     , Debug    ) const { return /*TODO*/ nullptr; }
Lambda*  Lambda ::stub(World& to, const Def* type, Debug dbg) const { assert(is_nominal()); return to.lambda (type->as<Pi>(),  dbg); }
Sigma*   Sigma  ::stub(World& to, const Def* type, Debug dbg) const { assert(is_nominal()); return to.sigma  (type, num_ops(), dbg); }
Variant* Variant::stub(World& to, const Def* type, Debug dbg) const { assert(is_nominal()); return to.variant(type, num_ops(), dbg); }

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

const Def* Def::shift_free_vars(size_t shift) const {
    return thorin::shift_free_vars(this, shift);
}

//------------------------------------------------------------------------------

/*
 * assignable/subtype_of
 * TODO: introduce subtyping by qualifiers? i.e. x:*u -> x:*a
 */

bool ArityKind::vsubtype_of(const Def* def) const {
    return (def->isa<MultiArityKind>() || def->isa<Star>()) && op(0) == def->op(0);
}

bool MultiArityKind::vsubtype_of(const Def* def) const {
    return def->isa<Star>() && op(0) == def->op(0);
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

// TODO is there subtyping on sigmas?
// bool Sigma::vsubtype_of(const Def* def) const {
//     if (type()->subtype_of(def->type())) {
//     }
//     return false;
// }

// bool Variadic::vsubtype_of(const Def* def) const {
// }

bool CnType        ::assignable(const Def* def) const { return def->destructing_type()->subtype_of(this); }
bool MultiArityKind::assignable(const Def* def) const { return def->destructing_type()->subtype_of(this); }
bool Pi            ::assignable(const Def* def) const { return def->destructing_type()->subtype_of(this); }

bool Sigma::assignable(const Def* def) const {
    auto& w = world();
    auto t = def->destructing_type();
    if (t == this)
        return true;
    if (is_nominal() && num_ops() == 1 && t == op(0))
        return true;
    if (!t->type()->subtype_of(type()))
        return false;
    Defs defs = def->ops(); // only correct when def is a tuple
    if (auto pack = def->isa<Pack>()) {
        if (auto arity = pack->arity()->isa<Arity>()) {
            if (num_ops() != arity->value())
                return false;
            defs = DefArray(num_ops(), [&](auto i) { return w.extract(def, i); });
        } else
            return false;
    } else if (!def->isa<Tuple>())
        return false;
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        auto reduced_type = reduce(op(i), defs.get_front(i));
        // TODO allow conversion from nominal -> structural, instead of simple comparison
        if (reduced_type->has_error() || !reduced_type->assignable(defs[i]))
            return false;
    }
    return true;
}

bool Star::assignable(const Def* def) const {
    return def->type()->subtype_of(this);
    auto type = def->type();
    return this == type || ((type->isa<MultiArityKind>() || type->isa<ArityKind>()) && op(0) == type->op(0));
}

bool Variadic::assignable(const Def* def) const {
    auto t = def->destructing_type();
    if (t == this)
        return true;
    if (auto pack = def->isa<Pack>()) {
        if (arity() != pack->arity())
            return false;
        return body()->assignable(pack->body());
    }
    // only need this because we don't normalize every variadic of constant arity to a sigma
    if (def->isa<Tuple>()) {
        if (auto a_lit = arity()->isa<Arity>()) {
            auto size = a_lit->value();
            if (size != def->num_ops())
                return false;
            for (size_t i = 0; i != size; ++i) {
                // body should actually not depend on index, but implemented for completeness
                auto reduced_type = reduce(body(), world().index(size, i));
                if (reduced_type->has_error() || !reduced_type->assignable(def->op(i)))
                    return false;
            }
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------

/*
 * check
 */

typedef std::vector<const Def*> Environment;

void check(const Def* def, Environment& types, EnvDefSet& checked) {
    // we assume any type/operand Def without free variables to be checked already
    if (def->free_vars().any())
        def->typecheck_vars(types, checked);
}

void dependent_check(Defs defs, Environment& types, EnvDefSet& checked, Defs bodies) {
    auto old_size = types.size();
    for (auto def : defs) {
        check(def, types, checked);
        types.push_back(def);
    }
    for (auto def : bodies) {
        check(def, types, checked);
    }
    types.erase(types.begin() + old_size, types.end());
}

bool is_nominal_typechecked(const Def* def, Environment& types, EnvDefSet& checked) {
    if (def->is_nominal())
        return checked.emplace(DefArray(types), def).second;
    return false;
}

void Def::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    if (type()) {
        check(type(), types, checked);
    } else
        assert(is_universe());
    for (auto op : ops())
        check(op, types, checked);
}

void Lambda::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    // do Pi type check inline to reuse built up environment
    check(type()->type(), types, checked);
    dependent_check({domain()}, types, checked, {type()->codomain(), body()});
}

void Pack::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    dependent_check({arity()}, types, checked, {body()});
}

void Pi::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    dependent_check({domain()}, types, checked, {codomain()});
}

void Sigma::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    check(type(), types, checked);
    dependent_check(ops(), types, checked, Defs());
}

void Var::typecheck_vars(Environment& types, EnvDefSet&) const {
    auto reverse_index = types.size() - 1 - index();
    auto shifted_type = type()->shift_free_vars(-index() - 1);
    auto env_type = types[reverse_index];
    assertf(env_type == shifted_type,
            "The shifted type {} of variable {} does not match the type {} declared by the binder.", shifted_type,
            index(), types[reverse_index]);
}

void Variadic::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    check(type(), types, checked);
    dependent_check({arity()}, types, checked, {body()});
}

//------------------------------------------------------------------------------

/*
 * stream
 */

std::ostream& Def::qualifier_stream(std::ostream& os) const {
    if (!has_values() || tag() == Tag::QualifierType)
        return os;
    if (type()->is_kind()) {
        auto q = type()->op(0)->isa<Qualifier>();
        if(q->qualifier_tag() != QualifierTag::u)
            os << q;
    }
    return os;
}

std::ostream& Any::vstream(std::ostream& os) const {
    os << "∨:";
    type()->name_stream(os);
    def()->name_stream(os << "(");
    return os << ")";
}

std::ostream& App::vstream(std::ostream& os) const {
    auto domain = callee()->type()->as<Pi>()->domain();
    if (domain->is_kind()) {
        qualifier_stream(os);
    }
    callee()->name_stream(os);
    if (!arg()->isa<Tuple>() && !arg()->isa<Pack>())
        os << "(";
    arg()->name_stream(os);
    if (!arg()->isa<Tuple>() && !arg()->isa<Pack>())
        os << ")";
    return os;
}

std::ostream& Arity::vstream(std::ostream& os) const {
    return os << name();
}

std::ostream& ArityKind::vstream(std::ostream& os) const {
    os << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return os;
    return os << op(0);
}

std::ostream& Axiom::vstream(std::ostream& os) const {
    return qualifier_stream(os) << name();
}

std::ostream& Cn::vstream(std::ostream& os) const {
    return streamf(os, "{}({})", callee(), arg());
}

std::ostream& CnType::vstream(std::ostream& os) const {
    return streamf(os, "cn {}", domain());
}

std::ostream& Error::vstream(std::ostream& os) const { return os << "<error>"; }

std::ostream& Extract::vstream(std::ostream& os) const {
    return scrutinee()->name_stream(os) << "#" << index();
}

std::ostream& Insert::vstream(std::ostream& os) const {
    return scrutinee()->name_stream(os) << "." << index() << "=" << value();
}

std::ostream& Intersection::vstream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∩ ");
}

std::ostream& Lit::vstream(std::ostream& os) const {
    return qualifier_stream(os) << std::to_string(box().get_u64());
}

std::ostream& Match::vstream(std::ostream& os) const {
    os << "match ";
    destructee()->name_stream(os);
    os << " with ";
    return stream_list(os, handlers(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& MultiArityKind::vstream(std::ostream& os) const {
    os << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return os;
    return os << op(0);
}

std::ostream& Param::vstream(std::ostream& os) const { return os << unique_name(); }

std::ostream& Lambda::vstream(std::ostream& os) const {
    domain()->name_stream(os << "λ");
    return body()->name_stream(os << ".");
}

std::ostream& Pack::vstream(std::ostream& os) const {
    os << "(";
    if (auto var = type()->isa<Variadic>())
        os << var->op(0);
    else {
        assert(type()->isa<Sigma>());
        os << type()->num_ops() << "ₐ";
    }
    return streamf(os, "; {})", body());
}

std::ostream& Pi::vstream(std::ostream& os) const {
    qualifier_stream(os);
    os  << "Π";
    domain()->name_stream(os);
    return codomain()->name_stream(os << ".");
}

std::ostream& Pick::vstream(std::ostream& os) const {
    os << "pick:";
    type()->name_stream(os);
    destructee()->name_stream(os << "(");
    return os << ")";
}

std::ostream& Qualifier::vstream(std::ostream& os) const {
    return os << name();
}

std::ostream& QualifierType::vstream(std::ostream& os) const {
    return os << name();
}

std::ostream& Sigma::vstream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "[", "]");
}

std::ostream& Singleton::vstream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "S(", ")");
}

std::ostream& Star::vstream(std::ostream& os) const {
    os << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return os;
    return os << op(0);
}

std::ostream& Universe::vstream(std::ostream& os) const {
    return os << name();
}

std::ostream& Tuple::vstream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& Var::vstream(std::ostream& os) const {
    os << "<" << index() << ":";
    return type()->name_stream(os) << ">";
}

std::ostream& Variadic::vstream(std::ostream& os) const {
    return streamf(os, "[{}; {}]", op(0), body());
}

std::ostream& Variant::vstream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∪ ");
}

//------------------------------------------------------------------------------

/*
 * misc
 */

bool Sigma::is_dependent() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        if (op(i)->free_vars().any_end(i))
            return true;
    }
    return false;
}

const Param* Cn::param(Debug dbg) const { return world().param(this, dbg); }
const Def* Cn::param(u64 i, Debug dbg) const { return world().extract(param(), i, dbg); }

Cn* Cn::set(const Def* filter, const Def* callee, const Def* arg, Debug dbg) {
    jump_debug_ = dbg;
    return Def::set(0, filter)->Def::set(1, callee)->Def::set(2, arg)->as<Cn>();
}

Cn* Cn::set(const Def* filter, const Def* callee, Defs args, Debug dbg) {
    return set(filter, callee, world().tuple(args), dbg);
}

Cn* Cn::jump(const Def* callee, const Def* arg, Debug dbg) { return set(world().lit_false(), callee, arg, dbg); }
Cn* Cn::jump(const Def* callee, Defs args, Debug dbg ) { return jump(callee, world().tuple(args), dbg); }

Cn* Cn::br(const Def* cond, const Def* t, const Def* f, Debug dbg) { return jump(world().cn_br(), {cond, t, f}, dbg); }

Cns Cn::preds() const {
    std::vector<Cn*> preds;
    std::queue<Use> queue;
    DefSet done;

    auto enqueue = [&] (const Def* def) {
        for (auto use : def->uses()) {
            if (done.find(use) == done.end()) {
                queue.push(use);
                done.insert(use);
            }
        }
    };

    done.insert(this);
    enqueue(this);

    while (!queue.empty()) {
        auto use = pop(queue);
        if (auto cn = use->isa_cn()) {
            preds.push_back(cn);
            continue;
        }

        enqueue(use);
    }

    return preds;
}

Cns Cn::succs() const {
    std::vector<Cn*> succs;
    std::queue<const Def*> queue;
    DefSet done;

    auto enqueue = [&] (const Def* def) {
        if (done.find(def) == done.end()) {
            queue.push(def);
            done.insert(def);
        }
    };

    done.insert(this);
    if (!empty())
        enqueue(callee());
    enqueue(arg());

    while (!queue.empty()) {
        auto def = pop(queue);
        if (auto cn = def->isa_cn()) {
            succs.push_back(cn);
            continue;
        }

        for (auto op : def->ops()) {
            if (op->contains_cn())
                enqueue(op);
        }
    }

    return succs;
}

//------------------------------------------------------------------------------

}
