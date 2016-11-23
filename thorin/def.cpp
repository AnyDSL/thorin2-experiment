#include "thorin/def.h"
#include "thorin/world.h"

#include <stack>

namespace thorin {

namespace Qualifier {
    bool operator==(URAL lhs, URAL rhs) {
        return static_cast<int>(lhs) == static_cast<int>(rhs);
    }

    bool operator<(URAL lhs, URAL rhs) {
        if (lhs == rhs) return false;
        if (rhs == Unrestricted) return true;
        if (lhs == Linear) return true;
        return false;
    }

    bool operator<=(URAL lhs, URAL rhs) {
        return lhs == rhs || lhs < rhs;
    }

    std::ostream& operator<<(std::ostream& ostream, const URAL s) {
        switch (s) {
        case Unrestricted:
            return ostream << ""; //ᵁ
        case Affine:
            return ostream << "ᴬ";
        case Relevant:
            return ostream << "ᴿ";
        case Linear:
            return ostream << "ᴸ";
        default:
            THORIN_UNREACHABLE;
        }
    }

    URAL meet(URAL lhs, URAL rhs) {
        return URAL(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    URAL meet(const Defs& defs) {
        return std::accumulate(defs.begin(), defs.end(), Unrestricted,
                               [](URAL q, const Def* const def) {
                                   return def ? meet(q, def->qualifier()) : q;});
    }
}

//------------------------------------------------------------------------------

Def::Sort Def::sort() const {
    if (!type())
        return Kind;
    else if (!type()->type())
        return Type;
    else {
        assert(!type()->type()->type());
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
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    assert(!def->uses_.contains(Use(i, this)));
    assert(def->sort() != Sort::Term || !def->type()->is_affine() || def->num_uses() == 0
           && "Affinely typed terms can be used at most once, check before calling this.");
    const auto& p = def->uses_.emplace(i, this);
    assert_unused(p.second);
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
    assert(sort() != Sort::Term || !type()->is_relevant()
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

Array<const Def*> gid_sorted(Defs defs) {
    Array<const Def*> result(defs);
    std::sort(result.begin(), result.end(), [](auto def1, auto def2) { return def1->gid() < def2-> gid(); });
    return result;
}

const Def* Pi::domain() const { return world().sigma(domains()); }

//------------------------------------------------------------------------------

/*
 * constructors
 */

Lambda::Lambda(World& world, const Pi* type, const Def* body, const std::string& name)
    : Constructor(world, Node_Lambda, type, {body}, name)
{}

Pi::Pi(World& world, Defs domains, const Def* body, Qualifier::URAL q, const std::string& name)
    : Quantifier(world, Node_Pi, body->type(), concat(domains, body), q, name)
{}

const Def* Quantifier::max_type(World& world, Defs ops, Qualifier::URAL q) {
    auto qualifier = Qualifier::Unrestricted;
    for (auto op : ops) {
        if (!op->type())
            return nullptr;
        qualifier = Qualifier::meet(qualifier, op->type()->qualifier());
    }
    assert(q <= qualifier &&
           "Provided qualifier must be as restricted as the meet of the operands qualifiers.");
    return world.star(q);
}

//------------------------------------------------------------------------------

/*
 * hash/equal
 */

uint64_t Def::vhash() const {
    if (is_nominal())
        return gid();

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(hash_fields()), type() ? type()->gid() : 0);
    for (auto op : ops()) {
        if (op)
            seed = thorin::hash_combine(seed, op->hash());
    }
    return seed;
}

bool Def::equal(const Def* other) const {
    if (is_nominal())
        return this == other;

    bool result = this->tag() == other->tag() && this->type() == other->type()
        && this->num_ops() == other->num_ops() && this->qualifier() == other->qualifier();
    if (result) {
        for (size_t i = 0, e = num_ops(); result && i != e; ++i)
            result &= this->op(i) == other->op(i);
    }

    return result;
}

uint64_t Extract::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }
uint64_t Var::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }
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

const Def* All         ::rebuild(World& to, const Def*  , Defs ops) const { return to.all(ops, name()); }
const Def* Any         ::rebuild(World& to, const Def* t, Defs ops) const { return to.any(t, ops[0], name()); }
const Def* App         ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops.skip_front(), name()); }
const Def* Extract     ::rebuild(World& to, const Def*  , Defs ops) const { return to.extract(ops[0], index(), name()); }
const Def* Assume      ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Error       ::rebuild(World& to, const Def*  , Defs    ) const { return to.error(); }
const Def* Intersection::rebuild(World& to, const Def* t, Defs ops) const { return to.intersection(ops, name()); }
const Def* Match       ::rebuild(World& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), name()); }
const Def* Lambda      ::rebuild(World& to, const Def*  , Defs ops) const { return to.lambda(ops.skip_back(), ops.back(), name()); }
const Def* Pi          ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi    (ops.skip_back(), ops.back(), name()); }
const Def* Pick        ::rebuild(World& to, const Def* t, Defs ops) const { return to.pick(ops[0], t, name()); }
const Def* Sigma       ::rebuild(World& to, const Def*  , Defs ops) const { assert(!is_nominal()); return to.sigma(ops, name()); }
const Def* Star        ::rebuild(World& to, const Def*  , Defs    ) const { return to.star(qualifier()); }
const Def* Tuple       ::rebuild(World& to, const Def* t, Defs ops) const { return to.tuple(t, ops, name()); }
const Def* Var         ::rebuild(World& to, const Def* t, Defs    ) const { return to.var(t, index(), name()); }
const Def* Variant     ::rebuild(World& to, const Def* t, Defs ops) const { return to.variant(ops, name()); }

//------------------------------------------------------------------------------

/*
 * substitute
 */

// helpers

Array<const Def*> substitute(Def2Def& map, int index, Defs defs, Defs args) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->substitute(map, index, args);
    return result;
}

Array<const Def*> binder_substitute(Def2Def& map, int index, Defs ops, Defs args) {
    Array<const Def*> new_ops(ops.size());
    Def2Def new_map;
    Def2Def* cur_map = &map;
    for (size_t i = 0, e = new_ops.size(); i != e; ++i) {
        new_ops[i] = ops[i]->substitute(*cur_map, index + i, args);
        cur_map = &new_map;
        new_map.clear();
    }
    return new_ops;
}

const Def* Def::substitute(Def2Def& map, int index, Defs args) const {
    if (auto result = find(map, this))
        return result;
    return map[this] = vsubstitute(map, index, args);
}

// vsubstitute

const Def* All::vsubstitute(Def2Def& map, int index, Defs args) const {
    return world().all(thorin::substitute(map, index, ops(), args), name());
}

const Def* Any::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_type = type()->substitute(map, index, args);
    return world().any(new_type, def()->substitute(map, index, args), name());
}

const Def* App::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto ops = thorin::substitute(map, index, this->ops(), args);
    return world().app(ops.front(), ops.skip_front(), name());
}

const Def* Assume::vsubstitute(Def2Def&, int, Defs) const { return this; }

const Def* Error::vsubstitute(Def2Def&, int, Defs) const { return this; }

const Def* Extract::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto op = this->destructee()->substitute(map, index, args);
    return world().extract(op, this->index(), name());
}

const Def* Intersection::vsubstitute(Def2Def& map, int index, Defs args) const {
    return world().intersection(thorin::substitute(map, index, ops(), args), name());
}

const Def* Match::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto ops = thorin::substitute(map, index, this->ops(), args);
    return world().match(ops.front(), ops.skip_front(), name());
}

const Def* Lambda::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_pi = type()->substitute(map, index, args)->as<Pi>();
    Def2Def new_map;
    return world().pi_lambda(new_pi, body()->substitute(new_map, index + domains().size(), args), name());
}

const Def* Pi::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_domains = thorin::binder_substitute(map, index, domains(), args);
    Def2Def new_map;
    return world().pi(new_domains, body()->substitute(new_map, index + domains().size(), args), name());
}

const Def* Pick::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_type = type()->substitute(map, index, args);
    return world().pick(new_type, destructee()->substitute(map, index, args), name());
}

const Def* Sigma::vsubstitute(Def2Def& map, int index, Defs args) const {
    if (is_nominal()) {
        assert(false && "TODO");
    }  else {
        return world().sigma(binder_substitute(map, index, ops(), args), name());
    }
}

const Def* Star::vsubstitute(Def2Def&, int, Defs) const { return this; }

const Def* Tuple::vsubstitute(Def2Def& map, int index, Defs args) const {
    return world().tuple(thorin::substitute(map, index, ops(), args), name());
}

const Def* Var::vsubstitute(Def2Def& map, int index, Defs args) const {
    if (this->index() == index)     // substitute
        return world().tuple(type(), args);
    else if (this->index() > index) // this is a free variable - shift by one
        return world().var(type(), this->index()-1, name());
    else                            // this variable is not free - keep index, substitute type
        return world().var(type()->substitute(map, index, args), this->index(), name());
}

const Def* Variant::vsubstitute(Def2Def& map, int index, Defs args) const {
    return world().variant(thorin::substitute(map, index, ops(), args), name());
}

//------------------------------------------------------------------------------

/*
 * stream
 */

std::ostream& All::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∧ ");
}

std::ostream& Any::stream(std::ostream& os) const {
    os << qualifier() << "∨:";
    type()->name_stream(os);
    def()->name_stream(os << "(");
    return os << ")";
}

std::ostream& App::stream(std::ostream& os) const {
    auto begin = "(";
    auto end = ")";
    if (destructee()->sort() == Type) {
        begin = "[";
        end = "]";
    }
    return stream_list(streamf(os << qualifier(), "%", destructee()), args(),
                       [&](const Def* def) { def->name_stream(os); }, begin, end);
}

std::ostream& Assume::stream(std::ostream& os) const { return os << qualifier() << name(); }

std::ostream& Error::stream(std::ostream& os) const { return os << "Error"; }

std::ostream& Extract::stream(std::ostream& os) const {
    return destructee()->name_stream(os) << "." << index();
}

std::ostream& Intersection::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∩ ");
}

std::ostream& Match::stream(std::ostream& os) const {
    os << "match:";
    type()->name_stream(os);
    destructee()->name_stream(os << "(");
    return os << ")";
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
    return os << qualifier() << '*';
}

std::ostream& Tuple::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& Var::stream(std::ostream& os) const {
    os << qualifier() << "<" << index() << ":";
    return type()->name_stream(os) << ">";
}

std::ostream& Variant::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")",
                       " ∪ ");
}

//------------------------------------------------------------------------------

}
