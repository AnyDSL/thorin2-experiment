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
#include "thorin/util/types.h"
#include "thorin/qualifier.h"

namespace thorin {

class App;
class Cn;
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
using DefLt   = GIDLt<const Def*>;
using SortedDefSet = std::set<const Def*, DefLt>;

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

DefArray qualifiers(Defs defs);
void gid_sort(DefArray* defs);
DefArray gid_sorted(Defs defs);
void unique_gid_sort(DefArray* defs);
DefArray unique_gid_sorted(Defs defs);

//------------------------------------------------------------------------------

typedef const Def* (*Normalizer)(World&, const Def*, const Def*, Debug);

/// Base class for all Def%s.
class Def : public RuntimeCast<Def>, public Streamable  {
public:
    enum class Tag {
        Any, Match, Variant,
        App, Lambda, Pi,
        Arity, ArityKind, MultiArityKind,
        Cn, Param, CnType,
        Extract, Insert, Tuple, Pack, Sigma, Variadic,
        Lit, Axiom,
        Pick, Intersection,
        Qualifier, QualifierType,
        Star, Universe,
        Error,
        Singleton,
        Var,
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
    Def(Tag tag, const Def* type, size_t num_ops, const Def** ops_ptr, Debug dbg)
        : debug_(dbg)
        , type_(type)
        , num_ops_(num_ops)
        , gid_(gid_counter_++)
        , tag_(unsigned(tag))
        , nominal_(true)
        , has_error_(false)
        , ops_(ops_ptr)
    {
        std::fill_n(ops_, num_ops, nullptr);
    }
    /// A @em structural Def.
    template<class I>
    Def(Tag tag, const Def* type, Range<I> ops, const Def** ops_ptr, Debug dbg)
        : debug_(dbg)
        , type_(type)
        , num_ops_(ops.distance())
        , gid_(gid_counter_++)
        , tag_(unsigned(tag))
        , nominal_(false)
        , has_error_(false)
        , ops_(ops_ptr)
    {
        std::copy(ops.begin(), ops.end(), ops_);
    }
    /// A @em structural Def.
    Def(Tag tag, const Def* type, Defs ops, const Def** ops_ptr, Debug dbg)
        : Def(tag, type, range(ops), ops_ptr, dbg)
    {}

    Def* set(World&, size_t i, const Def*);
    void finalize(World&);
    void unset(size_t i);
    void unregister_use(size_t i) const;
    void unregister_uses() const;

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

    //@{ get World, type, and Sort
    World& world() const {
        auto def = this;
        for (size_t i = 0; i != 3; ++i, def = def->type_) {
            if (def->has_world())
                return *reinterpret_cast<World*>(def->world_ & uintptr_t(-2));
        }
        assert(def->has_world());
        return *reinterpret_cast<World*>(def->world_ & uintptr_t(-2));
    }
    const Def* type() const { return has_world() ? nullptr : type_; }
    Sort sort() const;
    bool is_term() const { return sort() == Sort::Term; }
    bool is_type() const { return sort() == Sort::Type; }
    bool is_kind() const { return sort() == Sort::Kind; }
    bool is_universe() const { return sort() == Sort::Universe; }
    bool is_value() const;
    virtual bool has_values(World&) const { return false; }
    bool is_qualifier() const { return type() && type()->tag() == Tag::QualifierType; }
    //@}

    //@{ get Qualifier
    const Def* qualifier(World&) const;
    bool maybe_affine(World&) const;
    //@}

    //@{ misc getters
    virtual const Def* arity(World&) const;
    const BitSet& free_vars() const { return free_vars_; }
    uint32_t fields() const { return uint32_t(num_ops_) << 8_u32 | uint32_t(tag()); }
    uint32_t gid() const { return gid_; }
    static uint32_t gid_counter() { return gid_counter_; }
    /// A nominal Def is always different from each other Def.
    bool is_nominal() const { return nominal_; }
    bool has_error() const { return has_error_; }
    Tag tag() const { return Tag(tag_); }
    //@}

    void typecheck(World& world) const {
        assert(free_vars().none());
        DefVector types;
        EnvDefSet checked;
        typecheck_vars(world, types, checked);
    }
    virtual void typecheck_vars(World&, DefVector& types, EnvDefSet& checked) const;

    const Def* reduce(World& world, const Def* arg/*, size_t index = 0*/) const { return reduce(world, Defs{arg}); }
    const Def* reduce(World&, Defs args/*, size_t index = 0*/) const;
    const Def* shift_free_vars(World&, size_t shift) const;
    Def* stub(World& world, const Def* type) const {
        if (!name().empty()) {
            Debug new_dbg(debug());
            new_dbg.set(name() + std::to_string(Def::gid_counter()));
            return stub(world, type, new_dbg);
        }
        return stub(world, type, debug());
    }
    Normalizer normalizer() const { return normalizer_; }
    const Def* set_normalizer(Normalizer normalizer) const { normalizer_ = normalizer; return this; }

    virtual Def* stub(World&, const Def*, Debug) const { THORIN_UNREACHABLE; }
    virtual bool assignable(World&, const Def* def) const { return this == def->type(); }
    bool subtype_of(World& world, const Def* def) const {
        auto s = sort();
        return (!def->is_value() && s >= Sort::Type && (this == def || (s == def->sort() && vsubtype_of(world, def))));
    }

    std::ostream& qualifier_stream(std::ostream& os) const;
    virtual std::ostream& name_stream(std::ostream& os) const {
        if (name() != "" || is_nominal()) {
            qualifier_stream(os);
            return os << name();
        }
        return stream(os);
    }
    std::ostream& stream(std::ostream& os) const {
        if (is_nominal()) {
            qualifier_stream(os);
            return os << name();
        }
        return vstream(os);
    }

protected:
    //@{ hash and equal
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    //@}

private:
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

    virtual const Def* rebuild(World&, const Def*, Defs) const = 0;
    /// The qualifier of values inhabiting either this kind itself or inhabiting types within this kind.
    virtual const Def* kind_qualifier(World&) const;
    virtual bool vsubtype_of(World&, const Def*) const { return false; }
    virtual std::ostream& vstream(std::ostream& os) const = 0;
    bool has_world() const { return world_ & uintptr_t(1); }

    static uint32_t gid_counter_;

protected:
    BitSet free_vars_;

private:
    mutable Uses uses_;
    mutable uint64_t hash_ = 0;
    mutable Debug debug_;
    union {
        const Def* type_;
        uintptr_t world_;
    };
    mutable Normalizer normalizer_ = nullptr;
    uint32_t num_ops_;
    union {
        struct {
            unsigned gid_           : 24;
            unsigned tag_           :  6;
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

#define THORIN_OPS_PTR reinterpret_cast<const Def**>(reinterpret_cast<char*>(this+1))

uint64_t UseHash::hash(Use use) {
    return murmur3(uint64_t(use.index()) << 48_u64 | uint64_t(use->gid()));
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

class ArityKind : public Def {
private:
    ArityKind(World& world, const Def* qualifier);

public:
    const Def* arity(World&) const override;
    std::ostream& name_stream(std::ostream& os) const override { return vstream(os); }
    const Def* kind_qualifier(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    bool vsubtype_of(World&, const Def*) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class MultiArityKind : public Def {
private:
    MultiArityKind(World& world, const Def* qualifier);

public:
    const Def* arity(World&) const override;
    bool assignable(World&, const Def* def) const override;
    std::ostream& name_stream(std::ostream& os) const override { return vstream(os); }
    const Def* kind_qualifier(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    bool vsubtype_of(World&, const Def*) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Arity : public Def {
private:
    Arity(const ArityKind* type, u64 arity, Debug dbg)
        : Def(Tag::Arity, type, Defs(), THORIN_OPS_PTR, dbg)
        , arity_(arity)
    {}

public:
    const ArityKind* type() const { return Def::type()->as<ArityKind>(); }
    u64 value() const { return arity_; }
    const Def* arity(World&) const override;
    bool has_values(World&) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    u64 arity_;

    friend class World;
};

//------------------------------------------------------------------------------

class Pi : public Def {
private:
    Pi(const Def* type, const Def* domain, const Def* body, Debug dbg)
        : Def(Tag::Pi, type, {domain, body}, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* domain() const { return op(0); }
    const Def* body() const { return op(1); }
    const Def* apply(World&, const Def*) const;

    const Def* arity(World&) const override;
    bool assignable(World&, const Def* def) const override;
    bool has_values(World&) const override;
    void typecheck_vars(World&, DefVector&, EnvDefSet& checked) const override;
    const Def* kind_qualifier(World&) const override;


private:
    bool vsubtype_of(World&, const Def* def) const override;
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Lambda : public Def {
private:
    /// @em structural Lambda
    Lambda(const Pi* type, const Def* body, Debug dbg)
        : Def(Tag::Lambda, type, {body}, THORIN_OPS_PTR, dbg)
    {}
    /// @em nominal Lambda
    Lambda(const Pi* type, Debug dbg)
        : Def(Tag::Lambda, type, 1, THORIN_OPS_PTR, dbg)
    {}

public:
    Lambda* set(World& world, const Def* body) { return Def::set(world, 0, body)->as<Lambda>(); }
    const Def* domain() const { return type()->domain(); }
    const Def* body() const { return op(0); }
    const Def* apply(World&, const Def*) const;
    const Pi* type() const { return Def::type()->as<Pi>(); }
    void typecheck_vars(World&, DefVector&, EnvDefSet& checked) const override;
    Lambda* stub(World&, const Def*, Debug) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class App : public Def {
private:
    App(const Def* type, const Def* callee, const Def* arg, Debug dbg)
        : Def(Tag::App, type, {callee, arg}, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* callee() const { return op(0); }
    const Def* arg() const { return op(1); }

    const Def* arity(World&) const override;

    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    mutable const Def* cache_ = nullptr;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

//------------------------------------------------------------------------------

class SigmaBase : public Def {
protected:
    SigmaBase(Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(tag, type, ops, THORIN_OPS_PTR, dbg)
    {}
    SigmaBase(Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : Def(tag, type, num_ops, THORIN_OPS_PTR, dbg)
    {}
};

class Sigma : public SigmaBase {
private:
    /// Nominal Sigma kind
    Sigma(World&, size_t num_ops, Debug dbg);
    /// Nominal Sigma type, \a type is some Star/Universe
    Sigma(const Def* type, size_t num_ops, Debug dbg)
        : SigmaBase(Tag::Sigma, type, num_ops, dbg)
    {}
    Sigma(const Def* type, Defs ops, Debug dbg)
        : SigmaBase(Tag::Sigma, type, ops, dbg)
    {}

public:
    const Def* arity(World&) const override;
    const Def* kind_qualifier(World&) const override;
    bool has_values(World&) const override;
    bool assignable(World&, const Def* def) const override;
    bool is_unit() const { return ops().empty(); }
    bool is_dependent() const;
    Sigma* set(World& world, size_t i, const Def* def) { return Def::set(world, i, def)->as<Sigma>(); };

    Sigma* stub(World&, const Def*, Debug) const override;
    void typecheck_vars(World&, DefVector&, EnvDefSet& checked) const override;

private:
    static const Def* max_type(World& world, Defs ops, const Def* qualifier);
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Variadic : public SigmaBase {
private:
    Variadic(const Def* type, const Def* arity, const Def* body, Debug dbg)
        : SigmaBase(Tag::Variadic, type, {arity, body}, dbg)
    {}

public:
    const Def* arity(World&) const override { return op(0); }
    const Def* body() const { return op(1); }
    const Def* kind_qualifier(World&) const override;
    bool is_homogeneous() const { return !body()->free_vars().test(0); };
    bool has_values(World&) const override;
    bool assignable(World&, const Def* def) const override;
    void typecheck_vars(World&, DefVector&, EnvDefSet& checked) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class TupleBase : public Def {
protected:
    TupleBase(Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(tag, type, ops, THORIN_OPS_PTR, dbg)
    {}
};

class Tuple : public TupleBase {
private:
    Tuple(const SigmaBase* type, Defs ops, Debug dbg)
        : TupleBase(Tag::Tuple, type, ops, dbg)
    {}

    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Pack : public TupleBase {
private:
    Pack(const Def* type, const Def* body, Debug dbg)
        : TupleBase(Tag::Pack, type, {body}, dbg)
        {}

public:
    const Def* body() const { return op(0); }
    void typecheck_vars(World&, DefVector&, EnvDefSet& checked) const override;

private:
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Extract : public Def {
private:
    Extract(const Def* type, const Def* tuple, const Def* index, Debug dbg)
        : Def(Tag::Extract, type, {tuple, index}, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* scrutinee() const { return op(0); }
    const Def* index() const { return op(1); }

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Insert : public Def {
private:
    Insert(const Def* type, const Def* tuple, const Def* index, const Def* value, Debug dbg)
        : Def(Tag::Insert, type, {tuple, index, value}, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* scrutinee() const { return op(0); }
    const Def* index() const { return op(1); }
    const Def* value() const { return op(2); }

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Intersection : public Def {
private:
    Intersection(const Def* type, const SortedDefSet& ops, Debug dbg);

public:
    const Def* kind_qualifier(World&) const override;
    bool has_values(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Pick : public Def {
private:
    Pick(const Def* type, const Def* def, Debug dbg)
        : Def(Tag::Pick, type, {def}, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* destructee() const { return op(0); }

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Variant : public Def {
private:
    /// @em structural Variant
    Variant(const Def* type, const SortedDefSet& ops, Debug dbg);
    /// @em nominal Variant
    Variant(const Def* type, size_t num_ops, Debug dbg)
        : Def(Tag::Variant, type, num_ops, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* arity(World&) const override;
    Variant* set(World& world, size_t i, const Def* def) { return Def::set(world, i, def)->as<Variant>(); };
    const Def* kind_qualifier(World&) const override;
    bool has_values(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;
    Variant* stub(World&, const Def*, Debug) const override;

    friend class World;
};

/// Cast a Def to a Variant type.
class Any : public Def {
private:
    Any(const Variant* type, const Def* def, Debug dbg)
        : Def(Tag::Any, type, {def}, THORIN_OPS_PTR, dbg)
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

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Match : public Def {
private:
    Match(const Def* type, const Def* def, const Defs handlers, Debug dbg)
        : Def(Tag::Match, type, concat(def, handlers), THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* destructee() const { return op(0); }
    Defs handlers() const { return ops().skip_front(); }
    const Def* handler(size_t i) const { return handlers()[i]; }
    size_t num_handlers() const { return handlers().size(); }

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Singleton : public Def {
private:
    Singleton(const Def* def, Debug dbg)
        : Def(Tag::Singleton, def->type()->type(), {def}, THORIN_OPS_PTR, dbg)
    {
        assert((def->is_term() || def->is_type()) && "No singleton type universes allowed.");
    }

public:
    const Def* arity(World&) const override;
    const Def* kind_qualifier(World&) const override;
    bool has_values(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Qualifier : public Def {
private:
    Qualifier(World&, QualifierTag q);

public:
    QualifierTag qualifier_tag() const { return qualifier_tag_; }
    const Def* arity(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    QualifierTag qualifier_tag_;

    friend class World;
};

class QualifierType : public Def {
private:
    QualifierType(World& world);

public:
    const Def* arity(World&) const override;
    bool has_values(World&) const override;
    const Def* kind_qualifier(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world, const Def* qualifier);

public:
    const Def* arity(World&) const override;
    bool assignable(World&, const Def* def) const override;
    std::ostream& name_stream(std::ostream& os) const override { return vstream(os); }
    const Def* kind_qualifier(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Universe : public Def {
private:
    Universe(World& world)
        : Def(Tag::Universe, reinterpret_cast<const Def*>(uintptr_t(&world) | uintptr_t(1)), 0, THORIN_OPS_PTR, {"□"})
    {}

public:
    const Def* arity(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

class Var : public Def {
private:
    Var(const Def* type, u64 index, Debug dbg)
        : Def(Tag::Var, type, Defs(), THORIN_OPS_PTR, dbg)
    {
        assert(!type->is_universe());
        index_ = index;
        free_vars_.set(index);
    }

public:
    const Def* arity(World&) const override;
    u64 index() const { return index_; }
    /// Do not print variable names as they aren't bound in the output without analysing DeBruijn-Indices.
    std::ostream& name_stream(std::ostream& os) const override { return vstream(os); }
    void typecheck_vars(World&, DefVector&, EnvDefSet& checked) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    u64 index_;

    friend class World;
};

class Axiom : public Def {
private:
    Axiom(const Def* type, Debug dbg)
        : Def(Tag::Axiom, type, 0, THORIN_OPS_PTR, dbg)
    {
        assert(type->free_vars().none());
    }

public:
    const Def* arity(World&) const override;
    Axiom* stub(World&, const Def*, Debug) const override;
    bool has_values(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

// TODO remember which field in the box was actually used to have a better output
class Lit : public Def {
private:
    Lit(const Def* type, Box box, Debug dbg)
        : Def(Tag::Axiom, type, Defs(), THORIN_OPS_PTR, dbg)
        , box_(box)
    {}

public:
    const Def* arity(World&) const override;
    Box box() const { return box_; }

    bool has_values(World&) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    Box box_;

    friend class World;
};

#define CODE(T) inline T get_ ## T(const Def* def) { return def->as<Lit>()->box().get_ ## T(); }
THORIN_TYPES(CODE)
#undef CODE
inline s64 get_nat(const Def* def) { return get_s64(def); }
inline u64 get_index(const Def* def) { return get_u64(def); }

inline bool is_zero  (const Lit* lit) { return lit->box().get_u64() == 0_u64; }
inline bool is_one   (const Lit* lit) { return lit->box().get_u64() == 1_u64; }
inline bool is_allset(const Lit* lit) { return lit->box().get_u64() == u64(-1); }

/// Is @p lit an IEEE-754 zero? yields also true for negative zero.
inline bool is_rzero(int64_t w, const Lit* lit) { return (lit->box().get_u64() & ~(1 << (w-1))) == 0; }

inline bool is_one  (int64_t w, const Lit* lit) {
    switch (w) {
        case 16: return lit->box().get_r16() == 1._r16;
        case 32: return lit->box().get_r32() == 1._r32;
        case 64: return lit->box().get_r64() == 1._r64;
        default: THORIN_UNREACHABLE;
    }
}

class Error : public Def {
private:
    // TODO additional error message with more precise information
    Error(const Def* type)
        : Def(Tag::Error, type, Defs(), THORIN_OPS_PTR, {"<error>"})
    {}

public:
    const Def* arity(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

inline bool is_error(const Def* def) { return def->tag() == Def::Tag::Error; }

//------------------------------------------------------------------------------

/// Type type of a continuation @p Cn.
class CnType : public Def {
private:
    CnType(const Def* type, const Def* domain, Debug dbg)
        : Def(Tag::CnType, type, {domain}, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* domain() const { return op(0); }

    const Def* arity(World&) const override;
    bool assignable(World&, const Def* def) const override;
    bool has_values(World&) const override;
    const Def* kind_qualifier(World&) const override;

private:
    //bool vsubtype_of(World&, const Def* def) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    friend class World;
};

/// The @p Param%eter associated to a continuation @p Cn.
class Param : public Def {
private:
    Param(const Def* type, Debug dbg)
        : Def(Tag::Param, type, 0, THORIN_OPS_PTR, dbg)
    {}

public:
    const Cn* cn() const { return cn_; }
    const Def* arity(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    std::ostream& vstream(std::ostream&) const override;

    const Cn* cn_;

    friend class World;
};

/// A continuation value.
class Cn : public Def {
private:
    Cn(const Def* type, Debug dbg)
        : Def(Tag::Cn, type, 3, THORIN_OPS_PTR, dbg)
    {}

public:
    const Def* filter() const { return op(0); }
    const Def* callee() const { return op(1); }
    const Def* arg() const { return op(2); }
    const CnType* type() const { return type()->as<CnType>(); }
    const Param* param() const { return param_; }

    const Def* arity(World&) const override;

private:
    const Def* rebuild(World&, const Def*, Defs) const override;
    Cn* stub(World& to, const Def* type, Debug dbg) const override;
    std::ostream& vstream(std::ostream&) const override;

    const Param* param_;

    friend class World;
};

//------------------------------------------------------------------------------

}

#endif
