#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include <numeric>
#include <set>
#include <stack>

#include "thorin/util/array.h"
#include "thorin/util/bitset.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/iterator.h"
#include "thorin/util/location.h"
#include "thorin/util/stream.h"
#include "thorin/tables.h"
#include "thorin/qualifier.h"

namespace thorin {

class Def;
class World;

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

typedef Array<const Def*> DefArray;
typedef ArrayRef<const Def*> Defs;
typedef std::vector<const Def*> DefVector;

typedef std::pair<DefArray, const Def*> EnvDef;

struct EnvDefHash {
    inline static uint64_t hash(const EnvDef&);
    static bool eq(const EnvDef& a, const EnvDef& b) { return a == b; };
    static EnvDef sentinel() { return EnvDef(DefArray(), nullptr); }
};

typedef thorin::HashSet<std::pair<DefArray, const Def*>, EnvDefHash> EnvDefSet;

DefArray types(Defs defs);
DefArray qualifiers(Defs defs);
void gid_sort(DefArray* defs);
DefArray gid_sorted(Defs defs);
void unique_gid_sort(DefArray* defs);
DefArray unique_gid_sorted(Defs defs);

//------------------------------------------------------------------------------

/// Base class for all Def%s.
class Def : public RuntimeCast<Def>, public Streamable  {
public:
    enum class Tag {
        All,
        Any,
        App,
        ArityKind,
        Axiom,
        Error,
        Extract,
        Intersection,
        Insert,
        Lambda,
        Match,
        MultiArityKind,
        Pack,
        Pi,
        Pick,
        Sigma,
        Singleton,
        Star,
        Tuple,
        Universe,
        Var,
        Variadic,
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
    Def(World& world, Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : world_(&world)
        , debug_(dbg)
        , type_(type)
        , num_ops_(num_ops)
        , ops_capacity_(num_ops)
        , gid_(gid_counter_++)
        , tag_(unsigned(tag))
        , closed_(num_ops == 0)
        , nominal_(true)
        , has_error_(false)
        , ops_(ops_ptr())
    {
        std::fill_n(ops_, num_ops, nullptr);
    }
    /// A @em structural Def.
    template<class I>
    Def(World& world, Tag tag, const Def* type, Range<I> ops, Debug dbg)
        : world_(&world)
        , debug_(dbg)
        , type_(type)
        , num_ops_(ops.distance())
        , ops_capacity_(ops.distance())
        , gid_(gid_counter_++)
        , tag_(unsigned(tag))
        , closed_(true)
        , nominal_(false)
        , has_error_(false)
        , ops_(ops_ptr())
    {
        std::copy(ops.begin(), ops.end(), ops_);
    }
    /// A @em structural Def.
    Def(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(world, tag, type, range(ops), dbg)
    {}
    ~Def() override;

    Def* set(size_t i, const Def*);
    void finalize();
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
    bool is_value() const {
        switch (sort()) {
            case Sort::Universe:
            case Sort::Kind: return false;
            case Sort::Type: return type()->has_values();
            case Sort::Term: return true;
        }
        THORIN_UNREACHABLE;
    }
    virtual bool has_values() const { return false; }
    //@}

    //@{ get Qualifier
    const Def* qualifier() const;
    /// The qualifier of values inhabiting either this kind itself or inhabiting types within this kind.
    virtual const Def* kind_qualifier() const;
    bool maybe_affine() const;
    //@}

    //@{ misc getters
    virtual const Def* arity() const;
    const BitSet& free_vars() const { return free_vars_; }
    uint32_t fields() const { return uint32_t(num_ops_) << 8_u32 | uint32_t(tag()); }
    size_t gid() const { return gid_; }
    static size_t gid_counter() { return gid_counter_; }
    /// A nominal Def is always different from each other Def.
    bool is_nominal() const { return nominal_; }
    bool is_closed() const { return closed_; }
    bool has_error() const { return has_error_; }
    Tag tag() const { return Tag(tag_); }
    World& world() const { return *world_; }
    //@}

    void typecheck() const {
        assert(free_vars().none());
        DefVector types;
        EnvDefSet checked;
        typecheck_vars(types, checked);
    }
    virtual void typecheck_vars(DefVector& types, EnvDefSet& checked) const;

    /**
     * Substitutes Var%s beginning from @p index with @p args and shifts free Var%s by the number of @p args.
     * Note that @p args will be indexed in reverse order due to De Bruijn way of counting.
     */
    const Def* reduce(Defs args, size_t index = 0) const;
    const Def* reduce(const Def* arg, size_t index = 0) const { return reduce(Defs{arg}, index); }
    const Def* shift_free_vars(size_t shift) const;
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

    virtual Def* stub(World&, const Def*, Debug) const { THORIN_UNREACHABLE; }
    virtual bool assignable(const Def* def) const {
        return this == def->type();
    }
    std::ostream& qualifier_stream(std::ostream& os) const {
        if (!has_values())
            return os;
        return os << qualifier();
    }
    virtual std::ostream& name_stream(std::ostream& os) const {
        if (name() != "" || is_nominal()) {
            qualifier_stream(os);
            return os << name();
        }
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
        size_t index_;              ///< Used by Var, Arity and Index.
        Box box_;                   ///< Used by Axiom.
    };
    BitSet free_vars_;

private:
    const Def** ops_ptr() { return reinterpret_cast<const Def**>(reinterpret_cast<char*>(this) + sizeof(Def)); }
    virtual const Def* rebuild(World&, const Def*, Defs) const = 0;
    bool on_heap() const { return ops_ != const_cast<Def*>(this)->ops_ptr(); }

    static size_t gid_counter_;

    mutable Uses uses_;
    mutable uint64_t hash_ = 0;
    mutable World* world_;
    mutable Debug debug_;
    const Def* type_;
    uint32_t num_ops_;
    uint32_t ops_capacity_;
    union {
        struct {
            unsigned gid_           : 23;
            unsigned tag_           :  6;
            unsigned closed_        :  1;
            unsigned nominal_       :  1;
            unsigned has_error_     :  1;
            // this sum must be 32   ^^^
        };
        uint32_t fields_;
    };

    static_assert(int(Tag::Num) <= 64, "you must increase the number of bits in tag_");
    const Def** ops_;

    friend class App;
    friend class Cleaner;
    friend class Reducer;
    friend class Scope;
    friend class World;
};

uint64_t UseHash::hash(Use use) {
    return murmur3(uint64_t(use.index()) << 48ull | uint64_t(use->gid()));
}

uint64_t EnvDefHash::hash(const EnvDef& p) {
    uint64_t hash = hash_begin(p.second->gid());
    for (auto def : p.first)
        hash = hash_combine(hash, def->gid());
    return hash;
}

template<class T>
const SortedDefSet set_flatten(Defs defs) {
    SortedDefSet flat_defs;
    for (auto def : defs) {
        if (def->isa<T>())
            for (auto inner : def->ops())
                flat_defs.insert(inner);
        else
            flat_defs.insert(def);
    }
    return flat_defs;
}

//------------------------------------------------------------------------------

class Pi : public Def {
private:
    Pi(World& world, const Def* type, const Def* domain, const Def* body, Debug dbg);

public:
    const Def* arity() const override;
    const Def* domain() const { return op(0); }
    const Def* body() const { return op(1); }
    const Def* reduce(Defs) const;
    bool has_values() const override;
    void typecheck_vars(DefVector&, EnvDefSet& checked) const override;

    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Lambda : public Def {
private:
    Lambda(World& world, const Pi* type, const Def* body, Debug dbg);

public:
    const Def* domain() const { return type()->domain(); }
    const Def* body() const { return op(0); }
    const Def* reduce(Defs) const;
    const Pi* type() const { return Def::type()->as<Pi>(); }
    void typecheck_vars(DefVector&, EnvDefSet& checked) const override;

    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class App : public Def {
private:
    App(World& world, const Def* type, const Def* callee, const Def* arg, Debug dbg)
        : Def(world, Tag::App, type, {callee, arg}, dbg)
    {
        cache_ = nullptr;
    }

public:
    const Def* arity() const;
    const Def* callee() const { return op(0); }
    const Def* arg() const { return op(1); }

    std::ostream& stream(std::ostream&) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    const Def* try_reduce() const;

    friend class World;
};

//------------------------------------------------------------------------------

class SigmaBase : public Def {
protected:
    SigmaBase(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(world, tag, type, ops, dbg)
    {}
    SigmaBase(World& world, Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : Def(world, tag, type, num_ops, dbg)
    {}
};

class Sigma : public SigmaBase {
private:
    /// Nominal Sigma kind
    Sigma(World& world, size_t num_ops, Debug dbg);
    /// Nominal Sigma type, \a type is some Star/Universe
    Sigma(World& world, const Def* type, size_t num_ops, Debug dbg)
        : SigmaBase(world, Tag::Sigma, type, num_ops, dbg)
    {}
    Sigma(World& world, const Def* type, Defs ops, Debug dbg)
        : SigmaBase(world, Tag::Sigma, type, ops, dbg)
    {}

public:
    const Def* arity() const override;
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    bool assignable(const Def* def) const override;
    bool is_unit() const { return ops().empty(); }
    bool is_dependent() const;
    Sigma* set(size_t i, const Def* def) { return Def::set(i, def)->as<Sigma>(); };

    std::ostream& stream(std::ostream&) const override;
    Sigma* stub(World&, const Def*, Debug) const override;
    void typecheck_vars(DefVector&, EnvDefSet& checked) const override;

private:
    static const Def* max_type(World& world, Defs ops, const Def* qualifier);
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Variadic : public SigmaBase {
private:
    Variadic(World& world, const Def* type, const Def* arity, const Def* body, Debug dbg);

public:
    const Def* arity() const override { return op(0); }
    const Def* body() const { return op(1); }
    const Def* kind_qualifier() const override;
    bool is_homogeneous() const { return !body()->free_vars().test(0); };
    bool has_values() const override;
    bool assignable(const Def* def) const override;
    void typecheck_vars(DefVector&, EnvDefSet& checked) const override;
    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class TupleBase : public Def {
protected:
    TupleBase(World& world, Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(world, tag, type, ops, dbg)
    {}
};

class Tuple : public TupleBase {
private:
    Tuple(World& world, const SigmaBase* type, Defs ops, Debug dbg)
        : TupleBase(world, Tag::Tuple, type, ops, dbg)
    {}

public:
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Pack : public TupleBase {
private:
    Pack(World& world, const Def* type, const Def* body, Debug dbg);

public:
    const Def* body() const { return op(0); }
    void typecheck_vars(DefVector&, EnvDefSet& checked) const override;
    std::ostream& stream(std::ostream&) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Extract : public Def {
private:
    Extract(World& world, const Def* type, const Def* tuple, const Def* index, Debug dbg)
        : Def(world, Tag::Extract, type, {tuple, index}, dbg)
    {}

public:
    const Def* scrutinee() const { return op(0); }
    const Def* index() const { return op(1); }
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Insert : public Def {
private:
    Insert(World& world, const Def* type, const Def* tuple, const Def* index, const Def* value, Debug dbg)
        : Def(world, Tag::Insert, type, {tuple, index, value}, dbg)
    {}

public:
    const Def* scrutinee() const { return op(0); }
    const Def* index() const { return op(1); }
    const Def* value() const { return op(2); }
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Intersection : public Def {
private:
    Intersection(World& world, const Def* type, const SortedDefSet& ops, Debug dbg);

public:
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Pick : public Def {
private:
    Pick(World& world, const Def* type, const Def* def, Debug dbg)
        : Def(world, Tag::Pick, type, {def}, dbg)
    {}

public:
    const Def* destructee() const { return op(0); }
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Variant : public Def {
private:
    Variant(World& world, const Def* type, const SortedDefSet& ops, Debug dbg);
    // Nominal Variant
    Variant(World& world, const Def* type, size_t num_ops, Debug dbg)
        : Def(world, Tag::Variant, type, num_ops, dbg)
    {}

public:
    const Def* arity() const override;
    Variant* set(size_t i, const Def* def) { return Def::set(i, def)->as<Variant>(); };
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    Variant* stub(World&, const Def*, Debug) const override;

    friend class World;
};

/// Cast a Def to a Variant type.
class Any : public Def {
private:
    Any(World& world, const Variant* type, const Def* def, Debug dbg)
        : Def(world, Tag::Any, type, {def}, dbg)
    {}

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

class Match : public Def {
private:
    Match(World& world, const Def* type, const Def* def, const Defs handlers, Debug dbg)
        : Def(world, Tag::Match, type, concat(def, handlers), dbg)
    {}

public:
    const Def* destructee() const { return op(0); }
    Defs handlers() const { return ops().skip_front(); }
    const Def* handler(size_t i) const { return handlers()[i]; }
    size_t num_handlers() const { return handlers().size(); }

    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Singleton : public Def {
private:
    Singleton(World& world, const Def* def, Debug dbg)
        : Def(world, Tag::Singleton, def->type()->type(), {def}, dbg)
    {
        assert((def->is_term() || def->is_type()) && "No singleton type universes allowed.");
    }

public:
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    std::ostream& stream(std::ostream&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class ArityKind : public Def {
private:
    ArityKind(World& world, const Def* qualifier);

public:
    const Def* arity() const override;
    std::ostream& stream(std::ostream&) const override;
    const Def* kind_qualifier() const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class MultiArityKind : public Def {
private:
    MultiArityKind(World& world, const Def* qualifier);

public:
    const Def* arity() const override;
    bool assignable(const Def* def) const override;
    std::ostream& stream(std::ostream&) const override;
    const Def* kind_qualifier() const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};


class Star : public Def {
private:
    Star(World& world, const Def* qualifier);

public:
    const Def* arity() const override;
    bool assignable(const Def* def) const override;
    std::ostream& stream(std::ostream&) const override;
    const Def* kind_qualifier() const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Universe : public Def {
private:
    Universe(World& world)
        : Def(world, Tag::Universe, nullptr, 0, {"□"})
    {}

public:
    const Def* arity() const override;
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
        assert(!type->is_universe());
        index_ = index;
        free_vars_.set(index);
    }

public:
    const Def* arity() const override;
    size_t index() const { return index_; }
    std::ostream& stream(std::ostream&) const override;
    /// Do not print variable names as they aren't bound in the output without analysing DeBruijn-Indices.
    std::ostream& name_stream(std::ostream& os) const override { return stream(os); }
    void typecheck_vars(DefVector&, EnvDefSet& checked) const override;

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
    const Def* arity() const override;
    Box box() const { assert(!is_nominal()); return box_; }
    std::ostream& stream(std::ostream&) const override;
    Axiom* stub(World&, const Def*, Debug) const override;
    bool has_values() const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    friend class World;
};

class Error : public Def {
private:
    // TODO additional error message with more precise information
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
