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

    URAL min_qualifier(const Defs& defs) {
        auto q = std::accumulate(defs.begin(), defs.end(), Unrestricted,
                                 [](URAL q, const Def* const def) {
                                     return def ? std::min(q, def->qualifier()) : q;});
        return q;
    }

    URAL meet(URAL lhs, URAL rhs) {
        if (lhs <= rhs)
            return lhs;
        if (rhs < lhs)
            return rhs;
        return URAL::Linear;
    }

    URAL meet(const Defs& defs) {
        return std::accumulate(defs.begin(), defs.end(), Unrestricted,
                                 [](URAL q, const Def* const def) {
                                     return def ? meet(q, def->qualifier()) : q;});
    }
}
//------------------------------------------------------------------------------

size_t Def::gid_counter_ = 1;

void Def::set(Defs defs) {
    assert(defs.size() == num_ops());
    for (size_t i = 0, e = defs.size(); i != e; ++i)
        set(i, defs[i]);
}

void Def::set(size_t i, const Def* def) {
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    assert(!def->uses_.contains(Use(i, this)));
    assert(def->sort() != Sort::Term || !def->type()->is_affine() || def->num_uses() == 0
           && "Affinely typed terms can be used at most once.");
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

//------------------------------------------------------------------------------

/*
 * constructors
 */

Lambda::Lambda(World& world, Defs domains, const Def* body, const std::string& name)
    : Connective(world, Node_Lambda, world.pi(domains, body->type()), concat(domains, body), name)
{}


Pi::Pi(World& world, Defs domains, const Def* body, const std::string& name, Qualifier::URAL q)
    : Quantifier(world, Node_Pi, body->type(), concat(domains, body), name, q)
{}

const Def* Sigma::infer_type(World& world, Defs ops) {
    for (auto op : ops) {
        if (!op->type())
            return nullptr;
    }
    return world.star();
}

App::App(World& world, const Def* callee, Defs args, const std::string& name)
    : Def(world, Node_App, callee->type()->as<Quantifier>()->reduce(args),  concat(callee, args), name)
{
    assert(world.tuple(domain(), types(args)));
}

//------------------------------------------------------------------------------

/*
 * domain
 */

const Def* Tuple ::domain() const { return world().nat(); }
const Def* Sigma ::domain() const { return world().nat(); }
const Def* Lambda::domain() const { return world().sigma(domains()); }
const Def* Pi    ::domain() const { return world().sigma(domains()); }

//------------------------------------------------------------------------------

/*
 * hash/equal
 */

uint64_t Def::vhash() const {
    if (is_nominal())
        return gid();

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(int(tag())), num_ops(), type() ? type()->gid() : 0);
    seed = thorin::hash_combine(seed, static_cast<int>(qualifier()));
    for (auto op : ops_)
        seed = thorin::hash_combine(seed, op->hash());
    return seed;
}

bool Def::equal(const Def* other) const {
    if (is_nominal())
        return this == other;

    bool result = this->tag() == other->tag() && this->type() == other->type() && this->num_ops() == other->num_ops();
    result &= this->qualifier() == other->qualifier();
    if (result) {
        for (size_t i = 0, e = num_ops(); result && i != e; ++i)
            result &= this->op(i) == other->op(i);
    }

    return result;
}

uint64_t Var::vhash() const { return thorin::hash_combine(Def::vhash(), index()); }
bool Var::equal(const Def* other) const { return Def::equal(other) && this->index() == other->as<Var>()->index(); }

//------------------------------------------------------------------------------

/*
 * rebuild
 */

const Def* App   ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops.skip_front(), name()); }
const Def* Assume::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Lambda::rebuild(World& to, const Def*  , Defs ops) const { return to.lambda(ops.skip_back(), ops.back(), name()); }
const Def* Pi    ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi    (ops.skip_back(), ops.back(), name()); }
const Def* Sigma ::rebuild(World& to, const Def*  , Defs ops) const { assert(!is_nominal()); return to.sigma(ops, name()); }
const Def* Star  ::rebuild(World& to, const Def*  , Defs    ) const { return to.star(); }
const Def* Tuple ::rebuild(World& to, const Def* t, Defs ops) const { return to.tuple(t, ops, name()); }
const Def* Var   ::rebuild(World& to, const Def* t, Defs    ) const { return to.var(t, index(), name()); }

//------------------------------------------------------------------------------

/*
 * subst
 */

const Def* Tuple::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return op(std::stoi(defs.front()->name()));
}

const Def* Sigma::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return op(std::stoi(defs.front()->name()));
}

Array<const Def*> subst(Def2Def& map, int index, Defs defs, Defs args) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->subst(map, index, args);
    return result;
}

const Def* Def::subst(Def2Def& map, int index, Defs args) const {
    if (auto result = find(map, this))
        return result;
    return map[this] = vsubst(map, index, args);
}

const Def* Lambda::vsubst(Def2Def& map, int index, Defs args) const {
    auto new_domains = thorin::subst(map, index, domains(), args);
    Def2Def new_map;
    return world().lambda(new_domains, body()->subst(new_map, index+1, args), name());
}

const Def* Pi::vsubst(Def2Def& map, int index, Defs args) const {
    auto new_domains = thorin::subst(map, index, domains(), args);
    Def2Def new_map;
    return world().pi(new_domains, body()->subst(new_map, index+1, args), name());
}

const Def* Tuple::vsubst(Def2Def& map, int index, Defs args) const {
    return world().tuple(thorin::subst(map, index, ops(), args), name());
}

const Def* Sigma::vsubst(Def2Def& map, int index, Defs args) const {
    if (is_nominal()) {
        assert(false && "TODO");
    }  else {
        Array<const Def*> new_ops(num_ops());
        Def2Def new_map;
        Def2Def* cur_map = &map;
        for (size_t i = 0, e = num_ops(); i != e; ++i) {
            new_ops[i] = op(i)->subst(*cur_map, index + i, args);
            if (i == 0)
                cur_map = &new_map;
            new_map.clear();
        }

        return world().sigma(new_ops, name());
    }
}

const Def* Var::vsubst(Def2Def& map, int index, Defs args) const {
    if (this->index() == index)     // substitute
        return world().tuple(type(), args);
    else if (this->index() > index) // this is a free variable - shift by one
        return world().var(type(), this->index()-1, name());
    else                            // this variable is not free - keep index, subst type
        return world().var(type()->subst(map, index, args), this->index(), name());
}

const Def* App::vsubst(Def2Def& map, int index, Defs args) const {
    auto ops = thorin::subst(map, index, this->ops(), args);
    return world().app(ops.front(), ops.skip_front(), name());
}

const Def* Assume::vsubst(Def2Def&, int, Defs) const { return this; }
const Def* Star::vsubst(Def2Def&, int, Defs) const { return this; }

//------------------------------------------------------------------------------

/*
 * stream
 */

std::ostream& Lambda::stream(std::ostream& os) const {
    return streamf(stream_list(os << qualifier() << "λ", domains(), [&](const Def* def) { def->stream(os); }, "(", ")"), ".%", body());
}

std::ostream& Pi::stream(std::ostream& os) const {
    return streamf(stream_list(os << qualifier() << "Π", domains(), [&](const Def* def) { def->stream(os); }, "(", ")"), ".%", body());
}

std::ostream& Tuple::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->stream(os); }, "(", ")");
}

std::ostream& Sigma::stream(std::ostream& os) const {
    return stream_list(os << qualifier(), ops(), [&](const Def* def) { def->stream(os); }, "Σ(", ")");
}

std::ostream& Var::stream(std::ostream& os) const {
    return streamf(os << qualifier(), "<%:%>", index(), type());
}

std::ostream& Assume::stream(std::ostream& os) const { return os << qualifier() << name(); }

std::ostream& Star::stream(std::ostream& os) const {
    return os << '*';
}

std::ostream& App::stream(std::ostream& os) const {
    return stream_list(streamf(os << qualifier(), "(%)", callee()), args(), [&](const Def* def) { def->stream(os); }, "(", ")");
}

//------------------------------------------------------------------------------

}
