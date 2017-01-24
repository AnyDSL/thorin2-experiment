#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include <numeric>
#include <set>
#include <stack>

#include "thorin/util/array.h"
#include "thorin/util/bitset.h"
#include "thorin/util/location.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"
#include "thorin/tables.h"

namespace thorin {

class Def;
class WorldBase;

/**
 * References a user.
 * A Def @c u which uses Def @c d as @c i^th operand is a Use with Use::index_ @c i of Def @c d.
 */
class Use {
public:
    Use() {}
    Use(const Def* def, size_t index)
        : tagged_ptr_(def, index)
    {}

    size_t index() const { return tagged_ptr_.index(); }
    const Def* def() const { return tagged_ptr_.ptr(); }
    operator const Def*() const { return tagged_ptr_; }
    const Def* operator->() const { return tagged_ptr_; }
    bool operator==(Use other) const { return this->tagged_ptr_ == other.tagged_ptr_; }

private:
    TaggedPtr<const Def, size_t> tagged_ptr_;
};

//------------------------------------------------------------------------------

struct UseHash {
    inline static uint64_t hash(Use use);
    static bool eq(Use u1, Use u2) { return u1 == u2; }
    static Use sentinel() { return Use((const Def*)(-1), uint16_t(-1)); }
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
using SortedDefSet = std::set<const Def*, GIDLt<const Def*>>;

typedef ArrayRef<const Def*> Defs;

Array<const Def*> types(Defs defs);
void gid_sort(Array<const Def*>* defs);
Array<const Def*> gid_sorted(Defs defs);
void unique_gid_sort(Array<const Def*>* defs);
Array<const Def*> unique_gid_sorted(Defs defs);

//------------------------------------------------------------------------------

enum class Qualifier {
    Unrestricted,
    Relevant = 1 << 0,
    Affine   = 1 << 1,
    Linear = Affine | Relevant,
};

bool operator<(Qualifier lhs, Qualifier rhs);
bool operator<=(Qualifier lhs, Qualifier rhs);

std::ostream& operator<<(std::ostream& ostream, const Qualifier q);

Qualifier meet(Qualifier lhs, Qualifier rhs);
Qualifier meet(const Defs& defs);

template<class T>
std::array<const T*, 4> array_for_qualifiers(std::function<const T*(Qualifier)> fn) {
    return {fn(Qualifier::Unrestricted), fn(Qualifier::Relevant), fn(Qualifier::Affine),
            fn(Qualifier::Linear)};
}

//------------------------------------------------------------------------------

/// Base class for all Def%s.
class Def : public MagicCast<Def>, public Streamable  {
public:
    enum class Tag {
        All,
        Any,
        App,
        Arity,
        Axiom,
        Error,
        Extract,
        Index,
        Intersection,
        Lambda,
        Match,
        Pi,
        Pick,
        Sigma,
        Singleton,
        Space,
        Star,
        Tuple,
        Universe,
        Var,
        VariadicTuple,
        VariadicSigma,
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

    /// A @em nominal Def.
    Def(WorldBase& world, Tag tag, const Def* type, size_t num_ops, Debug dbg)
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

    /// A @em structural Def.
    template<class R>
    Def(WorldBase& world, Tag tag, const Def* type, const R& ops, Debug dbg,
        typename std::enable_if<!std::is_integral<R>::value>::type* dummy = nullptr)
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
        assert_unused(dummy == nullptr);
    }

    /// A @em structural Def.
    Def(WorldBase& world, Tag tag, const Def* type, std::initializer_list<const Def*> ops, Debug dbg)
        : Def(world, tag, type, Defs(ops), dbg)
    {}

    ~Def() override;

    void compute_free_vars();
    void set(size_t i, const Def*);
    void wire_uses() const;
    void unset(size_t i);
    void unregister_use(size_t i) const;
    void unregister_uses() const;
    void resize(size_t num_ops);

public:
    //@{ get operands
    Defs ops() const { return Defs(ops_, num_ops_); }
    const Def* op(size_t i) const { return ops()[i]; }
    size_t num_ops() const { return num_ops_; }
    //@}

    //@{ get Uses%s
    const Uses& uses() const { return uses_; }
    size_t num_uses() const { return uses().size(); }
    //@}

    //@{ get Debug information
    Debug& debug() const { return debug_; }
    Location location() const { return debug_; }
    const std::string& name() const { return debug().name(); }
    std::string unique_name() const;
    //@}

    //@{ get type and Sort
    const Def* type() const { return type_; }
    Sort sort() const;
    bool is_term() const { return sort() == Sort::Term; }
    bool is_type() const { return sort() == Sort::Type; }
    bool is_kind() const { return sort() == Sort::Kind; }
    bool is_universe() const { return sort() == Sort::Universe; }
    //@}

    //@{ get Qualifier
    Qualifier qualifier() const  { return type() ? type()->qualifier() : Qualifier(qualifier_); }
    bool is_unrestricted() const { return bool(qualifier()) & bool(Qualifier::Unrestricted); }
    bool is_affine() const       { return bool(qualifier()) & bool(Qualifier::Affine); }
    bool is_le_affine() const    { return bool(qualifier() <= Qualifier::Affine); }
    bool is_relevant() const     { return bool(qualifier()) & bool(Qualifier::Relevant); }
    bool is_le_relevant() const  { return bool(qualifier() <= Qualifier::Affine); }
    bool is_linear() const       { return bool(qualifier()) & bool(Qualifier::Linear); }
    //@}

    //@{ misc getters
    const BitSet& free_vars() const { return free_vars_; }
    uint32_t fields() const { return nominal_ << 23u | tag_ << 16u | num_ops_; }
    size_t gid() const { return gid_; }
    static size_t gid_counter() { return gid_counter_; }
    /// A nominal Def is always different from each other Def.
    bool is_nominal() const { return nominal_; }
    bool is_closed() const { return closed_; }
    Tag tag() const { return Tag(tag_); }
    WorldBase& world() const { return *world_; }
    //@}

    /**
     * Substitutes Var%s beginning from @p index with @p args and shifts free Var%s by the number of @p args.
     * Note that @p args will be indexed in reverse order due to De Bruijn way of counting.
     */
    const Def* reduce(Defs args, size_t index = 0) const;
    const Def* rebuild(const Def* type, Defs defs) const { return rebuild(world(), type, defs); }
    Def* stub(const Def* type) const {
        if (!name().empty()) {
            Debug new_dbg(debug());
            new_dbg.set(name() + std::to_string(Def::gid_counter()));
            return stub(type, new_dbg);
        }
        return stub(type, debug());
    }
    Def* stub(const Def* type, Debug dbg) const { return stub(world(), type, dbg); }

    virtual Def* stub(WorldBase&, const Def*, Debug) const { THORIN_UNREACHABLE; }
    virtual std::ostream& name_stream(std::ostream& os) const {
        if (name() != "" || is_nominal())
            return os << qualifier() << name();
        return stream(os);
    }

protected:
    /**
     * The amount to shift De Bruijn indices when descending into this Def's @p i's Def::op.
     * For example:
@code{.cpp}
    for (size_t i = 0, e = def->num_ops(); i != e; ++i) {
        size_t new_offset = cur_offset + def->shift(i);
        do_sth(def->op(i), new_offset);
    }
@endcode
    */
    virtual size_t shift(size_t i) const;

    //@{ hash and equal
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    //@}

    union {
        mutable const Def* cache_;  ///< Used by App.
        size_t arity_;              ///< Used by Arity.
        size_t index_;              ///< Used by Index, Var.
        Box box_;                   ///< Used by Axiom.
        Qualifier qualifier_;       ///< Used by Universe.
    };
    BitSet free_vars_;

private:
    virtual const Def* rebuild(WorldBase&, const Def*, Defs) const = 0;
    bool on_heap() const { return ops_ != vla_ops_; }
    // this must match with the 64bit fields below

    static size_t gid_counter_;

    mutable Debug debug_;
    mutable WorldBase* world_;
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
    friend class WorldBase;
};

uint64_t UseHash::hash(Use use) {
    return uint64_t(use.index() & 0x3) | uint64_t(use->gid()) << 2ull;
}

class Pi : public Def {
private:
    Pi(WorldBase& world, Defs domains, const Def* body, Qualifier q, Debug dbg);

public:
    const Def* domain() const;
    Defs domains() const { return ops().skip_back(); }
    size_t num_domains() const { return domains().size(); }
    const Def* body() const { return ops().back(); }
    const Def* reduce(Defs) const;

    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Lambda : public Def {
private:
    /// @em Nominal/recursive Lambda
    Lambda(WorldBase& world, const Pi* type, Debug dbg)
        : Def(world, Tag::Lambda, type, 1, dbg)
    {}
    Lambda(WorldBase& world, const Pi* type, const Def* body, Debug dbg);

public:
    const Def* domain() const { return type()->domain(); }
    Defs domains() const { return type()->domains(); }
    size_t num_domains() const { return domains().size(); }
    const Def* body() const { return op(0); }
    const Def* reduce(Defs) const;
    void set(const Def* def) { Def::set(0, def); };
    const Pi* type() const { return Def::type()->as<Pi>(); }
    Lambda* stub(WorldBase&, const Def*, Debug) const override;

    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class App : public Def {
private:
    App(WorldBase& world, const Def* type, const Def* callee, Defs args, Debug dbg)
        : Def(world, Tag::App, type, concat(callee, args), dbg)
    {
        cache_ = nullptr;
        compute_free_vars();
    }

public:
    const Def* callee() const { return op(0); }
    Defs args() const { return ops().skip_front(); }
    size_t num_args() const { return args().size(); }
    const Def* arg(size_t i) const { return args()[i]; }

    std::ostream& stream(std::ostream&) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;
    const Def* try_reduce() const;

    friend class WorldBase;
};

class Sigma : public Def {
private:
    /// Nominal Sigma kind
    Sigma(WorldBase& world, size_t num_ops, Qualifier q, Debug dbg);
    /// Nominal Sigma type, \a type is some Star/Universe
    Sigma(WorldBase& world, const Def* type, size_t num_ops, Debug dbg)
        : Def(world, Tag::Sigma, type, num_ops, dbg)
    {}
    Sigma(WorldBase& world, Defs ops, Qualifier q, Debug dbg)
        : Def(world, Tag::Sigma, max_type(world, ops, q), ops, dbg)
    {
        compute_free_vars();
    }

public:
    bool is_unit() const { return ops().empty(); }
    void set(size_t i, const Def* def) { Def::set(i, def); };
    std::ostream& stream(std::ostream&) const override;
    Sigma* stub(WorldBase&, const Def*, Debug) const override;

private:
    static const Def* max_type(WorldBase& world, Defs ops, Qualifier q);
    size_t shift(size_t) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Tuple : public Def {
private:
    Tuple(WorldBase& world, const Sigma* type, Defs ops, Debug dbg)
        : Def(world, Tag::Tuple, type, ops, dbg)
    {
        assert(type->num_ops() == ops.size());
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Extract : public Def {
private:
    Extract(WorldBase& world, const Def* type, const Def* tuple, const Def* index, Debug dbg)
        : Def(world, Tag::Extract, type, {tuple, index}, dbg)
    {
        compute_free_vars();
    }

public:
    const Def* tuple() const { return op(0); }
    const Def* index() const { return op(1); }
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Intersection : public Def {
private:
    Intersection(WorldBase& world, Defs ops, Qualifier q, Debug dbg);

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Pick : public Def {
private:
    Pick(WorldBase& world, const Def* type, const Def* def, Debug dbg)
        : Def(world, Tag::Pick, type, {def}, dbg)
    {
        compute_free_vars();
    }

public:
    const Def* destructee() const { return op(0); }
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Variant : public Def {
private:
    Variant(WorldBase& world, Defs ops, Qualifier q, Debug dbg);
    Variant(WorldBase& world, const Def* type, size_t num_ops, Debug dbg)
        : Def(world, Tag::Variant, type, num_ops, dbg)
    {}

public:
    void set(size_t i, const Def* def) { Def::set(i, def); };
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;
    Variant* stub(WorldBase&, const Def*, Debug) const override;

    friend class WorldBase;
};

/// Cast a Def to a Variant type.
class Any : public Def {
private:
    Any(WorldBase& world, const Variant* type, const Def* def, Debug dbg)
        : Def(world, Tag::Any, type, {def}, dbg)
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
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Match : public Def {
private:
    Match(WorldBase& world, const Def* type, const Def* def, const Defs handlers, Debug dbg)
        : Def(world, Tag::Match, type, concat(def, handlers), dbg)
    {
        compute_free_vars();
    }

public:
    const Def* destructee() const { return op(0); }
    Defs handlers() const { return ops().skip_front(); }
    const Def* handler(size_t i) const { return handlers()[i]; }
    size_t num_handlers() const { return handlers().size(); }

    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Singleton : public Def {
private:
    Singleton(WorldBase& world, const Def* def, Debug dbg)
        : Def(world, Tag::Singleton, def->type()->type(), {def}, dbg)
    {
        assert((def->is_term() || def->is_type()) && "No singleton type universes allowed.");
        compute_free_vars();
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Space : public Def {
private:
    Space(WorldBase& world, Qualifier q);

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Arity : public Def {
private:
    Arity(WorldBase& world, size_t arity, Qualifier q, Debug dbg);

public:
    size_t arity() const { return arity_; }
    std::ostream& stream(std::ostream&) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Index : public Def {
private:
    Index(WorldBase& world, const Arity* arity, size_t index, Debug dbg);

public:
    size_t index() const { return index_; }
    const Arity* type() const { return Def::type()->as<Arity>(); }
    size_t arity() const { return type()->arity(); }
    std::ostream& stream(std::ostream&) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class VariadicSigma : public Def {
private:
    VariadicSigma(WorldBase& world, const Def* dimension, const Def* body, Debug dbg);

public:
    const Def* dimension() const { return op(0); }
    const Def* body() const { return op(1); }
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class VariadicTuple : public Def {
private:
    VariadicTuple(WorldBase& world, const Def* type, const Def* body, Debug dbg);

public:
    const Def* body() const { return op(0); }
    //const Def* reduce(Defs) const;
    const VariadicSigma* type() const { return Def::type()->as<VariadicSigma>(); }

    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Star : public Def {
private:
    Star(WorldBase& world, Qualifier q);

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Universe : public Def {
private:
    Universe(WorldBase& world, Qualifier q)
        : Def(world, Tag::Universe, nullptr, 0, {"□"})
    {
        qualifier_ = q;
    }

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Var : public Def {
private:
    Var(WorldBase& world, const Def* type, size_t index, Debug dbg)
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
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Axiom : public Def {
private:
    /// A @em nominal axiom.
    Axiom(WorldBase& world, const Def* type, Debug dbg)
        : Def(world, Tag::Axiom, type, 0, dbg)
    {
        assert(type->free_vars().none());
    }

    /// A @em structural axiom.
    Axiom(WorldBase& world, const Def* type, Box box, Debug dbg)
        : Def(world, Tag::Axiom, type, Defs(), dbg)
    {
        box_ = box;
    }

public:
    Box box() const { assert(!is_nominal()); return box_; }
    std::ostream& stream(std::ostream&) const override;
    Axiom* stub(WorldBase&, const Def*, Debug) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

class Error : public Def {
private:
    // TODO additional error message with more precise information
    Error(WorldBase& world, const Def* type)
        : Def(world, Tag::Error, type, Defs(), {"<error>"})
    {}

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(WorldBase&, const Def*, Defs) const override;

    friend class WorldBase;
};

inline bool is_error(const Def* def) { return def->tag() == Def::Tag::Error; }

}

#endif
