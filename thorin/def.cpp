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

bool Def::maybe_affine() const {
    if (type() == world().qualifier_type())
        return false;
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

Lambda* Lambda::set(const Def* body) {
    return Def::set(0, normalize_ ? flatten(body, type()->domains()) : body)->as<Lambda>();
};

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

const Def* Pi::domain() const { return world().sigma(domains()); }
const Def* Pack::arity() const { return world().dim(type()); }

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

Pack::Pack(WorldBase& world, const Def* type, const Def* body, Debug dbg)
    : TupleBase(world, Tag::Pack, type, {body}, dbg)
{}

Pi::Pi(WorldBase& world, const Def* type, Defs domains, const Def* body, Debug dbg)
    : Def(world, Tag::Pi, type, concat(domains, body), dbg)
{}

Sigma::Sigma(WorldBase& world, size_t num_ops, Debug dbg)
    : Sigma(world, world.universe(), num_ops, dbg)
{}

Star::Star(WorldBase& world, const Def* qualifier)
    : Def(world, Tag::Star, world.universe(), {qualifier}, {"*"})
{}

Variadic::Variadic(WorldBase& world, const Def* type, const Def* arity, const Def* body, Debug dbg)
    : SigmaBase(world, Tag::Variadic, type, {arity, body}, dbg)
{}

Variant::Variant(WorldBase& world, const Def* type, Defs ops, Debug dbg)
    : Def(world, Tag::Variant, type, range(set_flatten<Variant>(ops)), dbg)
{
    // TODO does same sorted ops really hold? ex: matches that return different sorted stuff? allowed?
    assert(check_same_sorted_ops(sort(), ops));
}

//------------------------------------------------------------------------------

/*
 * has_values
 */

bool Axiom::has_values() const {
    return sort() == Sort::Type;
}

bool Intersection::has_values() const {
    return std::all_of(ops().begin(), ops().end(), [](auto op){ return op->has_values(); });
}

bool Pi::has_values() const {
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
    return world().unlimited();
}

const Def* Intersection::kind_qualifier() const {
    assert(is_kind());
    auto qualifiers = DefArray(num_ops(), [&](auto i) { return this->op(i)->qualifier(); });
    return world().intersection(world().qualifier_type(), qualifiers);
}

const Def* Sigma::kind_qualifier() const {
    assert(is_kind());
    auto unlimited = this->world().unlimited();
    if (num_ops() == 0)
        return unlimited;
    auto qualifiers = DefArray(num_ops(), [&](auto i) {
            return this->op(i)->has_values() ? this->op(i)->qualifier() : unlimited; });
    return world().variant(world().qualifier_type(), qualifiers);
}

const Def* Singleton::kind_qualifier() const {
    assert(is_kind());
    // TODO this might recur endlessly if op(0) has this as type, need to forbid that
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
 * dim
 */

const Def* Def::dim() const {
    DefArray arities = dims();
    if (arities.size() == 0)
        return world().arity(1);
    return world().sigma(arities);
}

DefArray Def::dims() const {
    if (is_value())
        return type()->dims();
    THORIN_UNREACHABLE; // must override this
}

// DefArray All::dim() const { return TODO; }

// DefArray Any::dim() const { return TODO; }

// DefArray App::dim() const { return TODO; }

DefArray Axiom::dims() const {
    if (is_value())
        return { world().dim(type()) };
    return {};
}

// DefArray Error::dims() const { return TODO; }

// DefArray Extract::dims() const { return TODO; }

// DefArray Intersection::dims() const { return TODO; }

// DefArray Match::dims() const { return TODO; }

// DefArray Pack::dims() const { return TODO; }

DefArray Pi::dims() const { return {}; }

// DefArray Pick::dims() const { return TODO ; }

DefArray Sigma::dims() const {
    auto size = num_ops();
    switch (size) {
    case 0: return { world().arity(0) };
    case 1: return { ops().front()->dim() }; // conceptually: 1, op->dim
    default:
        auto arity = world().arity(size);
        auto op_arities = world().tuple(DefArray(size, [&](auto i) {
                    return op(i)->dim()->shift_free_vars(i); }));
        return { arity, world().extract(op_arities, world().var(arity, 0)) };
    }
}

// DefArray Singleton::dims() const {

DefArray Star::dims() const { return {}; }

DefArray Universe::dims() const {
    THORIN_UNREACHABLE;
}

DefArray Var::dims() const {
    if (is_value())
        return type()->dims();
    return { world().dim(this) };
}

DefArray Variadic::dims() const {
    DefArray body_arities = body()->dims();
    DefArray arities(body_arities.size() + 1);
    arities[0] = arity();
    std::transform(body_arities.begin(), body_arities.end(), arities.begin() + 1,
                   [](auto arity) { return arity->shift_free_vars(1); });
    return arities;
}

DefArray Variant::dims() const {
    DefArray arities(num_ops(), [&](auto i) { return op(i)->dim(); });
    return { world().variant(arities) };
}


//------------------------------------------------------------------------------

/*
 * shift
 */

size_t Def::shift(size_t) const { return 0; }
size_t Lambda::shift(size_t) const { return num_domains(); }
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

const Def* Any         ::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.any(t, ops[0], debug()); }
const Def* App         ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.app(ops[0], ops.skip_front(), debug()); }
const Def* Dim         ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.dim(ops[0], debug()); }
const Def* Extract     ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.extract(ops[0], ops[1], debug()); }
const Def* Axiom       ::rebuild(WorldBase& to, const Def* t, Defs    ) const {
    assert(!is_nominal());
    return to.assume(t, box(), debug());
}
const Def* Error       ::rebuild(WorldBase& to, const Def* t, Defs    ) const { return to.error(t); }
const Def* Intersection::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.intersection(t, ops, debug()); }
const Def* Match       ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* Lambda      ::rebuild(WorldBase& to, const Def* t, Defs ops) const {
    assert(!is_nominal());
    return to.lambda(t->as<Pi>()->domains(), ops.front(), debug());
}
const Def* Pack        ::rebuild(WorldBase& to, const Def* t, Defs ops) const {
    return t->is_nominal() ? to.pack_nominal_sigma(t->as<Sigma>(), ops[0], debug()) : to.pack(to.dim(t), ops[0], debug());
}
const Def* Pi          ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.pi(ops.skip_back(), ops.back(), debug()); }
const Def* Pick        ::rebuild(WorldBase& to, const Def* t, Defs ops) const {
    assert(ops.size() == 1);
    return to.pick(ops.front(), t, debug());
}
const Def* Sigma       ::rebuild(WorldBase& to, const Def*  , Defs ops) const {
    assert(!is_nominal());
    return to.sigma(qualifier(), ops, debug());
}
const Def* Singleton   ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.singleton(ops[0]); }
const Def* Star        ::rebuild(WorldBase& to, const Def*  , Defs ops) const { return to.star(ops[0]); }
const Def* Tuple       ::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.tuple(t, ops, debug()); }
const Def* Universe    ::rebuild(WorldBase& to, const Def*  , Defs    ) const { return to.universe(); }
const Def* Var         ::rebuild(WorldBase& to, const Def* t, Defs    ) const { return to.var(t, index(), debug()); }
const Def* Variant     ::rebuild(WorldBase& to, const Def* t, Defs ops) const { return to.variant(t, ops, debug()); }
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
    auto pi = type->as<Pi>();
    return to.nominal_lambda(pi->domains(), pi->body(), pi->qualifier(), dbg);
}
Sigma* Sigma::stub(WorldBase& to, const Def* type, Debug dbg) const {
    return to.sigma(type, num_ops(), dbg);
}
Variant* Variant::stub(WorldBase& to, const Def* type, Debug dbg) const {
    return to.variant(type, num_ops(), dbg);
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

    if (auto lambda = callee()->isa<Lambda>()) {
        // TODO could reduce those with only affine return type, but requires always rebuilding the reduced body
        auto args = ops().skip_front();
        if (!pi_type->maybe_affine() && !pi_type->body()->maybe_affine() &&
            (!lambda->is_nominal() ||
             std::all_of(args.begin(), args.end(), [](auto def) { return def->free_vars().none(); }))) {
            if  (!lambda->is_closed()) // don't set cache as long lambda is unclosed
                return this;

            return thorin::reduce(lambda->body(), args, [&] (const Def* def) { cache_ = def; });
        }
    } else if (auto axiom = callee()->isa<Axiom>()) {
        // TODO implement constant folding for primops here
        (void*)axiom;
    }

    return cache_ = this;
}

const Def* Def::shift_free_vars(size_t shift) const {
    return thorin::shift_free_vars(this, shift);
}

//------------------------------------------------------------------------------

/*
 * assignable
 */

bool Sigma::assignable(Defs defs) const {
    if (num_ops() != defs.size())
        return false;
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        auto reduced_type = op(i)->reduce(defs.get_front(i));
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
        auto b = body()->reduce(world().index(i, size));
        assert(!b->has_error());
        if (defs[i]->type() != b)
            return false;
    }
    return true;
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
    dependent_check(domains(), types, checked, {type()->body(), body()});
}

void Pack::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    dependent_check({arity()}, types, checked, {body()});
}

void Pi::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    dependent_check(domains(), types, checked, {body()});
}

void Sigma::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    if (is_nominal_typechecked(this, types, checked))
        return;
    check(type(), types, checked);
    dependent_check(ops(), types, checked, Defs());
}

void Var::typecheck_vars(Environment& types, EnvDefSet& checked) const {
    check(type(), types, checked);
    auto reverse_index = types.size() - 1 - index();
    auto shifted_type = type()->shift_free_vars(index() + 1);
    auto env_type = types[reverse_index];
    assertf(env_type == shifted_type,
            "The type {} of variable {} does not match the type {} declared by the binder.", shifted_type,
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
    return scrutinee()->name_stream(os) << "." << index();
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

std::ostream& Pack::stream(std::ostream& os) const {
    return streamf(os, "({}; {})", arity(), body());
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
    if (num_ops() == 0 && is_kind())
        return os << "Σ*";
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
    if (num_ops() == 0 && is_type())
        return os << "():Σ*";
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
