#include <sstream>
#include <stack>

#include "thorin/def.h"
#include "thorin/reduce.h"
#include "thorin/world.h"

namespace thorin {

//------------------------------------------------------------------------------

/*
 * helpers
 */


DefArray qualifiers(World& world, Defs defs) {
    DefArray result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->qualifier(world);
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

Def::Sort Def::sort() const {
    if (auto t = type()) {
        if (auto tt = t->type()) {
            if (auto ttt = tt->type()) {
                assert(!ttt->type());
                return Sort::Term;
            }
            return Sort::Type;
        }
        return Sort::Kind;
    }
    assert(isa<Universe>());
    return Sort::Universe;
}

bool Def::maybe_affine(World& world) const {
    if (!is_value())
        return false;
    if (type()->isa<QualifierType>())
        return false;
    const Def* q = qualifier(world);
    assert(q != nullptr);
    if (auto qu = q->isa<Qualifier>()) {
        return qu->qualifier_tag() >= QualifierTag::Affine;
    }
    return true;
}

Def* Def::set(World& world, size_t i, const Def* def) {
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    if (i == num_ops() - 1)
        finalize(world);
    return this;
}

void Def::finalize(World& world) {
    has_error_ |= this->tag() == Tag::Error;

    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        assert(op(i) != nullptr);
        const auto& p = op(i)->uses_.emplace(this, i);
        assert_unused(p.second);
        free_vars_ |= op(i)->free_vars() >> shift(i);
        has_error_ |= op(i)->has_error();
    }

    if (type() != nullptr) {
        free_vars_ |= type()->free_vars_;
        has_error_ |= type()->has_error();
    }

#ifndef NDEBUG
    if (free_vars().none())
        typecheck(world);
    else
        assertf(!is_nominal(), "nominal expression {} may not have free variables", this);
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

std::string Def::unique_name() const { return name() + '_' + std::to_string(gid()); }

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
    return std::all_of(ops().begin(), ops().end(), [](auto op){ return op->has_values(); });
}
bool Lit::has_values() const { return sort() == Sort::Type && !type()->has_values(); }
bool Pi::has_values() const { return true; }
bool QualifierType::has_values() const { return true; }
bool Sigma::has_values() const { return true; }
bool Singleton::has_values() const { return op(0)->is_value(); }
bool Variadic::has_values() const { return true; }
bool Variant::has_values() const {
    return std::any_of(ops().begin(), ops().end(), [](auto op){ return op->has_values(); });
}

//------------------------------------------------------------------------------

/*
 * qualifier/kind_qualifier
 */

const Def* Def::qualifier(World& world) const {
    switch (sort()) {
        case Sort::Term:     return type()->type()->kind_qualifier(world);
        case Sort::Type:     return type()->kind_qualifier(world);
        case Sort::Kind:     return kind_qualifier(world);
        case Sort::Universe: return world.unlimited();
    }
    THORIN_UNREACHABLE;
}

const Def* Def::kind_qualifier(World& world) const {
    assert(is_kind() || is_universe());
    return world.unlimited();
}

const Def* ArityKind::kind_qualifier(World&) const { return op(0); }

const Def* CnType::kind_qualifier(World& world) const {
    // TODO
    return world.unlimited();
}

const Def* MultiArityKind::kind_qualifier(World&) const { return op(0); }

const Def* Intersection::kind_qualifier(World& world) const {
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(world); });
    return world.intersection(world.qualifier_type(), qualifiers);
}

const Def* Pi::kind_qualifier(World& world) const {
    assert(is_kind());
    // TODO the qualifier of a Pi is upper bounded by the qualifiers of the free variables within it
    return world.unlimited();
}

const Def* QualifierType::kind_qualifier(World& world) const { return world.unlimited(); }

const Def* Sigma::kind_qualifier(World& world) const {
    assert(is_kind());
    auto unlimited = world.unlimited();
    if (num_ops() == 0)
        return unlimited;
    auto qualifiers = DefArray(num_ops(), [&](auto i) {
       // XXX qualifiers should/must not be dependent within a sigma, as we couldn't express the qualifier of the sigma itself then
       return this->op(i)->has_values() ? this->op(i)->qualifier(world)->shift_free_vars(world, -i) : unlimited; });
    return world.variant(world.qualifier_type(), qualifiers);
}

const Def* Singleton::kind_qualifier(World& world) const {
    assert(is_kind() && op(0)->type() != this);
    return op(0)->qualifier(world);
}

const Def* Star::kind_qualifier(World&) const { return op(0); }

const Def* Variadic::kind_qualifier(World& world) const {
    assert(is_kind());
    return body()->has_values() ? body()->qualifier(world)->shift_free_vars(world, 1) : world.unlimited();
}

const Def* Variant::kind_qualifier(World& world) const {
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(world); });
    return world.variant(world.qualifier_type(), qualifiers);
}

//------------------------------------------------------------------------------

/*
 * arity
 */
//
// TODO assumption: every axiom that is not a value has arity 1
// TODO assumption: all callees of non-folded apps that yield a type are (originally) axioms

const Def* Def           ::arity(World& w) const { return is_value() ? type()->arity(w) : nullptr; }
const Def* Arity         ::arity(World& w) const { return w.arity(1); }
const Def* ArityKind     ::arity(World& w) const { return w.arity(1); }
// const Def* All::arity(World& w) const { return TODO; }
// const Def* Any::arity(World& w) const { return TODO; }
const Def* App           ::arity(World& w) const { return is_value() ? type()->arity(w) : w.arity(1); }
const Def* Axiom         ::arity(World& w) const { return is_value() ? type()->arity(w) : w.arity(1); }
const Def* Cn            ::arity(World& w) const { return w.arity(1); }
const Def* CnType        ::arity(World& w) const { return w.arity(1); }
const Def* Error         ::arity(World& w) const { return is_value() ? type()->arity(w) : w.arity(1); }
// const Def* Intersection::arity(World& world) const { return TODO; }
const Def* Lit           ::arity(World& w) const { return is_value() ? type()->arity(w) : w.arity(1); }
const Def* MultiArityKind::arity(World& w) const { return w.arity(1); }
const Def* Param         ::arity(World& w) const { return type()->arity(w); }
const Def* Pi            ::arity(World& w) const { return w.arity(1); }
const Def* Qualifier     ::arity(World& w) const { return w.arity(1); }
const Def* QualifierType ::arity(World& w) const { return w.arity(1); }
const Def* Sigma         ::arity(World& w) const { return w.arity(num_ops()); }
const Def* Singleton     ::arity(World& w) const { return op(0)->arity(w); }
const Def* Star          ::arity(World& w) const { return w.arity(1); }
const Def* Universe      ::arity(World&  ) const { THORIN_UNREACHABLE; }
const Def* Var           ::arity(World& w) const { return is_value() ? type()->arity(w) : nullptr; }
const Def* Variant       ::arity(World& w) const { return w.variant(DefArray(num_ops(), [&](auto i) { return op(i)->arity(w); })); }

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
const Def* Pack          ::rebuild(World& to, const Def*  , Defs ops) const { return to.pack(arity(to), ops[0], debug()); }
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
    return to.sigma(t->qualifier(to), ops, debug());
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
Sigma*   Sigma  ::stub(World& to, const Def* type, Debug dbg) const { return to.sigma(type, num_ops(), dbg); }
Variant* Variant::stub(World& to, const Def* type, Debug dbg) const { return to.variant(type, num_ops(), dbg); }

//------------------------------------------------------------------------------

/*
 * reduce/apply
 */

const Def* Def::reduce(World& world, Defs args/*, size_t index*/) const {
    return thorin::reduce(world, this, args, 0);
}

const Def* Pi::apply(World& world, const Def* arg) const {
    assert(domain()->assignable(world, arg));
    return body()->reduce(world, arg);
}

const Def* Lambda::apply(World& world, const Def* arg) const {
    assert(domain()->assignable(world, arg));
    return world.app(this, arg);
}

const Def* Def::shift_free_vars(World& world, size_t shift) const {
    return thorin::shift_free_vars(world, this, shift);
}

//------------------------------------------------------------------------------

/*
 * assignable/subtype_of
 * TODO: introduce subtyping by qualifiers? i.e. x:*u -> x:*a
 */

bool ArityKind::vsubtype_of(World&, const Def* def) const {
    return (def->isa<MultiArityKind>() || def->isa<Star>()) && op(0) == def->op(0);
}

bool MultiArityKind::vsubtype_of(World&, const Def* def) const {
    return def->isa<Star>() && op(0) == def->op(0);
}

bool Pi::vsubtype_of(World& world, const Def* def) const {
    if (type()->subtype_of(world, def->type())) {
        if (auto other_pi = def->isa<Pi>()) {
            if (other_pi->domain() == domain() && body()->subtype_of(world, other_pi->body()))
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

// bool Variadic::vsubtype_of(World& world, const Def* def) const {
// }

bool CnType        ::assignable(World& world, const Def* def) const { return def->type()->subtype_of(world, this); }
bool MultiArityKind::assignable(World& world, const Def* def) const { return def->type()->subtype_of(world, this); }
bool Pi            ::assignable(World& world, const Def* def) const { return def->type()->subtype_of(world, this); }

bool Sigma::assignable(World& world, const Def* def) const {
    if (def->type() == this)
        return true;
    if (is_nominal() && num_ops() == 1 && def->type() == op(0))
        return true;
    if (!def->type()->type()->subtype_of(world, type()))
        return false;
    Defs defs = def->ops(); // only correct when def is a tuple
    if (auto pack = def->isa<Pack>()) {
        if (auto arity = pack->arity(world)->isa<Arity>()) {
            if (num_ops() != arity->value())
                return false;
            defs = DefArray(num_ops(), [&](auto i) { return world.extract(def, i); });
        } else
            return false;
    } else if (!def->isa<Tuple>())
        return false;
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        auto reduced_type = op(i)->reduce(world, defs.get_front(i));
        // TODO allow conversion from nominal -> structural, instead of simple comparison
        if (reduced_type->has_error() || !reduced_type->assignable(world, defs[i]))
            return false;
    }
    return true;
}

bool Star::assignable(World& world, const Def* def) const {
    return def->type()->subtype_of(world, this);
    auto type = def->type();
    return this == type || ((type->isa<MultiArityKind>() || type->isa<ArityKind>()) && op(0) == type->op(0));
}

bool Variadic::assignable(World& world, const Def* def) const {
    if (def->type() == this)
        return true;
    if (auto pack = def->isa<Pack>()) {
        if (arity(world) != pack->arity(world))
            return false;
        return body()->assignable(world, pack->body());
    }
    // only need this because we don't normalize every variadic of constant arity to a sigma
    if (def->isa<Tuple>()) {
        if (auto a_lit = arity(world)->isa<Arity>()) {
            auto size = a_lit->value();
            if (size != def->num_ops())
                return false;
            for (size_t i = 0; i != size; ++i) {
                // body should actually not depend on index, but implemented for completeness
                auto reduced_type = body()->reduce(world, world.index(size, i));
                if (reduced_type->has_error() || !reduced_type->assignable(world, def->op(i)))
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

void check(World& world, const Def* def, Environment& types, EnvDefSet& checked) {
    // we assume any type/operand Def without free variables to be checked already
    if (def->free_vars().any())
        def->typecheck_vars(world, types, checked);
}

void dependent_check(World& world, Defs defs, Environment& types, EnvDefSet& checked, Defs bodies) {
    auto old_size = types.size();
    for (auto def : defs) {
        check(world, def, types, checked);
        types.push_back(def);
    }
    for (auto def : bodies) {
        check(world, def, types, checked);
    }
    types.erase(types.begin() + old_size, types.end());
}

bool is_nominal_typechecked(const Def* def, Environment& types, EnvDefSet& checked) {
    if (def->is_nominal()) {
        return checked.emplace(DefArray(types), def).second;
    }
    return false;
}

void Def::typecheck_vars(World& world, Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    if (type()) {
        check(world, type(), types, checked);
    } else
        assert(is_universe());
    for (auto op : ops())
        check(world, op, types, checked);
}

void Lambda::typecheck_vars(World& world, Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    // do Pi type check inline to reuse built up environment
    check(world, type()->type(), types, checked);
    dependent_check(world, {domain()}, types, checked, {type()->body(), body()});
}

void Pack::typecheck_vars(World& world, Environment& types, EnvDefSet& checked) const {
    check(world, type(), types, checked);
    dependent_check(world, {arity(world)}, types, checked, {body()});
}

void Pi::typecheck_vars(World& world, Environment& types, EnvDefSet& checked) const {
    check(world, type(), types, checked);
    dependent_check(world, {domain()}, types, checked, {body()});
}

void Sigma::typecheck_vars(World& world, Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    check(world, type(), types, checked);
    dependent_check(world, ops(), types, checked, Defs());
}

void Var::typecheck_vars(World& world, Environment& types, EnvDefSet&) const {
    auto reverse_index = types.size() - 1 - index();
    auto shifted_type = type()->shift_free_vars(world, -index() - 1);
    auto env_type = types[reverse_index];
    assertf(env_type == shifted_type,
            "The shifted type {} of variable {} does not match the type {} declared by the binder.", shifted_type,
            index(), types[reverse_index]);
}

void Variadic::typecheck_vars(World& world, Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    check(world, type(), types, checked);
    dependent_check(world, {arity(world)}, types, checked, {body()});
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
    return body()->name_stream(os << ".");
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

//------------------------------------------------------------------------------

}
