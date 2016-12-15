#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include <bitset>
#include <numeric>
#include <stack>

#include "thorin/util/array.h"
#include "thorin/util/bitset.h"
#include "thorin/util/debug.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"
#include "thorin/tables.h"

namespace thorin {

class Def;
class World;

/**
 * References a user.
 * A @p Def @c u which uses @p Def @c d as @c i^th operand is a @p Use with @p index_ @c i of @p Def @c d.
 */
class Use {
public:
    Use() {}
    Use(uint16_t index, const Def* def)
        : tagged_ptr_(index, def)
    {}

    size_t index() const { return tagged_ptr_.tag(); }
    const Def* def() const { return tagged_ptr_.ptr(); }
    operator const Def*() const { return tagged_ptr_; }
    const Def* operator->() const { return tagged_ptr_; }
    bool operator==(Use other) const { return this->tagged_ptr_ == other.tagged_ptr_; }

private:
    TaggedPtr<const Def> tagged_ptr_;
};

//------------------------------------------------------------------------------

struct UseHash {
    inline static uint64_t hash(Use use);
    static bool eq(Use u1, Use u2) { return u1 == u2; }
    static Use sentinel() { return Use(uint16_t(-1), (const Def*)(-1)); }
};

typedef HashSet<Use, UseHash> Uses;

template<class T>
struct GIDLt {
    bool operator()(T a, T b) { return a->gid() < b->gid(); }
};

template<class T>
struct GIDHash {
    static uint64_t hash(T n) { return n->gid(); }
    static bool eq(T a, T b) { return a == b; }
    static T sentinel() { return T(1); }
};

template<class Key, class Value>
using GIDMap = thorin::HashMap<Key, Value, GIDHash<Key>>;
template<class Key>
using GIDSet = thorin::HashSet<Key, GIDHash<Key>>;

template<class To>
using DefMap  = GIDMap<const Def*, To>;
using DefSet  = GIDSet<const Def*>;
using Def2Def = DefMap<const Def*>;

typedef ArrayRef<const Def*> Defs;

Array<const Def*> types(Defs defs);
void gid_sort(Array<const Def*>* defs);
Array<const Def*> gid_sorted(Defs defs);
void unique_gid_sort(Array<const Def*>* defs);
Array<const Def*> unique_gid_sorted(Defs defs);

class DefIndex;
class DefIndexHash;
typedef thorin::HashMap<DefIndex, const Def*, DefIndexHash> Substitutions;
typedef std::stack<DefIndex> NominalTodos;

//------------------------------------------------------------------------------

enum class Qualifier {
    Unrestricted,
    Affine   = 1 << 0,
    Relevant = 1 << 1,
    Linear = Affine | Relevant,
};

bool operator<(Qualifier lhs, Qualifier rhs);
bool operator<=(Qualifier lhs, Qualifier rhs);

std::ostream& operator<<(std::ostream& ostream, const Qualifier q);

Qualifier meet(Qualifier lhs, Qualifier rhs);
Qualifier meet(const Defs& defs);

//------------------------------------------------------------------------------

/// Base class for all @p Def%s.
class Def : public MagicCast<Def>, public Streamable  {
public:
    enum class Tag {
        All,
        Any,
        App,
        Axiom,
        Error,
        Extract,
        Intersection,
        Lambda,
        Match,
        Pi,
        Pick,
        Sigma,
        Star,
        Tuple,
        Universe,
        Var,
        Variant,
        Num
    };

    enum class Sort {
        Term, Type, Kind, Universe
    };

protected:
    Def(const Def&) = delete;
    Def(Def&&) = delete;
    Def& operator=(const Def&) = delete;

    /// A @p nominal @p Def.
    Def(World& world, Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : debug_(dbg)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , ops_capacity_(num_ops)
        , closed_(num_ops == 0)
        , tag_(unsigned(tag))
        , nominal_(true)
        , num_ops_(num_ops)
        , ops_(&vla_ops_[0])
    {
        std::fill(ops_, ops_ + num_ops, nullptr);
    }

    /// A @p structural @p Def.
    Def(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : debug_(dbg)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , ops_capacity_(ops.size())
        , closed_(true)
        , tag_(unsigned(tag))
        , nominal_(false)
        , num_ops_(ops.size())
        , ops_(&vla_ops_[0])
    {
        std::copy(ops.begin(), ops.end(), ops_);
    }

    ~Def() override;

    void compute_free_vars();
    void set(size_t i, const Def*);
    void wire_uses() const;
    void unset(size_t i);
    void unregister_use(size_t i) const;
    void unregister_uses() const;
    void resize(size_t num_ops);

public:
    //@{
    /// get operands
    Defs ops() const { return Defs(ops_, num_ops_); }
    const Def* op(size_t i) const { return ops()[i]; }
    size_t num_ops() const { return num_ops_; }
    //@}

    //@{
    /// get @p Uses%s
    const Uses& uses() const { return uses_; }
    size_t num_uses() const { return uses().size(); }
    //@}

    //@{
    /// get @p Debug information
    Debug& debug() const { return debug_; }
    Location location() const { return debug_; }
    const std::string& name() const { return debug().name(); }
    std::string unique_name() const;
    //@}

    //@{
    /// get type and @p Sort
    const Def* type() const { return type_; }
    Sort sort() const;
    bool is_term() const { return sort() == Sort::Term; }
    bool is_type() const { return sort() == Sort::Type; }
    bool is_kind() const { return sort() == Sort::Kind; }
    bool is_universe() const { return sort() == Sort::Universe; }
    //@}

    //@{
    /// get @p Qualifier
    Qualifier qualifier() const  { return type() ? type()->qualifier() : Qualifier(qualifier_); }
    bool is_unrestricted() const { return bool(qualifier()) & bool(Qualifier::Unrestricted); }
    bool is_affine() const       { return bool(qualifier()) & bool(Qualifier::Affine); }
    bool is_relevant() const     { return bool(qualifier()) & bool(Qualifier::Relevant); }
    bool is_linear() const       { return bool(qualifier()) & bool(Qualifier::Linear); }
    //@}

    //@{
    /// misc getters
    uint32_t fields() const { return nominal_ << 23u | tag_ << 16u | num_ops_; }
    size_t gid() const { return gid_; }
    static size_t gid_counter() { return gid_counter_; }
    /// A nominal @p Def is always different from each other @p Def.
    bool is_nominal() const { return nominal_; }
    bool is_closed() const { return closed_; }
    Tag tag() const { return Tag(tag_); }
    World& world() const { return *world_; }
    //@}

    //@{
    /// free variables
    // TODO return (dynamic) bitset (wrapper)
    const std::bitset<64>& free_vars() const { return free_vars_; }

    bool has_free_var(size_t index) const {
        return free_vars_[index];
    }

    /// Whether his Def has a free variable with index in [index, index+length).
    bool has_free_var_in(size_t index, size_t length) const {
        std::bitset<64> range;
        for (size_t i = 0; i != length; ++i)
            range.set(index + i);
        return (free_vars() & range).any();
    }

    bool has_free_var_ge(size_t index) const {
        return (free_vars() & (std::bitset<64>().flip() << index)).any();
    }
    //@}

    const Def* reduce(Defs args) const { return reduce(0, args); }
    /// Substitutes variables beginning from the given index with the given Defs and shifts free variables by
    /// the amount of Defs given. Note that the Defs will be indexed in reverse order.
    const Def* reduce(size_t, Defs) const;
    const Def* rebuild(const Def* type, Defs defs) const { return rebuild(world(), type, defs); }
    Def* stub(const Def* type) const { return stub(type, debug()); }
    Def* stub(const Def* type, Debug dbg) const { return stub(world(), type, dbg); }

    virtual Def* stub(World&, const Def*, Debug) const { THORIN_UNREACHABLE; }
    virtual bool maybe_dependent() const { return true; }
    virtual std::ostream& name_stream(std::ostream& os) const {
        if (name() != "" || is_nominal())
            return os << qualifier() << name();
        return stream(os);
    }

protected:
    // TODO could replace this with something that returns an iterator
    virtual void foreach_op_index(size_t index, std::function<void(size_t, const Def*, size_t)> fn) const {
        for (size_t i = 0, e = num_ops(); i < e; ++i)
            fn(i, op(i), index);
    }

    //@{
    /// hash and equal
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    //@}

    union {
        mutable const Def* cache_;  ///< Used by @p App.
        size_t index_;              ///< Used by @p Var, @p Extract.
        Box box_;                   ///< Used by @p Axiom.
        Qualifier qualifier_;       ///< Used by @p Universe.
    };
    std::bitset<64> free_vars_;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const = 0;
    bool on_heap() const { return ops_ != vla_ops_; }
    // this must match with the 64bit fields below

    static size_t gid_counter_;

    mutable Debug debug_;
    mutable World* world_;
    const Def* type_;
    mutable uint64_t hash_ = 0;
    unsigned gid_           : 23;
    unsigned ops_capacity_  : 16;
    unsigned closed_        :  1;
    unsigned tag_           :  7;
    unsigned nominal_       :  1;
    unsigned num_ops_       : 16;
    // this sum must be 64   ^^^

    static_assert(int(Tag::Num) <= 128, "you must increase the number of bits in tag_");

    mutable Uses uses_;
    const Def** ops_;
    const Def* vla_ops_[0];

    friend class App;
    friend class Cleaner;
    friend class Reducer;
    friend class Scope;
    friend class World;
};

uint64_t UseHash::hash(Use use) {
    return uint64_t(use.index() & 0x3) | uint64_t(use->gid()) << 2ull;
}

class Quantifier : public Def {
protected:
    Quantifier(World& world, Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : Def(world, tag, type, num_ops, dbg)
    {}
    Quantifier(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(world, tag, type, ops, dbg)
    {}

    static const Def* max_type(World&, Defs, Qualifier = Qualifier::Unrestricted);
};

class Constructor : public Def {
protected:
    Constructor(World& world, Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : Def(world, tag, type, num_ops, dbg)
    {}
    Constructor(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(world, tag, type, ops, dbg)
    {}

};

class Destructor : public Def {
protected:
    Destructor(World& world, Tag tag, const Def* type, const Def* op, Debug dbg)
        : Destructor(world, tag, type, Defs({op}), dbg)
    {}
    Destructor(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(world, tag, type, ops, dbg)
    {
        cache_ = nullptr;
    }

public:
    const Def* destructee() const { return ops().front(); }
    const Quantifier* quantifier() const { return destructee()->type()->as<Quantifier>(); }
    Defs args() const { return ops().skip_front(); }
    size_t num_args() const { return args().size(); }
    const Def* arg(size_t i = 0) const { return args()[i]; }
};

class Pi : public Quantifier {
private:
    Pi(World& world, Defs domains, const Def* body, Qualifier q, Debug dbg);

public:
    const Def* domain() const;
    Defs domains() const { return ops().skip_back(); }
    size_t num_domains() const { return domains().size(); }
    const Def* body() const { return ops().back(); }
    const Def* reduce(Defs) const;

    std::ostream& stream(std::ostream&) const override;

private:
    void foreach_op_index(size_t index, std::function<void(size_t, const Def*, size_t)> fn) const override {
        for (size_t i = 0, e = num_ops(); i < e; ++i)
            fn(i, op(i), index++);
    }
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Lambda : public Constructor {
private:
    /// Nominal/recursive Lambda
    Lambda(World& world, const Pi* type, Debug dbg)
        : Constructor(world, Tag::Lambda, type, 1, dbg)
    {}
    Lambda(World& world, const Pi* type, const Def* body, Debug dbg);

public:
    const Def* domain() const { return type()->domain(); }
    Defs domains() const { return type()->domains(); }
    size_t num_domains() const { return domains().size(); }
    const Def* body() const { return op(0); }
    const Def* reduce(Defs) const;
    void set(const Def* def) { Def::set(0, def); };
    const Pi* type() const { return Constructor::type()->as<Pi>(); }
    Lambda* stub(World&, const Def*, Debug) const override;

    std::ostream& stream(std::ostream&) const override;

private:
    void foreach_op_index(size_t index, std::function<void(size_t, const Def*, size_t)> fn) const override {
        for (size_t i = 0, e = num_ops(); i < e; ++i)
            fn(i, op(i), index + num_domains());
    }
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class App : public Destructor {
private:
    App(World& world, const Def* type, const Def* callee, Defs args, Debug dbg)
        : Destructor(world, Tag::App, type, concat(callee, args), dbg)
    {
        compute_free_vars();
    }

public:
    const Def* callee() const { return destructee(); }
    std::ostream& stream(std::ostream&) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    const Def* try_reduce() const;

    friend class World;
};

class Sigma : public Quantifier {
private:
    /// Nominal Sigma kind
    Sigma(World& world, size_t num_ops, Qualifier q, Debug dbg);
    /// Nominal Sigma type, \a type is some Star/Universe
    Sigma(World& world, const Def* type, size_t num_ops, Debug dbg)
        : Quantifier(world, Tag::Sigma, type, num_ops, dbg)
    {}
    Sigma(World& world, Defs ops, Qualifier q, Debug dbg)
        : Quantifier(world, Tag::Sigma, max_type(world, ops, q), ops, dbg)
    {
        compute_free_vars();
    }

public:
    bool is_unit() const { return ops().empty(); }
    void set(size_t i, const Def* def) { Def::set(i, def); };
    std::ostream& stream(std::ostream&) const override;
    Sigma* stub(World&, const Def*, Debug) const override;

private:
    void foreach_op_index(size_t index, std::function<void(size_t, const Def*, size_t)> fn) const override {
        for (size_t i = 0, e = num_ops(); i < e; ++i)
            fn(i, op(i), index++);
    }
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Tuple : public Constructor {
private:
    Tuple(World& world, const Sigma* type, Defs ops, Debug dbg)
        : Constructor(world, Tag::Tuple, type, ops, dbg)
    {
        assert(type->num_ops() == ops.size());
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;
    static const Def* extract_type(World&, const Def* tuple, size_t index);

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Extract : public Destructor {
private:
    Extract(World& world, const Def* type, const Def* tuple, size_t index, Debug dbg)
        : Destructor(world, Tag::Extract, type, tuple, dbg)
    {
        index_ = index;
        compute_free_vars();
    }

public:
    size_t index() const { return index_; }
    std::ostream& stream(std::ostream&) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Intersection : public Quantifier {
private:
    Intersection(World& world, Defs ops, Qualifier q, Debug dbg)
        : Quantifier(world, Tag::Intersection, max_type(world, ops, q), gid_sorted(ops), dbg)
    {
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class All : public Constructor {
private:
    All(World& world, const Intersection* type, Defs ops, Debug dbg)
        : Constructor(world, Tag::All, type, ops, dbg)
    {
        assert(type->num_ops() == ops.size());
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Pick : public Destructor {
private:
    Pick(World& world, const Def* type, const Def* def, Debug dbg)
        : Destructor(world, Tag::Pick, type, def, dbg)
    {
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Variant : public Quantifier {
private:
    Variant(World& world, Defs ops, Qualifier q, Debug dbg)
        : Quantifier(world, Tag::Variant, max_type(world, ops, q), gid_sorted(ops), dbg)
    {
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Any : public Constructor {
private:
    Any(World& world, const Def* type, const Def* def, Debug dbg)
        : Constructor(world, Tag::Any, type, {def}, dbg)
    {
        compute_free_vars();
    }

public:
    const Def* def() const { return op(0); }
    size_t index() const {
        auto def_type = def()->type();
        auto variant = type()->as<Variant>();
        for (size_t i = 0; i < variant->num_ops(); ++i)
            if (def_type == variant->op(i))
                return i;
        THORIN_UNREACHABLE;
    }

    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Match : public Destructor {
private:
    Match(World& world, const Def* type, const Def* def, const Defs handlers, Debug dbg)
        : Destructor(world, Tag::Match, type, concat(def, handlers), dbg)
    {
        compute_free_vars();
    }

public:
    Defs handlers() const { return ops().skip_front(); }

    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world, Qualifier q);

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Universe : public Def {
private:
    Universe(World& world, Qualifier q)
        : Def(world, Tag::Universe, nullptr, 0, {"□"})
    {
        qualifier_ = q;
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Var : public Def {
private:
    Var(World& world, const Def* type, size_t index, Debug dbg)
        : Def(world, Tag::Var, type, Defs(), dbg)
    {
        index_ = index;
        compute_free_vars();
        free_vars_.set(index);
    }

public:
    size_t index() const { return index_; }
    std::ostream& stream(std::ostream&) const override;
    /// Do not print variable names as they aren't bound in the output without analysing DeBruijn-Indices.
    std::ostream& name_stream(std::ostream& os) const override {
        return stream(os);
    }

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Axiom : public Def {
private:
    /// A @em nominal axiom.
    Axiom(World& world, const Def* type, Debug dbg)
        : Def(world, Tag::Axiom, type, 0, dbg)
    {
        assert(type->free_vars().none());
    }

    /// A @em structural axiom.
    Axiom(World& world, const Def* type, Box box, Debug dbg)
        : Def(world, Tag::Axiom, type, Defs(), dbg)
    {
        box_ = box;
    }

public:
    Box box() const { assert(!is_nominal()); return box_; }
    bool maybe_dependent() const override { return false; }
    std::ostream& stream(std::ostream&) const override;
    Axiom* stub(World&, const Def*, Debug) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Error : public Def {
private:
    Error(World& world, const Def* type)
        : Def(world, Tag::Error, type, Defs(), {"<error>"})
    {}

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

inline bool is_error(const Def* def) { return def->tag() == Def::Tag::Error; }

}

#endif
