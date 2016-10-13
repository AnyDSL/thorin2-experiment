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

std::string Def::unique_name() const {
    return name() + '_' + std::to_string(gid());
}

/*
 * constructors
 */

Lambda::Lambda(World& world, const Def* domain, const Def* body, const std::string& name)
    : Connective(world, Node_Lambda, world.pi(domain, body, name), {domain, body}, name)
{}

Pi::Pi(World& world, const Def* domain, const Def* body, const std::string& name)
    : Quantifier(world, Node_Pi, body->type(), {domain, body}, name)
{}

Tuple::Tuple(World& world, Defs ops, const std::string& name)
    : Connective(world, Node_Tuple, nullptr, ops, name)
{
    Array<const Def*> types(ops.size());
    for (size_t i = 0, e = ops.size(); i != e; ++i)
        types[i] = ops[i]->type();
    set_type(world.sigma(types, name));
}

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
    return thorin::hash_combine(thorin::hash_begin(int(tag())), depth());
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
    return other->isa<Var>() ? this->as<Var>()->depth() == other->as<Var>()->depth() : false;
}

//------------------------------------------------------------------------------

/*
 * rebuild
 */

const Def* Def::rebuild(World& to, Defs ops) const {
    assert(num_ops() == ops.size());
    if (ops.empty() && &world() == &to)
        return this;
    return vrebuild(to, ops);
}

const Def* Sigma::vrebuild(World& to, Defs ops) const {
    if (is_nominal()) {
        auto sigma = to.sigma(ops.size(), name());
        for (size_t i = 0, e = ops.size(); i != e; ++i)
            sigma->set(i, ops[i]);
        return sigma;
    } else
        return to.sigma(ops, name());
}

const Def* App   ::vrebuild(World& to, Defs ops) const { return to.app(ops[0], ops[1], name()); }
const Def* Tuple ::vrebuild(World& to, Defs ops) const { return to.tuple(ops, name()); }
const Def* Lambda::vrebuild(World& to, Defs ops) const { return to.lambda(ops[0], ops[1], name()); }
const Def* Var   ::vrebuild(World& to, Defs ops) const { return to.var(ops[0], depth(), name()); }

//------------------------------------------------------------------------------

/*
 * reduce
 */

const Def* Def::reduce(int depth, const Def* def, Def2Def& map) const {
    if (auto result = find(map, this))
        return result;
    return map[this] = vreduce(depth, def, map);
}

Array<const Def*> Def::reduce_ops(int depth, const Def* def, Def2Def& map) const {
    Array<const Def*> result(num_ops());
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        result[i] = op(i)->reduce(depth, def, map);
    return result;
}

const Def* Lambda::vreduce(int depth, const Def* def, Def2Def& map) const {
    return world().lambda(domain(), body()->reduce(depth+1, def, map), name());
}

const Def* Pi::vreduce(int depth, const Def* def, Def2Def& map) const {
    return world().pi(domain(), body()->reduce(depth+1, def, map), name());
}

const Def* Tuple::vreduce(int depth, const Def* def, Def2Def& map) const {
    return world().tuple(reduce_ops(depth, def, map), name());
}

const Def* Sigma::vreduce(int depth, const Def* def, Def2Def& map) const {
    if (is_nominal()) {
        auto sigma = world().sigma(num_ops(), name());
        map[this] = sigma;

        for (size_t i = 0, e = num_ops(); i != e; ++i)
            sigma->set(i, op(i)->reduce(depth+i, def, map));

        return sigma;
    }  else {
        Array<const Def*> ops(num_ops());
        for (size_t i = 0, e = num_ops(); i != e; ++i)
            ops[i] = op(i)->reduce(depth+i, def, map);
        return map[this] = world().sigma(ops, name());
    }
}

const Def* Var::vreduce(int depth, const Def* def, Def2Def&) const {
    if (this->depth() == depth)
        return def;
    else if (this->depth() > depth)
        return world().var(type(), this->depth()-1, name()); // this is a free variable - shift by one
    else
        return this;                                                // this variable is not free - don't adjust
}

const Def* App::vreduce(int depth, const Def* def, Def2Def& map) const {
    auto ops = reduce_ops(depth, def, map);
    return world().app(ops[0], ops[1], name());
}

//------------------------------------------------------------------------------

}
