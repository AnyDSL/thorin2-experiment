#include "thorin/def.h"
#include "thorin/world.h"

namespace thorin {

size_t Def::gid_counter_ = 1;

//------------------------------------------------------------------------------

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
    assert(ops_[i] && "must be set");
    unregister_use(i);
    ops_[i] = nullptr;
}

std::string Def::unique_name() const { return name() + '_' + std::to_string(gid()); }

Array<const Def*> types(Defs defs) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->type();
    return  result;
}

const Def* Lambda::domain() const { return world().sigma(ops().skip_back()); }
const Def* Pi    ::domain() const { return world().sigma(ops().skip_back()); }

/*
 * constructors
 */

App::App(World& world, const Def* callee, Defs args, const std::string& name)
    : Def(world, Node_App, callee->type()->as<Quantifier>()->reduce(1, args), concat(callee, args), name)
{
    assert(world.tuple(domain(), types(args)));
}

Lambda::Lambda(World& world, Defs domain, const Def* body, const std::string& name)
    : Connective(world, Node_Lambda, world.pi(domain, body->type()), concat(domain, body), name)
{}

Pi::Pi(World& world, Defs domain, const Def* body, const std::string& name)
    : Quantifier(world, Node_Pi, body->type(), concat(domain, body), name)
{}

const Def* Sigma::infer_type(World& world, Defs ops) {
    for (auto op : ops)
        if (!op->type())
            return nullptr;
    return world.star();
}

/*
 * domain
 */

const Def* Tuple::domain() const { return world().nat(); }
const Def* Sigma::domain() const { return world().nat(); }

//------------------------------------------------------------------------------

/*
 * hash
 */

uint64_t Def::vhash() const {
    if (is_nominal())
        return gid();

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(int(tag())), num_ops(), type() ? type()->gid() : 0);
    for (auto op : ops_)
        seed = thorin::hash_combine(seed, op->hash());
    return seed;
}

uint64_t Var::vhash() const {
    return thorin::hash_combine(thorin::hash_begin(int(tag())), index(), type() ? type()->gid() : 0);
}

//------------------------------------------------------------------------------

/*
 * equal
 */

bool Def::equal(const Def* other) const {
    if (is_nominal())
        return this == other;

    bool result = this->tag() == other->tag() && this->num_ops() == other->num_ops();

    if (result) {
        for (size_t i = 0, e = num_ops(); result && i != e; ++i)
            result &= this->op(i) == other->op(i);
    }

    return result;
}

bool Var::equal(const Def* other) const {
    return other->isa<Var>() && this->as<Var>()->index() == other->as<Var>()->index()
                             && this->as<Var>()->type()  == other->as<Var>()->type();
}

//------------------------------------------------------------------------------

/*
 * rebuild
 */

const Def* App   ::rebuild(World& to, Defs ops) const { return to.app(ops[0], ops.skip_front(), name()); }
const Def* Assume::rebuild(World&   , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Lambda::rebuild(World& to, Defs ops) const { return to.lambda(ops.skip_back(), ops.back(), name()); }
const Def* Pi    ::rebuild(World& to, Defs ops) const { return to.pi    (ops.skip_back(), ops.back(), name()); }
const Def* Sigma ::rebuild(World& to, Defs ops) const { assert(is_structural()); return to.sigma(ops, name()); }
const Def* Star  ::rebuild(World& to, Defs    ) const { return to.star(); }
const Def* Tuple ::rebuild(World& to, Defs ops) const { return to.tuple(ops, name()); }
const Def* Var   ::rebuild(World& to, Defs ops) const { return to.var(ops[0], index(), name()); }

//------------------------------------------------------------------------------

/*
 * reduce
 */

Array<const Def*> reduce(Def2Def& map, int index, Defs defs, Defs args) {
    Array<const Def*> result(defs.size());
    for (size_t i = 0, e = result.size(); i != e; ++i)
        result[i] = defs[i]->reduce(map, index, args);
    return result;
}

const Def* Def::reduce(Def2Def& map, int index, Defs defs) const {
    if (auto result = find(map, this))
        return result;
    return map[this] = vreduce(map, index, defs);
}

const Def* Lambda::vreduce(Def2Def& map, int index, Defs defs) const {
    auto new_domain = domain()->reduce(map, index, defs);
    return world().lambda(new_domain, body()->reduce(map, index+1, defs), name());
}

const Def* Pi::vreduce(Def2Def& map, int index, Defs defs) const {
    return world().pi(domain(), body()->reduce(map, index+1, defs), name());
}

const Def* Tuple::vreduce(Def2Def& map, int index, Defs defs) const {
    return world().tuple(thorin::reduce(map, index, ops(), defs), name());
}

const Def* Sigma::vreduce(Def2Def& map, int index, Defs defs) const {
    if (is_nominal()) {
        auto sigma = world().sigma(num_ops(), name());
        map[this] = sigma;

        for (size_t i = 0, e = num_ops(); i != e; ++i)
            sigma->set(i, op(i)->reduce(map, index+i, defs));

        return sigma;
    }  else {
        Array<const Def*> ops(num_ops());
        for (size_t i = 0, e = num_ops(); i != e; ++i)
            ops[i] = op(i)->reduce(map, index+i, defs);
        return map[this] = world().sigma(ops, name());
    }
}

const Def* Var::vreduce(Def2Def& map, int index, Defs defs) const {
    if (this->index() == index)
        return world().tuple(type(), defs);
    else if (this->index() > index) // this is a free variable - shift by one
        return world().var(type(), this->index()-1, name());
    else                            // this variable is not free - keep index, reduce type
        return world().var(type()->reduce(map, index, defs), this->index(), name());
}

const Def* App::vreduce(Def2Def& map, int index, Defs defs) const {
    auto ops = thorin::reduce(map, index, this->ops(), defs);
    return world().app(ops[0], ops[1], name());
}

const Def* Assume::vreduce(Def2Def&, int, Defs) const { return this; }
const Def* Star::vreduce(Def2Def&, int, Defs) const { return this; }

//------------------------------------------------------------------------------

/*
 * stream
 */


std::ostream& Lambda::stream(std::ostream& os) const {
    return streamf(os, "λ%.%", domain(), body());
}

std::ostream& Pi::stream(std::ostream& os) const {
    return streamf(os, "Π%.%", domain(), body());
}

std::ostream& Tuple::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->stream(os); }, "(", ")");
}

std::ostream& Sigma::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->stream(os); }, "Σ(", ")");
}

std::ostream& Var::stream(std::ostream& os) const {
    return streamf(os, "<%:%>", index(), type());
}

std::ostream& Assume::stream(std::ostream& os) const { return os << name(); }

std::ostream& Star::stream(std::ostream& os) const {
    return os << '*';
}

std::ostream& App::stream(std::ostream& os) const {
    return stream_list(streamf(os, "(%)", callee()), args(), [&](const Def* def) { def->stream(os); }, "(", ")");
}

}
