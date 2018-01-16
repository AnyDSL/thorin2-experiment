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


DefArray qualifiers(Defs defs) {
    DefArray result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->qualifier();
    return result;
}

void gid_sort(DefArray* defs) {
    std::sort(defs->begin(), defs->end(), GIDLt<const Def*>());
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

bool check_same_sorted_ops(Def::Sort sort, Defs ops) {
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

size_t Def::gid_counter_ = 1;

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

bool Def::maybe_affine() const {
    if (type() == world().qualifier_type())
        return false;
    const Def* q = qualifier();
    assert(q != nullptr);
    if (auto qu = isa_const_qualifier(q)) {
        return qu->box().get_qualifier() >= Qualifier::Affine;
    }
    return true;
}

void Def::resize(size_t num_ops) {
    num_ops_ = num_ops;
    if (num_ops_ > ops_capacity_) {
        if (on_heap())
            delete[] ops_;
        else
            on_heap_ = true;
        ops_capacity_ *= size_t(2);
        ops_ = new const Def*[ops_capacity_]();
    }
}

Def* Def::set(size_t i, const Def* def) {
    assert(!is_closed() && is_nominal());
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");

    ops_[i] = def;

    if (i == num_ops() - 1) {
        closed_ = true;
        finalize();
    }
    return this;
}

void Def::finalize() {
    assert(is_closed());

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

std::string Def::unique_name() const { return name() + '_' + std::to_string(gid()); }

//------------------------------------------------------------------------------

/*
 * constructors/destructor
 */

Def::~Def() {
    if (on_heap())
        delete[] ops_;
}

ArityKind::ArityKind(World& world, const Def* qualifier)
    : Def(world, Tag::ArityKind, world.universe(), {qualifier}, ops_ptr<ArityKind>(), {"𝔸"})
{}

Intersection::Intersection(World& world, const Def* type, const SortedDefSet& ops, Debug dbg)
    : Def(world, Tag::Intersection, type, range(ops), ops_ptr<Intersection>(), dbg)
{
    assert(check_same_sorted_ops(sort(), this->ops()));
}

Lambda::Lambda(World& world, const Pi* type, const Def* body, Debug dbg)
    : Def(world, Tag::Lambda, type, {body}, ops_ptr<Lambda>(), dbg)
{}

MultiArityKind::MultiArityKind(World& world, const Def* qualifier)
    : Def(world, Tag::MultiArityKind, world.universe(), {qualifier}, ops_ptr<MultiArityKind>(), {"𝕄"})
{}

Pack::Pack(World& world, const Def* type, const Def* body, Debug dbg)
    : TupleBase(world, Tag::Pack, type, {body}, dbg)
{}

Pi::Pi(World& world, const Def* type, const Def* domain, const Def* body, Debug dbg)
    : Def(world, Tag::Pi, type, {domain, body}, ops_ptr<Pi>(), dbg)
{}

QualifierType::QualifierType(World& world)
    : Def(world, Tag::QualifierType, world.universe(), 0, ops_ptr<Universe>(), {"ℚ"})
{}

Sigma::Sigma(World& world, size_t num_ops, Debug dbg)
    : Sigma(world, world.universe(), num_ops, dbg)
{}

Star::Star(World& world, const Def* qualifier)
    : Def(world, Tag::Star, world.universe(), {qualifier}, ops_ptr<Star>(), {"*"})
{}

Variadic::Variadic(World& world, const Def* type, const Def* arity, const Def* body, Debug dbg)
    : SigmaBase(world, Tag::Variadic, type, {arity, body}, dbg)
{}

Variant::Variant(World& world, const Def* type, const SortedDefSet& ops, Debug dbg)
    : Def(world, Tag::Variant, type, range(ops), ops_ptr<Variant>(), dbg)
{
    // TODO does same sorted ops really hold? ex: matches that return different sorted stuff? allowed?
    assert(check_same_sorted_ops(sort(), this->ops()));
}

//------------------------------------------------------------------------------

/*
 * has_values
 */

bool Axiom::has_values() const {
    return sort() == Sort::Type && !type()->has_values();
}

bool Intersection::has_values() const {
    return std::all_of(ops().begin(), ops().end(), [](auto op){ return op->has_values(); });
}

bool Pi::has_values() const {
    return true;
}

bool QualifierType::has_values() const {
    return true;
}

bool Sigma::has_values() const {
    return true;
}

bool Singleton::has_values() const {
    return op(0)->is_value();
}

bool Variadic::has_values() const {
    return true;
}

bool Variant::has_values() const {
    return std::any_of(ops().begin(), ops().end(), [](auto op){ return op->has_values(); });
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

const Def* ArityKind::kind_qualifier() const {
    return op(0);
}

const Def* MultiArityKind::kind_qualifier() const {
    return op(0);
}

const Def* Intersection::kind_qualifier() const {
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return world().intersection(world().qualifier_type(), qualifiers);
}

const Def* QualifierType::kind_qualifier() const {
    return world().unlimited();
}

const Def* Sigma::kind_qualifier() const {
    assert(is_kind());
    auto unlimited = this->world().unlimited();
    if (num_ops() == 0)
        return unlimited;
    auto qualifiers = DefArray(num_ops(), [&](auto i) {
       // XXX qualifiers should/must not be dependent within a sigma, as we couldn't express the qualifier of the sigma itself then
       return this->op(i)->has_values() ? this->op(i)->qualifier()->shift_free_vars(-i) : unlimited; });
    return world().variant(world().qualifier_type(), qualifiers);
}

const Def* Singleton::kind_qualifier() const {
    assert(is_kind());
    // TODO this might recur endlessly if op(0) has this as type, need to guarantee that doesn't occurr/errors early
    return op(0)->qualifier();
}

const Def* Star::kind_qualifier() const {
    return op(0);
}

const Def* Variadic::kind_qualifier() const {
    assert(is_kind());
    return body()->has_values() ? body()->qualifier()->shift_free_vars(1) : world().unlimited();
}

const Def* Variant::kind_qualifier() const {
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return world().variant(world().qualifier_type(), qualifiers);
}

//------------------------------------------------------------------------------

/*
 * arity
 */

const Def* Def::arity() const {
    if (is_value())
        return type()->arity();
    return nullptr;
}

const Def* ArityKind::arity() const { return world().arity(1); }

// const Def* All::arity() const { return TODO; }

// const Def* Any::arity() const { return TODO; }

const Def* App::arity() const {
    if (is_value())
        return type()->arity();
    return world().arity(1); // TODO assumption: all callees of non-folded apps that yield a type are (originally) axioms
}

const Def* Axiom::arity() const {
    if (is_value())
        return type()->arity();
    return world().arity(1); // TODO assumption: every axiom that is not a value has arity 1
}

// const Def* Error::arity() const { return TODO; }

// const Def* Intersection::arity() const { return TODO; }

const Def* MultiArityKind::arity() const { return world().arity(1); }

const Def* Pi::arity() const { return world().arity(1); }

const Def* QualifierType::arity() const { return world().arity(1); }

const Def* Sigma::arity() const { return world().arity(num_ops()); }

// const Def* Singleton::arity() const {

const Def* Star::arity() const { return world().arity(1); }

const Def* Universe::arity() const { THORIN_UNREACHABLE; }

const Def* Var::arity() const {
    if (is_value())
        return type()->arity();
    return nullptr; // unknown arity
}

const Def* Variant::arity() const {
    DefArray arities(num_ops(), [&](auto i) { return op(i)->arity(); });
    return world().variant(arities);
}


//------------------------------------------------------------------------------

/*
 * shift
 */

size_t Def::shift(size_t) const { return 0; }
size_t Lambda::shift(size_t i) const { assert_unused(i == 0); return 1; }
size_t Pack::shift(size_t i) const { assert_unused(i == 0); return 1; }
size_t Pi::shift(size_t i) const { return i; }
size_t Sigma::shift(size_t i) const { return i; }
size_t Variadic::shift(size_t i) const { return i; }

//------------------------------------------------------------------------------

/*
 * hash
 */

uint64_t Def::vhash() const {
    if (is_nominal() || (is_value() && maybe_affine()))
        return murmur3(gid());

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(fields()), type()->gid());
    for (auto op : ops())
        seed = thorin::hash_combine(seed, op->gid());
    return seed;
}

uint64_t Axiom::vhash() const {
    auto seed = Def::vhash();
    if (is_nominal())
        return seed;

    return thorin::hash_combine(seed, box_.get_u64());
}

uint64_t Var::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }

//------------------------------------------------------------------------------

/*
 * equal
 */

bool Def::equal(const Def* other) const {
    if (is_nominal() || (sort() == Sort::Term && maybe_affine()))
        return this == other;

    bool result = this->fields() == other->fields() && this->type() == other->type();
    if (result) {
        for (size_t i = 0, e = num_ops(); result && i != e; ++i)
            result &= this->op(i) == other->op(i);
    }

    return result;
}

bool Axiom::equal(const Def* other) const {
    if (is_nominal() || (sort() == Sort::Term && maybe_affine()))
        return this == other;

    return this->fields() == other->fields() && this->type() == other->type()
        && this->box_.get_u64() == other->as<Axiom>()->box().get_u64();
}

bool Var::equal(const Def* other) const {
    return Def::equal(other) && this->index() == other->as<Var>()->index();
}

//------------------------------------------------------------------------------

/*
 * rebuild
 */

const Def* Any           ::rebuild(World& to, const Def* t, Defs ops) const { return to.any(t, ops[0], debug()); }
const Def* App           ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops[1], debug()); }
const Def* ArityKind     ::rebuild(World& to, const Def*  , Defs ops) const { return to.arity_kind(ops[0]); }
const Def* Axiom         ::rebuild(World& to, const Def* t, Defs    ) const {
    assert(!is_nominal());
    return to.assume(t, box(), debug());
}
const Def* Error         ::rebuild(World& to, const Def* t, Defs    ) const { return to.error(t); }
const Def* Extract       ::rebuild(World& to, const Def*  , Defs ops) const { return to.extract(ops[0], ops[1], debug()); }
const Def* Insert        ::rebuild(World& to, const Def*  , Defs ops) const { return to.insert(ops[0], ops[1], ops[2], debug()); }
const Def* Intersection  ::rebuild(World& to, const Def* t, Defs ops) const { return to.intersection(t, ops, debug()); }
const Def* Lambda        ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.lambda(t->as<Pi>()->domain(), ops.front(), debug());
}
const Def* Match         ::rebuild(World& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* MultiArityKind::rebuild(World& to, const Def*  , Defs ops) const { return to.multi_arity_kind(ops[0]); }
const Def* Pack          ::rebuild(World& to, const Def*  , Defs ops) const { return to.pack(arity(), ops[0], debug()); }
const Def* Pi            ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi(ops[0], ops[1], debug()); }
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

Axiom* Axiom::stub(World& to, const Def*, Debug) const {
    assert(&world() != &to);
    assert(is_nominal());
    return const_cast<Axiom*>(this);
}
Sigma* Sigma::stub(World& to, const Def* type, Debug dbg) const {
    return to.sigma(type, num_ops(), dbg);
}
Variant* Variant::stub(World& to, const Def* type, Debug dbg) const {
    return to.variant(type, num_ops(), dbg);
}

//------------------------------------------------------------------------------

/*
 * reduce/apply
 */

const Def* Def::reduce(Defs args/*, size_t index*/) const {
    return thorin::reduce(this, args, 0);
}

const Def* Pi::apply(const Def* arg) const {
    assert(domain()->assignable(arg));
    return body()->reduce(arg);
}

const Def* Lambda::apply(const Def* arg) const {
    assert(domain()->assignable(arg));
    return world().app(this, arg);
}

const Def* App::try_reduce() const {
    if (cache_)
        return cache_;

    auto pi_type = callee()->type()->as<Pi>();

    if (auto lambda = callee()->isa<Lambda>()) {
        // TODO could reduce those with only affine return type, but requires always rebuilding the reduced body
        if (!pi_type->maybe_affine() && !pi_type->body()->maybe_affine() &&
            (!lambda->is_nominal() || arg()->free_vars().none())) {
            if  (!lambda->is_closed()) // don't set cache as long lambda is unclosed
                return this;

            return thorin::reduce(lambda->body(), {arg()}, [&] (const Def* def) { cache_ = def; });
        }
    } else if (/*auto axiom =*/ callee()->isa<Axiom>()) {
        // TODO implement constant folding for primops here
    }

    return cache_ = this;
}

const Def* Def::shift_free_vars(size_t shift) const {
    return thorin::shift_free_vars(this, shift);
}

//------------------------------------------------------------------------------

/*
 * assignable
 * TODO: qualifiers?
 */

bool MultiArityKind::assignable(const Def* def) const {
    return this == def->type() || def->type()->isa<ArityKind>();
}

bool Sigma::assignable(const Def* def) const {
    if (def->type() == this)
        return true;
    if (is_nominal() && num_ops() == 1 && def->type() == op(0))
        return true;
    Defs defs = def->ops(); // only correct when def is a tuple
    if (auto pack = def->isa<Pack>()) {
        if (auto arity = pack->arity(); auto assume = arity->isa<Axiom>()) {
            if (num_ops() != assume->box().get_u64())
                return false;
            defs = DefArray(num_ops(), [&](auto i) { return world().extract(def, i); });
        } else
            return false;
    } else if (!def->isa<Tuple>())
        return false;
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        auto reduced_type = op(i)->reduce(defs.get_front(i));
        // TODO allow conversion from nominal -> structural, instead of simple comparison
        if (reduced_type->has_error() || !reduced_type->assignable(defs[i]))
            return false;
    }
    return true;
}

bool Star::assignable(const Def* def) const {
    auto type = def->type();
    return this == type || (kind_qualifier() == type->kind_qualifier()
                            && (type->isa<MultiArityKind>() || type->isa<ArityKind>()));
}

bool Variadic::assignable(const Def* def) const {
    if (def->type() == this)
        return true;
    if (auto pack = def->isa<Pack>()) {
        if (arity() != pack->arity())
            return false;
        return body()->assignable(pack->body());
    }
    // only need this because we don't normalize every variadic of constant arity to a sigma
    if (def->isa<Tuple>()) {
        if (const Axiom* assume = arity()->isa<Axiom>()) {
            auto size = assume->box().get_u64();
            if (size != def->num_ops())
                return false;
            for (size_t i = 0; i != size; ++i) {
                // body should actually not depend on index, but implemented for completeness
                auto reduced_type = body()->reduce(world().index(i, size));
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
    if (def->is_nominal()) {
        return checked.emplace(DefArray(types), def).second;
    }
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
    dependent_check({domain()}, types, checked, {type()->body(), body()});
}

void Pack::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    dependent_check({arity()}, types, checked, {body()});
}

void Pi::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    dependent_check({domain()}, types, checked, {body()});
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

std::ostream& Any::stream(std::ostream& os) const {
    os << "∨:";
    type()->name_stream(os);
    def()->name_stream(os << "(");
    return os << ")";
}

std::ostream& App::stream(std::ostream& os) const {
    auto domain = callee()->type()->as<Pi>()->domain();
    if (domain->is_kind()) {
        qualifier_stream(os);
    }
    callee()->name_stream(os) << "(";
    return arg()->name_stream(os) << ")";
}

std::ostream& ArityKind::stream(std::ostream& os) const {
    return os << name() << op(0);
}

std::ostream& Axiom::stream(std::ostream& os) const { return qualifier_stream(os) << name(); }

std::ostream& Error::stream(std::ostream& os) const { return os << "<error>"; }

std::ostream& Extract::stream(std::ostream& os) const {
    return scrutinee()->name_stream(os) << "#" << index();
}

std::ostream& Insert::stream(std::ostream& os) const {
    return scrutinee()->name_stream(os) << "." << index() << "=" << value();
}

std::ostream& Intersection::stream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∩ ");
}

std::ostream& Match::stream(std::ostream& os) const {
    os << "match ";
    destructee()->name_stream(os);
    os << " with ";
    return stream_list(os, handlers(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& MultiArityKind::stream(std::ostream& os) const {
    return os << name() << op(0);
}

std::ostream& Lambda::stream(std::ostream& os) const {
    qualifier_stream(os) << "λ";
    domain()->name_stream(os);
    return body()->name_stream(os << ".");
}

std::ostream& Pack::stream(std::ostream& os) const {
    return streamf(os, "({}; {})", arity(), body());
}

std::ostream& Pi::stream(std::ostream& os) const {
    qualifier_stream(os) << "Π";
    domain()->name_stream(os);
    return body()->name_stream(os << ".");
}

std::ostream& Pick::stream(std::ostream& os) const {
    os << "pick:";
    type()->name_stream(os);
    destructee()->name_stream(os << "(");
    return os << ")";
}

std::ostream& QualifierType::stream(std::ostream& os) const {
    return os << name();
}

std::ostream& Sigma::stream(std::ostream& os) const {
    if (num_ops() == 0 && is_kind())
        return os << "[]*";
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "[", "]");
}

std::ostream& Singleton::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "S(", ")");
}

std::ostream& Star::stream(std::ostream& os) const {
    return os << name() << op(0);
}

std::ostream& Universe::stream(std::ostream& os) const {
    return qualifier_stream(os) << name();
}

std::ostream& Tuple::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& Var::stream(std::ostream& os) const {
    os << "<" << index() << ":";
    return type()->name_stream(os) << ">";
}

std::ostream& Variadic::stream(std::ostream& os) const {
    return streamf(os, "[{}; {}]", arity(), body());
}

std::ostream& Variant::stream(std::ostream& os) const {
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
