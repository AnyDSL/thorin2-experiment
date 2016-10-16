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
    : Connective(world, Node_Lambda, world.pi(domain, body->type()), {domain, body}, name)
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

/*
 * infer_type
 */

const Def* Tuple::infer_type(World& world, Defs ops, const std::string& name) {
}

const Def* Sigma::infer_type(World& world, Defs ops, const std::string& name) {
}

const Def* App::infer_type(World& world, const Def* callee, const Def* arg, const std::string& name) {
    return callee->type()->as<Quantifier>()->reduce(1, arg);
}

const Def* App::infer_type(World& world, const Def* callee, Defs arg, const std::string& name) {
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
    return thorin::hash_combine(thorin::hash_begin(int(tag())), depth(), type() ? type()->gid() : 0);
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
    return other->isa<Var>() && this->as<Var>()->depth() == other->as<Var>()->depth()
                             && this->as<Var>()->type()  == other->as<Var>()->type();
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

const Def* App   ::vrebuild(World& to, Defs ops) const { return to.app(ops[0], ops[1], name()); }
const Def* Assume::vrebuild(World& to, Defs ops) const { THORIN_UNREACHABLE; }
const Def* Lambda::vrebuild(World& to, Defs ops) const { return to.lambda(ops[0], ops[1], name()); }
const Def* Pi    ::vrebuild(World& to, Defs ops) const { return to.pi(ops[0], ops[1], name()); }
const Def* Sigma ::vrebuild(World& to, Defs ops) const { assert(is_structural()); return to.sigma(ops, name()); }
const Def* Star  ::vrebuild(World& to, Defs    ) const { return to.star(); }
const Def* Tuple ::vrebuild(World& to, Defs ops) const { return to.tuple(ops, name()); }
const Def* Var   ::vrebuild(World& to, Defs ops) const { return to.var(ops[0], depth(), name()); }

//------------------------------------------------------------------------------

/*
 * reduce
 */

const Def* Def::reduce(Def2Def& map, int depth, const Def* def) const {
    if (auto result = find(map, this))
        return result;
    return map[this] = vreduce(map, depth, def);
}

Array<const Def*> Def::reduce_ops(Def2Def& map, int depth, const Def* def) const {
    Array<const Def*> result(num_ops());
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        result[i] = op(i)->reduce(map, depth, def);
    return result;
}

const Def* Lambda::vreduce(Def2Def& map, int depth, const Def* def) const {
    auto new_domain = domain()->reduce(map, depth, def);
    return world().lambda(new_domain, body()->reduce(map, depth+1, def), name());
}

const Def* Pi::vreduce(Def2Def& map, int depth, const Def* def) const {
    return world().pi(domain(), body()->reduce(map, depth+1, def), name());
}

const Def* Tuple::vreduce(Def2Def& map, int depth, const Def* def) const {
    return world().tuple(reduce_ops(map, depth, def), name());
}

const Def* Sigma::vreduce(Def2Def& map, int depth, const Def* def) const {
    if (is_nominal()) {
        auto sigma = world().sigma(num_ops(), name());
        map[this] = sigma;

        for (size_t i = 0, e = num_ops(); i != e; ++i)
            sigma->set(i, op(i)->reduce(map, depth+i, def));

        return sigma;
    }  else {
        Array<const Def*> ops(num_ops());
        for (size_t i = 0, e = num_ops(); i != e; ++i)
            ops[i] = op(i)->reduce(map, depth+i, def);
        return map[this] = world().sigma(ops, name());
    }
}

const Def* Var::vreduce(Def2Def& map, int depth, const Def* def) const {
    if (this->depth() == depth)
        return def;
    else if (this->depth() > depth) // this is a free variable - shift by one
        return world().var(type(), this->depth()-1, name());
    else                            // this variable is not free - keep depth, reduce type
        return world().var(type()->reduce(map, depth, def), this->depth(), name());
}

const Def* App::vreduce(Def2Def& map, int depth, const Def* def) const {
    auto ops = reduce_ops(map, depth, def);
    return world().app(ops[0], ops[1], name());
}

const Def* Assume::vreduce(Def2Def&, int depth, const Def* def) const { return this; }
const Def* Star::vreduce(Def2Def&, int, const Def*) const { return this; }

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
    return streamf(os, "<%:%>", depth(), type());
}

std::ostream& Assume::stream(std::ostream& os) const { return os << name(); }

std::ostream& Star::stream(std::ostream& os) const {
    return os << '*';
}

std::ostream& App::stream(std::ostream& os) const {
    return stream_list(streamf(os, "(%)", callee()), args(), [&](const Def* def) { def->stream(os); }, "(", ")");
}

}
