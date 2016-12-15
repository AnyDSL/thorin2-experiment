#include <sstream>
#include <stack>

#include "thorin/def.h"
#include "thorin/world.h"

namespace thorin {

// HACK
uint16_t g_hash_gid_counter = 0;

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

void Def::compute_free_vars() {
    foreach_op_index(0, [&] (size_t, const Def* op, size_t shift) {
            free_vars_ |= op->free_vars_ >> shift;
    });

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
    if (!type())
        return Sort::Universe;
    else if (!type()->type())
        return Sort::Kind;
    else if (!type()->type()->type())
        return Sort::Type;
    else {
        assert(!type()->type()->type()->type());
        return Sort::Term;
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
    if (i == num_ops() - 1) {
        assert(std::all_of(ops().begin(), ops().end(), [](const Def* def) { return static_cast<bool>(def); }));
        compute_free_vars();
        closed_ = true;
    }
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
{
    compute_free_vars();
}

Pi::Pi(World& world, Defs domains, const Def* body, Qualifier q, Debug dbg)
    : Quantifier(world, Tag::Pi, body->type()->is_universe() ? (const Def*) world.universe(q) : world.star(q),
                 concat(domains, body), dbg)
{
    compute_free_vars();
}

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
        is_kind |= op->is_kind();
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
const Def* Axiom       ::rebuild(World& to, const Def* t, Defs    ) const {
    assert(!is_nominal());
    return to.assume(t, box(), debug());
}
const Def* Error       ::rebuild(World& to, const Def* t, Defs    ) const { return to.error(t); }
const Def* Intersection::rebuild(World& to, const Def*  , Defs ops) const { return to.intersection(ops, debug()); }
const Def* Match       ::rebuild(World& to, const Def*  , Defs ops) const { return to.match(ops[0], ops.skip_front(), debug()); }
const Def* Lambda      ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(ops.size() == 1);
    assert(!is_nominal());
    return to.pi_lambda(t->as<Pi>(), ops.front(), debug());
}
const Def* Pi          ::rebuild(World& to, const Def*  , Defs ops) const { return to.pi    (ops.skip_back(), ops.back(), debug()); }
const Def* Pick        ::rebuild(World& to, const Def* t, Defs ops) const {
    assert(ops.size() == 1);
    return to.pick(ops.front(), t, debug());
}
const Def* Sigma       ::rebuild(World& to, const Def*  , Defs ops) const {
    assert(!is_nominal());
    return to.sigma(ops, qualifier(), debug());
}
const Def* Star        ::rebuild(World& to, const Def*  , Defs    ) const { return to.star(qualifier()); }
const Def* Tuple       ::rebuild(World& to, const Def* t, Defs ops) const { return to.tuple(t, ops, debug()); }
const Def* Universe    ::rebuild(World& to, const Def*  , Defs    ) const { return to.universe(qualifier()); }
const Def* Var         ::rebuild(World& to, const Def* t, Defs    ) const { return to.var(t, index(), debug()); }
const Def* Variant     ::rebuild(World& to, const Def*  , Defs ops) const { return to.variant(ops, debug()); }

Axiom* Axiom::stub(World& to, const Def*, Debug) const {
    assert(&world() != &to);
    assert(is_nominal());
    return const_cast<Axiom*>(this);
}
Lambda* Lambda::stub(World& to, const Def* type, Debug dbg) const {
   return to.pi_lambda(type->as<Pi>(), dbg);
}
Sigma* Sigma::stub(World& to, const Def* type, Debug dbg) const {
   return to.sigma(num_ops(), type, dbg);
}

//------------------------------------------------------------------------------

/*
 * reduce
 */

class DefIndex {
public:
    DefIndex(const DefIndex&) = default;
    DefIndex(DefIndex&&) = default;
    DefIndex& operator=(DefIndex&&) = default;

    DefIndex() : def_(nullptr), index_(0) {}
    DefIndex(const Def* n, size_t i)
        : def_(n), index_(i)
    {}

    bool operator==(const DefIndex& b) const {
        return def_ == b.def_ && index_ == b.index_;
    }

    const Def* def() const { return def_; }
    size_t index() const { return index_; }

private:
    const Def* def_;
    size_t index_;
};

class DefIndexHash {
public:
    static uint64_t hash(const DefIndex& s) {
        return hash_combine(hash_begin(), s.def(), s.index());
    }
    static bool eq(const DefIndex& a, const DefIndex& b) { return a == b; }
    static DefIndex sentinel() { return DefIndex(); }
};

typedef thorin::HashMap<DefIndex, const Def*, DefIndexHash> Substitutions;
typedef std::stack<DefIndex> NominalTodos;

class Reducer {
public:
    Reducer(const Def* def, Defs args)
        : world_(def->world()), def_(def), index_(0), args_(args)
    {}
    Reducer(const Def* def, size_t index, Defs args)
        : world_(def->world()), def_(def), index_(index), args_(args)
    {}

    const Def* reduce() {
        auto result = reduce_without_nominals();
        reduce_nominals();
        return result;
    }

    const Def* reduce_up_to_nominals() {
        if (!def_->has_free_var_ge(index_)) {
            return def_;
        }
        return reduce(def_, index_);
    }

    void reduce_nominals() {
        while (!nominals_.empty()) {
            const auto& subst = nominals_.top();
            nominals_.pop();
            if (auto replacement = find(map_, subst)) {
                if (replacement == subst.def() || replacement->is_closed()) // XXX why is_closed?
                    continue;
                subst.def()->foreach_op_index(
                    subst.index(), [&] (size_t op_index, const Def* op, size_t shifted_index) {
                        auto new_op = reduce(op, shifted_index);
                        const_cast<Def*>(replacement)->set(op_index, new_op);
                    });
            }
        }
    }


private:
    const Def* reduce(const Def* def, size_t shift) {
        if (auto replacement = find(map_, {def, shift}))
            return replacement;
        if (!def->has_free_var_ge(shift)) {
            map_[{def, shift}] = def;
            return def;
        } else if (def->is_nominal()) {
            auto new_type = reduce(def->type(), shift);
            auto replacement = def->stub(new_type); // TODO better debug info for these
            map_[{def, shift}] = replacement;
            nominals_.push({def, shift});
            return replacement;
        }

        auto new_type = reduce(def->type(), shift);

        if (auto var = def->isa<Var>()) {
            return var_reduce(var, shift, new_type);
        }
        return rebuild_reduce(def, shift, new_type);
    }

    const Def* var_reduce(const Var* var, size_t shift, const Def* new_type) {
        // The shift argument always corresponds to args.back() and thus corresponds to args.size() - 1.
        // Map index() back into the original argument array.
        int arg_index = args_.size() - 1 - var->index() + shift;
        if (arg_index >= 0 && size_t(arg_index) < args_.size()) {
            if (new_type != args_[arg_index]->type())
                // Use the expected type, not the one provided by the arg.
                return world().error(new_type);
            return args_[arg_index];
        } else if (arg_index < 0) {
            // this is a free variable - need to shift by args.size() for the current reduction
            return world().var(var->type(), var->index() - args_.size(), var->debug());
        }
        // this variable is not free - keep index, substitute type
        return world().var(new_type, var->index(), var->debug());
    }

    const Def* rebuild_reduce(const Def* def, size_t shift, const Def* new_type) {
        Array<const Def*> new_ops(def->num_ops());
        def->foreach_op_index(shift, [&] (size_t op_index, const Def* op, size_t shifted_index) {
                auto new_op = reduce(op, shifted_index);
                new_ops[op_index] = new_op;
            });

        auto new_def = def->rebuild(world(), new_type, new_ops);
        return map_[{def, shift}] = new_def;
    }

    World& world() const { return world_; }

    World& world_;
    const Def* def_;
    size_t index_;
    Array<const Def*> args_;
    Substitutions map_;
    NominalTodos nominals_;
};

const Def* Def::reduce(size_t index, Defs args) const {
    return Reducer(this, index, args).reduce();
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
    // Can only really reduce if it's not an Assume of Pi type
    if (auto lambda = callee()->isa<Lambda>()) {
        if  (!lambda->is_closed()) {
            // do not set cache here as a real reduce attempt might be made later when the lambda is closed
            return this;
        }
        // TODO can't reduce if args types don't match the domains
        auto args = ops().skip_front();

        Reducer reducer(lambda->body(), args);
        auto reduced = reducer.reduce_up_to_nominals();

        cache_ = reduced; // possibly an unclosed Def

        // This may build new, identical Apps and thus we need the cache in place before calling it.
        reducer.reduce_nominals();

        return reduced;
    }
    return cache_ = this;
}

const Def* Tuple::extract_type(World& world, const Def* tuple, size_t index) {
    auto sigma = tuple->type()->as<Sigma>();
    auto type = sigma->op(index);
    if (!type->has_free_var_in(0, index))
        return type;

    size_t skipped_shifts = 0;
    for (size_t delta = 1; delta <= index; delta++) {
        if (!type->has_free_var_ge(skipped_shifts)) {
            skipped_shifts++;
            continue;
        }
        auto prev_extract = world.extract(tuple, index - delta);

        // This also shifts any Var with index > skipped_shifts by -1
        type = type->reduce(skipped_shifts, {prev_extract});
    }
    return type;
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
