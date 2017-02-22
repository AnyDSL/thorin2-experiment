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

bool is_array(const Def* def) { return def->isa<Variadic>() && !def->free_vars().test(0); }

DefArray types(Defs defs) {
    DefArray result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->type();
    return result;
}

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

template<class T>
const SortedDefSet set_flatten(Defs defs) {
    SortedDefSet flat_defs;
    for (auto def : defs) {
        if (def->isa<T>())
            for (auto inner : def->ops())
                flat_defs.insert(inner);
        else
            flat_defs.insert(def);
    }
    return flat_defs;
}

bool check_same_sorted_ops(Def::Sort sort, Defs ops) {
    for (auto op : ops)
        assert(sort == op->sort() && "operands must be of the same sort");

    assertf(sort == Def::Sort::Type || sort == Def::Sort::Kind, "only sort type or kind allowed");
    return true;
}

//------------------------------------------------------------------------------

/*
 * misc
 */

size_t Def::gid_counter_ = 1;

Def::Sort Def::sort() const {
    if (!type()) {
        assert(isa<Universe>());
        return Sort::Universe;
    } else if (!type()->type())
        return Sort::Kind;
    else if (!type()->type()->type())
        return Sort::Type;
    else {
        assert(!type()->type()->type()->type());
        return Sort::Term;
    }
}

const Def* Def::qualifier() const {
    switch(sort()) {
        case Sort::Term:     return type()->type()->kind_qualifier();
        case Sort::Type:     return type()->kind_qualifier();
        case Sort::Kind:     return kind_qualifier();
        case Sort::Universe: return world().unlimited();
        default:             THORIN_UNREACHABLE;
    }
}

bool Def::maybe_affine() const {
    const Def* q = qualifier();
    assert(q != nullptr);
    if (auto qu = world().isa_const_qualifier(q)) {
        return qu->box().get_qualifier() >= Qualifier::Affine;
    }
    return true;
}

void Def::resize(size_t num_ops) {
    num_ops_ = num_ops;
    if (num_ops_ > ops_capacity_) {
        if (on_heap())
            delete[] ops_;
        ops_capacity_ *= size_t(2);
        ops_ = new const Def*[ops_capacity_]();
    }
}

void Def::set(size_t i, const Def* def) {
    assert(!is_closed() && is_nominal());
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");

    ops_[i] = def;

    if (i == num_ops() - 1) {
        assert(std::all_of(ops().begin(), ops().end(), [](const Def* def) { return def != nullptr; }));
        closed_ = true;
        finalize();
    }
}

void Def::finalize() {
    assert(is_closed());

    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        const auto& p = op(i)->uses_.emplace(this, i);
        assert_unused(p.second);
        free_vars_ |= op(i)->free_vars() >> shift(i);
        has_error_ |= op(i)->has_error();
    }

    if (type() != nullptr)
        free_vars_ |= type()->free_vars_;
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

const Def* Pi::domain() const { return world().sigma(domains()); }

//------------------------------------------------------------------------------

/*
 * constructors/destructor
 */

Def::~Def() {
    if (on_heap())
        delete[] ops_;
}

Dim::Dim(WorldBase& world, const Def* def, Debug dbg)
    : Def(world, Tag::Dim, world.arity_kind(), {def}, dbg)
{}

Intersection::Intersection(WorldBase& world, const Def* type, Defs ops, Debug dbg)
    : Def(world, Tag::Intersection, type, range(set_flatten<Intersection>(ops)), dbg)
{
    assert(check_same_sorted_ops(sort(), ops));
}

Lambda::Lambda(WorldBase& world, const Pi* type, const Def* body, Debug dbg)
    : Def(world, Tag::Lambda, type, {body}, dbg)
{}

Pi::Pi(WorldBase& world, Defs domains, const Def* body, const Def* q, Debug dbg)
    : Def(world, Tag::Pi, body->type()->is_universe() ? (const Def*) world.universe() : world.star(q),
          concat(domains, body), dbg)
{}

Sigma::Sigma(WorldBase& world, size_t num_ops, Debug dbg)
    : Sigma(world, world.universe(), num_ops, dbg)
{}

Star::Star(WorldBase& world, const Def* qualifier)
    : Def(world, Tag::Star, world.universe(), {qualifier}, {"*"})
{}

Variadic::Variadic(WorldBase& world, const Def* arity, const Def* body, Debug dbg)
    : SigmaBase(world, Tag::Variadic, body->type(), {arity, body}, dbg)
{}

Variant::Variant(WorldBase& world, const Def* type, Defs ops, Debug dbg)
    : Def(world, Tag::Variant, type, range(set_flatten<Variant>(ops)), dbg)
{
    // TODO does same sorted ops really hold? ex: matches that return different sorted stuff? allowed?
    assert(check_same_sorted_ops(sort(), ops));
}

//------------------------------------------------------------------------------

/*
 * kind_qualifier
 */

const Def* Def::kind_qualifier() const {
    return world().unlimited();
}

const Def* Intersection::kind_qualifier() const {
    return is_kind() ? world().intersection(qualifiers(ops()), world().qualifier_kind()) :
        world().unlimited();
}

// const Def* Sigma::kind_qualifier() const {
//     // TODO can Sigma kinds have types that are inhabited on the term level and provide a qualifier for them?
//     return world().unlimited();
// }

const Def* Singleton::kind_qualifier() const {
    // Types with a Singleton kind are equivalent to the operand of the Singleton and thus have the same
    // qualifier
    return is_kind() ? op(0)->qualifier() : world().unlimited();
}

const Def* Star::kind_qualifier() const {
    return is_kind() ? op(0) : world().unlimited();
}

// const Def* Variadic::kind_qualifier() const {
//     // TODO can Variadic kinds have types that are inhabited on the term level and provide a qualifier for them?
//     return world().unlimited();
// }

const Def* Variant::kind_qualifier() const {
    return is_kind() ? world().variant(qualifiers(ops()), world().qualifier_kind()) : world().unlimited();
}

//------------------------------------------------------------------------------

/*
 * shift
 */

size_t Def::shift(size_t) const { return 0; }
size_t Pi::shift(size_t i) const { return i; }
size_t Lambda::shift(size_t) const { return num_domains(); }
size_t Sigma::shift(size_t i) const { return i; }
size_t Variadic::shift(size_t i) const { return i; }

//------------------------------------------------------------------------------

/*
 * hash
 */

uint64_t Def::vhash() const {
    if (is_nominal() || (sort() == Sort::Term && maybe_affine()))
        return gid();

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

const Def* Any         ::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.any(t, ops[0], debug()); }
const Def* App         ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.app(ops[0], ops.skip_front(), debug()); }
const Def* Dim         ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.dim(ops[0], debug()); }
const Def* Extract     ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.extract(ops[0], ops[1], debug()); }
const Def* Axiom       ::rebuild(WorldBase& to, const Def* t, Defs    ) const {
    assert(!is_nominal());
    return to.assume(t, box(), debug());
}
const Def* Error       ::rebuild(WorldBase& to, const Def* t, Defs    ) const { return to.error(t); }
const Def* Intersection::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.intersection(ops, t, debug()); }
const Def* Match       ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* Lambda      ::rebuild(WorldBase& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.lambda(t->as<Pi>()->domains(), ops.front(), debug());
}
const Def* Pi          ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.pi(ops.skip_back(), ops.back(), debug()); }
const Def* Pick        ::rebuild(WorldBase& to, const Def* t, Defs ops) const {
    assert(ops.size() == 1);
    return to.pick(ops.front(), t, debug());
}
const Def* Sigma       ::rebuild(WorldBase& to, const Def*  , Defs ops) const {
    assert(!is_nominal());
    return to.sigma(ops, qualifier(), debug());
}
const Def* Singleton   ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.singleton(ops.front()); }
const Def* Star        ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.star(ops.front()); }
const Def* Tuple       ::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.tuple(t, ops, debug()); }
const Def* Universe    ::rebuild(WorldBase& to, const Def*  , Defs    ) const { return to.universe(); }
const Def* Var         ::rebuild(WorldBase& to, const Def* t, Defs    ) const { return to.var(t, index(), debug()); }
const Def* Variant     ::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.variant(ops, t, debug()); }
const Def* Variadic    ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.variadic(ops[0], ops[1], debug()); }

//------------------------------------------------------------------------------

/*
 * stub
 */

Axiom* Axiom::stub(WorldBase& to, const Def*, Debug) const {
    assert(&world() != &to);
    assert(is_nominal());
    return const_cast<Axiom*>(this);
}
Lambda* Lambda::stub(WorldBase& to, const Def* type, Debug dbg) const {
    return to.pi_lambda(type->as<Pi>(), dbg);
}
Sigma* Sigma::stub(WorldBase& to, const Def* type, Debug dbg) const {
    return to.sigma(num_ops(), type, dbg);
}
Variant* Variant::stub(WorldBase& to, const Def* type, Debug dbg) const {
    return to.variant(num_ops(), type, dbg);
}

//------------------------------------------------------------------------------

/*
 * reduce
 */

const Def* Def::reduce(Defs args, size_t index) const {
    return thorin::reduce(this, args, index);
}

const Def* Pi::reduce(Defs args) const {
    assert(args.size() == num_domains());
    return body()->reduce(args);
}

const Def* Lambda::reduce(Defs args) const {
    assert(args.size() == num_domains());
    return world().app(this, args);
}

const Def* App::try_reduce() const {
    if (cache_)
        return cache_;

    auto pi_type = callee()->type()->as<Pi>();
    // TODO could reduce those with only affine return type, but requires always rebuilding the reduced body
    if (!pi_type->maybe_affine() && !pi_type->body()->maybe_affine()) {
        if (auto lambda = callee()->isa<Lambda>()) {
            if  (!lambda->is_closed()) // don't set cache as long lambda is unclosed
                return this;

            return thorin::reduce(lambda->body(), ops().skip_front(), [&] (const Def* def) { cache_ = def; });
        }
    }

    return cache_ = this;
}

//------------------------------------------------------------------------------

/*
 * Unification
 */

bool Sigma::assignable(Defs defs) const {
    if (num_ops() != defs.size())
        return false;
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        auto reduced_type = thorin::reduce(op(i), defs.get_front(i));
        if (reduced_type->has_error() || defs[i]->type() != reduced_type)
            return false;
    }
    return true;
}

bool Variadic::assignable(Defs defs) const {
    auto size = defs.size();
    if (arity() != world().arity(size))
        return false;
    for (size_t i = 0; i != size; ++i) {
        auto indexed_type = thorin::reduce(body(), {world().index(i, size)});
        assert(!indexed_type->has_error());
        if (defs[i]->type() != indexed_type)
            return false;
    }
    return true;
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
    auto begin = "(";
    auto end = ")";
    auto domains = callee()->type()->as<Pi>()->domains();
    if (std::any_of(domains.begin(), domains.end(), [](auto t) { return t->is_kind(); })) {
        qualifier_stream(os);
        begin = "[";
        end = "]";
    }
    callee()->name_stream(os);
    return stream_list(os, args(), [&](const Def* def) { def->name_stream(os); }, begin, end);
}

std::ostream& Axiom::stream(std::ostream& os) const { return qualifier_stream(os) << name(); }
std::ostream& Dim::stream(std::ostream& os) const { return streamf(os, "dim({})", of()); }

std::ostream& Error::stream(std::ostream& os) const { return os << "<error>"; }

std::ostream& Extract::stream(std::ostream& os) const {
    return tuple()->name_stream(os) << "." << index();
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

std::ostream& Lambda::stream(std::ostream& os) const {
    stream_list(qualifier_stream(os) << "λ", domains(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
    return body()->name_stream(os << ".");
}

std::ostream& Pi::stream(std::ostream& os) const {
    stream_list(qualifier_stream(os) << "Π", domains(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
    return body()->name_stream(os << ".");
}

std::ostream& Pick::stream(std::ostream& os) const {
    os << "pick:";
    type()->name_stream(os);
    destructee()->name_stream(os << "(");
    return os << ")";
}

std::ostream& Sigma::stream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "Σ(", ")");
}

std::ostream& Singleton::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "S(", ")");
}

std::ostream& Star::stream(std::ostream& os) const {
    return os << op(0) << name();
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
    return streamf(os, "[{} × {}]", arity(), body());
}

std::ostream& Variant::stream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∪ ");
}

//------------------------------------------------------------------------------

}
