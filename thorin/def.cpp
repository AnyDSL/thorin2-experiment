#include "thorin/def.h"
#include "thorin/world.h"

#include <stack>

namespace thorin {

std::ostream& Use::stream(std::ostream& os) const {
    return os << def_;
}

//------------------------------------------------------------------------------

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
    return result;
}

//------------------------------------------------------------------------------

/*
 * constructors
 */

Lambda::Lambda(World& world, const Def* type, Defs domains, const Def* body, const std::string& name)
    : Connective(world, Node_Lambda, type, concat(domains, body), name)
{}

Pi::Pi(World& world, Defs domains, const Def* body, const std::string& name)
    : Quantifier(world, Node_Pi, body->type(), concat(domains, body), name)
{}

App::App(World& world, const Def* type, const Def* callee, Defs args, const std::string& name)
    : Def(world, Node_App, type,  concat(callee, args), name)
{
    cache_ = nullptr;
}

const Def* Quantifier::max_type(World& world, Defs ops) {
    for (auto op : ops) {
        if (!op->type())
            return nullptr;
    }
    return world.star();
}

//------------------------------------------------------------------------------

/*
 * domain
 */

const Def* All         ::domain() const { return /*TODO*/nullptr; }
const Def* Any         ::domain() const { return /*TODO*/nullptr; }
const Def* Intersection::domain() const { return /*TODO*/nullptr; }
const Def* Lambda      ::domain() const { return world().sigma(domains()); }
const Def* Pi          ::domain() const { return world().sigma(domains()); }
const Def* Sigma       ::domain() const { return world().nat(); }
const Def* Tuple       ::domain() const { return world().nat(); }
const Def* Variant     ::domain() const { return /*TODO*/nullptr; }

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

    bool result = this->tag() == other->tag() && this->type() == other->type() && this->num_ops() == other->num_ops();
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

const Def* All         ::rebuild(World& to, const Def*  , Defs ops) const { return to.all(ops, name()); }
const Def* Any         ::rebuild(World& to, const Def* t, Defs ops) const { return to.any(t, ops[0], name()); }
const Def* App         ::rebuild(World& to, const Def*  , Defs ops) const { return to.app(ops[0], ops.skip_front(), name()); }
const Def* Assume      ::rebuild(World&   , const Def*  , Defs    ) const { THORIN_UNREACHABLE; }
const Def* Intersection::rebuild(World& to, const Def* t, Defs ops) const { return to.intersection(ops, name()); }
const Def* Lambda      ::rebuild(World& to, const Def*  , Defs ops) const { return to.lambda(ops.skip_back(), ops.back(), name()); }
const Def* Pi          ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi    (ops.skip_back(), ops.back(), name()); }
const Def* Sigma       ::rebuild(World& to, const Def*  , Defs ops) const { assert(!is_nominal()); return to.sigma(ops, name()); }
const Def* Star        ::rebuild(World& to, const Def*  , Defs    ) const { return to.star(); }
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

const Def* Def::substitute(Def2Def& map, int index, Defs args) const {
    if (auto result = find(map, this))
        return result;
    return map[this] = vsubstitute(map, index, args);
}

// reduce

const Def* All::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return /*TODO*/nullptr;
}

const Def* Any::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return /*TODO*/nullptr;
}

const Def* Intersection::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return /*TODO*/nullptr;
}

const Def* Sigma::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return op(std::stoi(defs.front()->name()));
}

const Def* Tuple::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return op(std::stoi(defs.front()->name()));
}

const Def* Variant::reduce(Defs defs) const {
    assert(defs.size() == 1);
    return /*TODO*/nullptr;
}

// vsubstitute

const Def* All::vsubstitute(Def2Def& map, int index, Defs args) const {
    return world().all(thorin::substitute(map, index, ops(), args), name());
}

const Def* Any::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_type = substitute(map, index, {type()});
    return world().any(new_type, substitute(map, index, {def()}));
}

const Def* App::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto ops = thorin::substitute(map, index, this->ops(), args);
    return world().app(ops.front(), ops.skip_front(), name());
}

const Def* Assume::vsubstitute(Def2Def&, int, Defs) const { return this; }


const Def* Intersection::vsubstitute(Def2Def& map, int index, Defs args) const {
    return world().intersection(thorin::substitute(map, index, ops(), args), name());
}

const Def* Lambda::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_domains = thorin::substitute(map, index, domains(), args);
    Def2Def new_map;
    return world().lambda(new_domains, body()->substitute(new_map, index+1, args), name());
}

const Def* Pi::vsubstitute(Def2Def& map, int index, Defs args) const {
    auto new_domains = thorin::substitute(map, index, domains(), args);
    Def2Def new_map;
    return world().pi(new_domains, body()->substitute(new_map, index+1, args), name());
}

const Def* Sigma::vsubstitute(Def2Def& map, int index, Defs args) const {
    if (is_nominal()) {
        assert(false && "TODO");
    }  else {
        Array<const Def*> new_ops(num_ops());
        Def2Def new_map;
        Def2Def* cur_map = &map;
        for (size_t i = 0, e = num_ops(); i != e; ++i) {
            new_ops[i] = op(i)->substitute(*cur_map, index + i, args);
            if (i == 0)
                cur_map = &new_map;
            new_map.clear();
        }

        return world().sigma(new_ops, name());
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
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")", " ∧ ");
}

std::ostream& Any::stream(std::ostream& os) const {
    os << "∧:";
    type()->name_stream(os);
    def()->name_stream(os << "(");
    return os << ")";
}

std::ostream& App::stream(std::ostream& os) const {
    auto begin = "(";
    auto end = ")";
    if (callee()->sort() == Type) {
        begin = "[";
        end = "]";
    }
    return stream_list(streamf(os, "%", callee()), args(),
                      [&](const Def* def) { def->name_stream(os); }, begin, end);
}

std::ostream& Assume::stream(std::ostream& os) const { return os << name(); }

std::ostream& Intersection::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")", " ∩ ");
}

std::ostream& Lambda::stream(std::ostream& os) const {
    stream_list(os << "λ", domains(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
    return body()->name_stream(os << ".");
}

std::ostream& Pi::stream(std::ostream& os) const {
    stream_list(os << "Π", domains(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
    return body()->name_stream(os << ".");
}

std::ostream& Sigma::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "Σ(", ")");
}

std::ostream& Star::stream(std::ostream& os) const {
    return os << '*';
}

std::ostream& Tuple::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")");
}

std::ostream& Var::stream(std::ostream& os) const {
    os << "<" << index() << ":";
    return type()->name_stream(os) << ">";
}

std::ostream& Variant::stream(std::ostream& os) const {
    return stream_list(os, ops(), [&](const Def* def) { def->name_stream(os); }, "(", ")", " ∪ ");
}

//------------------------------------------------------------------------------

}
