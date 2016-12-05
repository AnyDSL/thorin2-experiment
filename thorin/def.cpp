#include "thorin/def.h"
#include "thorin/world.h"

namespace thorin {

bool operator<(Qualifier lhs, Qualifier rhs) {
    if (lhs == rhs) return false;
    if (rhs == Qualifier::Unrestricted) return true;
    if (lhs == Qualifier::Linear) return true;
    return false;
}

bool operator<=(Qualifier lhs, Qualifier rhs) {
    return lhs == rhs || lhs < rhs;
}

std::ostream& operator<<(std::ostream& ostream, const Qualifier s) {
    switch (s) {
    case Qualifier::Unrestricted:
        return ostream << ""; //ᵁ
    case Qualifier::Affine:
        return ostream << "ᴬ";
    case Qualifier::Relevant:
        return ostream << "ᴿ";
    case Qualifier::Linear:
        return ostream << "ᴸ";
    default:
        THORIN_UNREACHABLE;
    }
}

Qualifier meet(Qualifier lhs, Qualifier rhs) {
    return Qualifier(static_cast<int>(lhs) | static_cast<int>(rhs));
}

Qualifier meet(const Defs& defs) {
    return std::accumulate(defs.begin(), defs.end(), Qualifier::Unrestricted,
                            [](Qualifier q, const Def* const def) {
                                return def ? meet(q, def->qualifier()) : q;});
}

//------------------------------------------------------------------------------

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
    if (!type())
        return Universe;
    else if (!type()->type())
        return Kind;
    else if (!type()->type()->type())
        return Type;
    else {
        assert(!type()->type()->type()->type());
        return Term;
    }
}

size_t Def::gid_counter_ = 1;

void Def::wire_uses() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i) {
        if (auto def = op(i)) {
            assert(!def->uses_.contains(Use(i, this)));
            const auto& p = def->uses_.emplace(i, this);
            assert_unused(p.second);
        }
    }
}

void Def::set(size_t i, const Def* def) {
    assert(!closed_);
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    assert(!def->uses_.contains(Use(i, this)));
    assert((def->sort() != Sort::Term || !def->type()->is_affine() || def->num_uses() == 0)
           && "Affinely typed terms can be used at most once, check before calling this.");
    const auto& p = def->uses_.emplace(i, this);
    assert_unused(p.second);
    if (i == num_ops() - 1)
        closed_ = true;
}

void Def::unregister_uses() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        unregister_use(i);
}

void Def::unregister_use(size_t i) const {
    auto def = ops_[i];
    assert(def->uses_.contains(Use(i, this)));
    def->uses_.erase(Use(i, this));
    assert(!def->uses_.contains(Use(i, this)));
}

void Def::unset(size_t i) {
    assert((sort() != Sort::Term || !type()->is_relevant())
           && "Do not remove the use of a relevant value.");
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

const Def* Pi::domain() const { return world().sigma(domains()); }

//------------------------------------------------------------------------------

/*
 * constructors
 */

Def::~Def() {
    if (on_heap())
        delete[] ops_;
}

Lambda::Lambda(World& world, const Pi* type, const Def* body, Debug dbg)
    : Constructor(world, Tag::Lambda, type, {body}, dbg)
{}

Pi::Pi(World& world, Defs domains, const Def* body, Qualifier q, Debug dbg)
    : Quantifier(world, Tag::Pi, body->type()->is_universe() ? (const Def*) world.universe(q) : world.star(q),
                 concat(domains, body), dbg)
{}

Sigma::Sigma(World& world, size_t num_ops, Qualifier q, Debug dbg)
    : Sigma(world, world.universe(q), num_ops, dbg)
{}

Star::Star(World& world, Qualifier q)
    : Def(world, Tag::Star, world.universe(q), Defs(), {"*"})
{}

const Def* Quantifier::max_type(World& world, Defs ops, Qualifier q) {
    auto qualifier = Qualifier::Unrestricted;
    auto is_kind = false;
    for (auto op : ops) {
        assert(op->type() && "Type universes shouldn't be operands");
        qualifier = meet(qualifier, op->type()->qualifier());
        is_kind |= op->type()->is_kind();
    }
    assert(q <= qualifier &&
           "Provided qualifier must be as restricted as the meet of the operands qualifiers.");
    return is_kind ? (const Def*) world.universe(q) : world.star(q);
}

//------------------------------------------------------------------------------

/*
 * hash/equal
 */

uint64_t Def::vhash() const {
    if (is_nominal())
        return gid();

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(fields()), type() ? type()->gid() : 0);
    for (auto op : ops())
        seed = thorin::hash_combine(seed, op->gid());
    return seed;
}

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

uint64_t Axiom::vhash() const {
    auto seed = Def::vhash();
    if (is_nominal())
        return seed;

    return thorin::hash_combine(seed, box_.get_u64());
}

uint64_t Extract::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }
uint64_t Var::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }

bool Axiom::equal(const Def* other) const {
    if (is_nominal())
        return this == other;

    return this->fields() == other->fields() && this->type() == other->type()
        && this->box_.get_u64() == other->as<Axiom>()->box().get_u64();
}

bool Extract::equal(const Def* other) const {
    return Def::equal(other) && this->index() == other->as<Extract>()->index();
}
bool Var::equal(const Def* other) const {
    return Def::equal(other) && this->index() == other->as<Var>()->index();
}

//------------------------------------------------------------------------------

/*
 * rebuild
 */

const Def* All         ::rebuild(World& to, const Def*  , Defs ops) const { return to.all(ops, debug()); }
const Def* Any         ::rebuild(World& to, const Def* t, Defs ops) const { return to.any(t, ops[0], debug()); }
const Def* App         ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops.skip_front(), debug()); }
const Def* Extract     ::rebuild(World& to, const Def*  , Defs ops) const { return to.extract(ops[0], index(), debug()); }
const Def* Axiom       ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Error       ::rebuild(World& to, const Def* t, Defs    ) const { return to.error(t); }
const Def* Intersection::rebuild(World& to, const Def*  , Defs ops) const { return to.intersection(ops, debug()); }
const Def* Match       ::rebuild(World& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* Lambda      ::rebuild(World& to, const Def*  , Defs ops) const { return to.lambda(ops.skip_back(), ops.back(), debug()); }
const Def* Pi          ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi    (ops.skip_back(), ops.back(), debug()); }
const Def* Pick        ::rebuild(World& to, const Def* t, Defs ops) const { return to.pick(ops[0], t, debug()); }
const Def* Sigma       ::rebuild(World& to, const Def*  , Defs ops) const { assert(!is_nominal()); return to.sigma(ops, debug()); }
const Def* Star        ::rebuild(World& to, const Def*  , Defs    ) const { return to.star(qualifier()); }
const Def* Tuple       ::rebuild(World& to, const Def* t, Defs ops) const { return to.tuple(t, ops, debug()); }
const Def* Universe    ::rebuild(World& to, const Def*  , Defs    ) const { return to.universe(qualifier()); }
const Def* Var         ::rebuild(World& to, const Def* t, Defs    ) const { return to.var(t, index(), debug()); }
const Def* Variant     ::rebuild(World& to, const Def*  , Defs ops) const { return to.variant(ops, debug()); }

//------------------------------------------------------------------------------

/*
 * reduce
 */

const Def* reduce(const Def* body, Defs args) {
    size_t num_args = args.size();
    Def2Def nominals;
    Def2Def map;
    for (size_t i = 0; i < num_args; ++i) {
        body = body->substitute(nominals, map, num_args - 1 - i, {args[i]});
    }
    return body;
}

const Def* Pi::reduce(Defs args) const {
    assert(args.size() == num_domains());
    return thorin::reduce(body(), args);
}

const Def* Lambda::reduce(Defs args) const {
    assert(args.size() == num_domains());
    if (is_nominal()) {
        // TODO Worklist algorithm for this and substitute
    }
    return thorin::reduce(body(), args);
}

//------------------------------------------------------------------------------

/*
 * substitute
 */

// helpers

Array<const Def*> substitute(Def2Def& nominals, Def2Def& map, size_t index, Defs defs, Defs args) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->substitute(nominals, map, index, args);
    return result;
}

Array<const Def*> binder_substitute(Def2Def& nominals, Def2Def& map, size_t index, Defs ops, Defs args) {
    Array<const Def*> new_ops(ops.size());
    Def2Def new_map;
    Def2Def* cur_map = &map;
    for (size_t i = 0, e = new_ops.size(); i != e; ++i) {
        new_ops[i] = ops[i]->substitute(nominals, *cur_map, index + i, args);
        cur_map = &new_map;
        new_map.clear();
    }
    return new_ops;
}

const Def* Def::substitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    if (is_nominal())
        if (auto result = find(nominals, this))
            return result;
    if (auto result = find(map, this))
        return result;
    auto def = vsubstitute(nominals, map, index, args);
    return map[this] = def;
}

// vsubstitute

const Def* All::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    return world().all(thorin::substitute(nominals, map, index, ops(), args), debug());
}

const Def* Any::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto new_type = type()->substitute(nominals, map, index, args);
    return world().any(new_type, def()->substitute(nominals, map, index, args), debug());
}

const Def* App::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto ops = thorin::substitute(nominals, map, index, this->ops(), args);
    return world().app(ops.front(), ops.skip_front(), debug());
}

const Def* Axiom::vsubstitute(Def2Def&, Def2Def&, size_t, Defs) const { return this; }

const Def* Error::vsubstitute(Def2Def&, Def2Def&, size_t, Defs) const { return this; }

const Def* Extract::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto op = this->destructee()->substitute(nominals, map, index, args);
    return world().extract(op, this->index(), debug());
}

const Def* Intersection::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    return world().intersection(thorin::substitute(nominals, map, index, ops(), args), debug());
}

const Def* Match::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto ops = thorin::substitute(nominals, map, index, this->ops(), args);
    return world().match(ops.front(), ops.skip_front(), debug());
}

const Def* Lambda::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto new_pi = type()->substitute(nominals, map, index, args)->as<Pi>();
    Def2Def new_map;
    if (is_nominal()) {
        auto lambda = world().pi_lambda(new_pi, debug());
        nominals[this] = lambda;
        lambda->set(body()->substitute(nominals, new_map, index + domains().size(), args));
        return lambda;
    } else {
        return world().pi_lambda(new_pi, body()->substitute(nominals, new_map, index + domains().size(), args),
                                 debug());
    }
}

const Def* Pi::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto new_domains = thorin::binder_substitute(nominals, map, index, domains(), args);
    Def2Def new_map;
    return world().pi(new_domains, body()->substitute(nominals, new_map, index + domains().size(), args),
                      debug());
}

const Def* Pick::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    auto new_type = type()->substitute(nominals, map, index, args);
    return world().pick(new_type, destructee()->substitute(nominals, map, index, args), debug());
}

const Def* Sigma::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    if (is_nominal()) {
        auto sigma = world().sigma(num_ops(), type(), debug());
        nominals[this] = sigma;
        auto new_operands = binder_substitute(nominals, map, index, ops(), args);
        for (size_t i = 0, e = num_ops(); i != e; ++i)
            sigma->set(i, new_operands[i]);
        return sigma;
    }  else {
        return world().sigma(binder_substitute(nominals, map, index, ops(), args), debug());
    }
}

const Def* Star::vsubstitute(Def2Def&, Def2Def&, size_t, Defs) const { return this; }

const Def* Tuple::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    return world().tuple(thorin::substitute(nominals, map, index, ops(), args), debug());
}

const Def* Universe::vsubstitute(Def2Def&, Def2Def&, size_t, Defs) const { return this; }

const Def* Var::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    if (this->index() == index)     // substitute
        return world().tuple(type(), args);
    else if (this->index() > index) // this is a free variable - shift by one
        return world().var(type(), this->index()-1, debug());
    else                            // this variable is not free - keep index, substitute type
        return world().var(type()->substitute(nominals, map, index, args), this->index(), debug());
}

const Def* Variant::vsubstitute(Def2Def& nominals, Def2Def& map, size_t index, Defs args) const {
    return world().variant(thorin::substitute(nominals, map, index, ops(), args), debug());
}

//------------------------------------------------------------------------------

/*
 * stream
 */

std::ostream& All::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")", " ∧ ");
}

std::ostream& Any::stream(std::ostream& os) const {
    os << "∨:";
    type()->name_stream(os);
    def()->name_stream(os << "(");
    return os << ")";
}

std::ostream& App::stream(std::ostream& os) const {
    auto begin = "(";
    auto end = ")";
    auto domains = destructee()->type()->as<Pi>()->domains();
    if (std::any_of(domains.begin(), domains.end(), [](auto t) { return t->is_kind(); })) {
        os << qualifier();
        begin = "[";
        end = "]";
    }
    destructee()->name_stream(os);
    return stream_list(os, args(), [&](const Def* def) { def->name_stream(os); }, begin, end);
}

std::ostream& Axiom::stream(std::ostream& os) const { return os << qualifier() << name(); }

std::ostream& Error::stream(std::ostream& os) const { return os << "Error"; }

std::ostream& Extract::stream(std::ostream& os) const {
    return destructee()->name_stream(os) << "." << index();
}

std::ostream& Intersection::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∩ ");
}

std::ostream& Match::stream(std::ostream& os) const {
    os << "match ";
    destructee()->name_stream(os);
    os << " with ";
    return stream_list(os, handlers(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& Lambda::stream(std::ostream& os) const {
    stream_list(os << qualifier() << "λ", domains(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
    return body()->name_stream(os << ".");
}

std::ostream& Pi::stream(std::ostream& os) const {
    stream_list(os << qualifier() << "Π", domains(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
    return body()->name_stream(os << ".");
}

std::ostream& Pick::stream(std::ostream& os) const {
    os << "pick:";
    type()->name_stream(os);
    destructee()->name_stream(os << "(");
    return os << ")";
}

std::ostream& Sigma::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "Σ(", ")");
}

std::ostream& Star::stream(std::ostream& os) const {
    return os << qualifier() << name();
}

std::ostream& Universe::stream(std::ostream& os) const {
    return os << qualifier() << name();
}

std::ostream& Tuple::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& Var::stream(std::ostream& os) const {
    os << "<" << index() << ":";
    return type()->name_stream(os) << ">";
}

std::ostream& Variant::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∪ ");
}

//------------------------------------------------------------------------------

}
