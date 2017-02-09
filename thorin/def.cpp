#include <sstream>
#include <stack>

#include "thorin/def.h"
#include "thorin/reduce.h"
#include "thorin/world.h"

namespace thorin {

const Def* Def::qualifier() const {
    switch(sort()) {
    case Sort::Term:
        return type()->type()->kind_qualifier();
    case Sort::Type:
        return type()->kind_qualifier();
    case Sort::Kind:
        return kind_qualifier();
    case Sort::Universe:
        return world().unlimited();
    }
}

const Def* Def::kind_qualifier() const {
    return world().unlimited();
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

const Def* Intersection::kind_qualifier() const {
    return is_kind() ? world().intersection(qualifiers(ops()), world().qualifier_kind()) :
        world().unlimited();
}

bool Def::maybe_affine() const {
    const Def* q = qualifier();
    assert(q != nullptr);
    if (auto qu = world().isa_const_qualifier(q)) {
        return qu->box().get_qualifier() >= Qualifier::Affine;
    }
    return true;
}
//------------------------------------------------------------------------------

size_t Def::gid_counter_ = 1;

void Def::compute_free_vars() {
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        free_vars_ |= op(i)->free_vars() >> shift(i);
    free_vars_ |= type()->free_vars_;
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

void Def::wire_uses() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        if (auto def = op(i)) {
            assert(!def->uses_.contains(Use(this, i)));
            const auto& p = def->uses_.emplace(this, i);
            assert_unused(p.second);
        }
    }
}

void Def::set(size_t i, const Def* def) {
    assert(!closed_);
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    assert(!def->uses_.contains(Use(this, i)));
    const auto& p = def->uses_.emplace(this, i);
    assert_unused(p.second);
    if (i == num_ops() - 1) {
        assert(std::all_of(ops().begin(), ops().end(), [](const Def* def) { return static_cast<bool>(def); }));
        compute_free_vars();
        closed_ = true;
    }
    // TODO qualifier inference for types and thus possibly modification of kind?
    // TODO this needs to be overwritten by Variant/Intersection, they may need to rewrite/fold? quadratic!
    // TODO rather to something like mu? or do nominal variants/intersections just make no sense?
    // other on-the-fly normalisation we need to do?
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

void Def::unset(size_t i) {
    assert(ops_[i] && "must be set");
    unregister_use(i);
    ops_[i] = nullptr;
}

std::string Def::unique_name() const { return name() + '_' + std::to_string(gid()); }

Array<const Def*> types(Defs defs) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->type();
    return result;
}

Array<const Def*> qualifiers(Defs defs) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->qualifier();
    return result;
}

void gid_sort(Array<const Def*>* defs) {
    std::sort(defs->begin(), defs->end(), GIDLt<const Def*>());
}

Array<const Def*> gid_sorted(Defs defs) {
    Array<const Def*> result(defs);
    gid_sort(&result);
    return result;
}

void unique_gid_sort(Array<const Def*>* defs) {
    gid_sort(defs);
    auto first_non_unique = std::unique(defs->begin(), defs->end());
    defs->shrink(std::distance(defs->begin(), first_non_unique));
}

Array<const Def*> unique_gid_sorted(Defs defs) {
    Array<const Def*> result(defs);
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
    for (auto op : ops) {
        assert(sort == op->sort() && "Operands must be of the same sort.");
    }
    assertf(sort == Def::Sort::Type || sort == Def::Sort::Kind,
            "Only sort type or kind allowed.");
    return true;
}

const Def* Pi::domain() const { return world().sigma(domains()); }
bool is_array(const Def* def) { return def->isa<Variadic>() && !def->free_vars().test(0); }

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
{
    compute_free_vars();
}

Intersection::Intersection(WorldBase& world, const Def* type, Defs ops, Debug dbg)
    : Def(world, Tag::Intersection, type, set_flatten<Intersection>(ops), dbg)
{
    assert(check_same_sorted_ops(sort(), ops));
    compute_free_vars();
}

Lambda::Lambda(WorldBase& world, const Pi* type, const Def* body, Debug dbg)
    : Def(world, Tag::Lambda, type, {body}, dbg)
{
    compute_free_vars();
}

Pi::Pi(WorldBase& world, Defs domains, const Def* body, const Def* q, Debug dbg)
    : Def(world, Tag::Pi, body->type()->is_universe() ? (const Def*) world.universe() : world.star(q),
          concat(domains, body), dbg)
{
    compute_free_vars();
}

Sigma::Sigma(WorldBase& world, size_t num_ops, Debug dbg)
    : Sigma(world, world.universe(), num_ops, dbg)
{}

Star::Star(WorldBase& world, const Def* qualifier)
    : Def(world, Tag::Star, world.universe(), {qualifier}, {"*"})
{
    compute_free_vars();
}

Variadic::Variadic(WorldBase& world, const Def* arity, const Def* body, Debug dbg)
    : SigmaBase(world, Tag::Variadic, world.universe(), {arity, body}, dbg)
{
    compute_free_vars();
}

Variant::Variant(WorldBase& world, const Def* type, Defs ops, Debug dbg)
    : Def(world, Tag::Variant, type, set_flatten<Variant>(ops), dbg)
{
    // TODO does same sorted ops really hold? ex: matches that return different sorted stuff? allowed?
    assert(check_same_sorted_ops(sort(), ops));
    compute_free_vars();
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
    assert(ops.size() == 1);
    assert(!is_nominal());
    return to.pi_lambda(t->as<Pi>(), ops.front(), debug());
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
    // TODO assert arg types / return error
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

std::ostream& Error::stream(std::ostream& os) const { return os << "Error"; }

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

//std::ostream& Index::stream(std::ostream& os) const {
    //qualifier_stream(os) << index();

    //std::vector<std::array<char, 3>> digits;
    //for (size_t a = arity(); a > 0; a /= 10)
        //digits.push_back({char(0xe2), char(0x82), char(char(0x80) + char(a % 10))}); // utf-8 prefix for subscript 0

    //for (auto i = digits.rbegin(), e = digits.rend(); i != e; ++i) {
        //const auto& digit = *i;
        //os << digit[0] << digit[1] << digit[2];
    //}

    //return os;
//}

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
    return streamf(os, "x{}({})", arity(), body());
}

std::ostream& Variant::stream(std::ostream& os) const {
    return stream_list(qualifier_stream(os), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∪ ");
}

//------------------------------------------------------------------------------

}
