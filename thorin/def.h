#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include <set>

#include "thorin/util/array.h"
#include "thorin/util/bitset.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/iterator.h"
#include "thorin/util/debug.h"
#include "thorin/util/types.h"
#include "thorin/print.h"
#include "thorin/qualifier.h"

namespace thorin {

class App;
class Axiom;
class Def;
class Lambda;
class TypeCheck;
class Tracker;
class Param;
class World;

typedef const Def* (*Normalizer)(const Def*, const Def*, Debug);
const Axiom* get_axiom(const Def* def);

//------------------------------------------------------------------------------

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

struct UseHash {
    inline static uint64_t hash(Use use);
    static bool eq(Use u1, Use u2) { return u1 == u2; }
    static Use sentinel() { return Use((const Def*)(-1), uint16_t(-1)); }
};

typedef HashSet<Use, UseHash> Uses;

//------------------------------------------------------------------------------

template<class T>
struct GIDLt {
    bool operator()(T a, T b) const { return a->gid() < b->gid(); }
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

typedef TaggedPtr<const Def, size_t> DefIndex;

struct DefIndexHash {
    static uint64_t hash(DefIndex s);
    static bool eq(DefIndex a, DefIndex b) { return a == b; }
    static DefIndex sentinel() { return DefIndex(nullptr, 0); }
};

typedef HashSet<DefIndex, DefIndexHash> DefIndexSet;

template<class To>
using DefIndexMap = HashMap<DefIndex, To, DefIndexHash>;

typedef Array<const Def*> DefArray;
typedef ArrayRef<const Def*> Defs;
typedef std::vector<const Def*> DefVector;

DefArray qualifiers(Defs defs);
void gid_sort(DefArray* defs);
DefArray gid_sorted(Defs defs);
void unique_gid_sort(DefArray* defs);
DefArray unique_gid_sorted(Defs defs);

//------------------------------------------------------------------------------

/**
 * Base class for all Def%s.
 * The data layout (see World::alloc) looks like this:
\verbatim
|| Def | op(0) ... op(num_ops-1) | Extra ||
\endverbatim
 * This means that any subclass of @p Def must not introduce additional members.
 * However, you can have this Extra field.
 * See App or Lit how this is done.
 */
class Def : public RuntimeCast<Def>, public Streamable<Printer> {
public:
    enum class Tag {
        Match, Variant,
        App, Lambda, Param, Pi,
        Arity, ArityKind, MultiArityKind,
        Extract, Insert, Tuple, Pack, Sigma, Variadic,
        Lit, Axiom,
        Pick, Intersection,
        Qualifier, QualifierType,
        Star, Universe,
        Bottom, Top,
        Singleton,
        Unknown,
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
    Def(Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : debug_(dbg)
        , type_(type)
        , num_ops_(num_ops)
        , gid_(gid_counter_++)
        , tag_(unsigned(tag))
        , nominal_(true)
        , contains_lambda_(false)
        , is_dependent_(false)
    {
        std::fill_n(ops_ptr(), num_ops, nullptr);
    }
    /// A @em structural Def.
    template<class I>
    Def(Tag tag, const Def* type, Range<I> ops, Debug dbg)
        : debug_(dbg)
        , type_(type)
        , num_ops_(ops.distance())
        , gid_(gid_counter_++)
        , tag_(unsigned(tag))
        , nominal_(false)
        , contains_lambda_(false)
        , is_dependent_(false)
    {
        std::copy(ops.begin(), ops.end(), ops_ptr());
    }
    /// A @em structural Def.
    Def(Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(tag, type, range(ops), dbg)
    {}

    void finalize();
    void unset(size_t i);
    void unregister_use(size_t i) const;
    void unregister_uses() const;

public:
    //@{ get/set operands
    Defs ops() const { return Defs(ops_ptr() , num_ops_); }
    const Def* op(size_t i) const { return ops()[i]; }
    size_t num_ops() const { return num_ops_; }
    bool empty() const { return num_ops() == 0; }
    /// Sets the @c i'th @p op to @p def.
    /// @attention { You *must* set the last operand last. This will invoke @p finalize. }
    Def* set(size_t i, const Def* def); // TODO only have one setter
    Def* set(Defs);
    //@}

    //@{ get Uses%s
    const Uses& uses() const { return uses_; }
    size_t num_uses() const { return uses().size(); }
    Array<Use> copy_uses() const { return Array<Use>(uses_.begin(), uses_.end()); }
    //@}

    //@{ get Debug information
    Debug& debug() const { return debug_; }
    /// In Debug build if World::enable_history is true, this thing keeps the gid to track a history of gid%s.
    Debug debug_history() const;
    Loc loc() const { return debug_; }
    Symbol name() const { return debug().name(); }
    std::string unique_name() const;
    //@}

    //@{ get World, type, and Sort
    World& world() const {
        auto has_world = [](Tag tag) { return tag == Tag::Universe || tag == Tag::Unknown; };
        if (has_world(tag())) return *world_;
        if (has_world(type()->tag())) return *type()->world_;
        if (has_world(type()->type()->tag())) return *type()->type()->world_;
        assert(has_world(type()->type()->type()->tag()));
        return *type()->type()->type()->world_;
    }
    const Def* type() const { assert(tag() != Tag::Universe); return type_; }
    const Def* destructing_type() const;
    Sort sort() const;
    bool is_term() const { return sort() == Sort::Term; }
    bool is_type() const { return sort() == Sort::Type; }
    bool is_kind() const { return sort() == Sort::Kind; }
    bool is_universe() const { return sort() == Sort::Universe; }
    bool is_value() const;
    virtual bool has_values() const { return false; }
    bool is_qualifier() const { return type() && type()->tag() == Tag::QualifierType; }
    //@}

    //@{ get Qualifier
    const Def* qualifier() const;
    bool maybe_affine() const;
    bool is_substructural() const;
    //@}

    //@{ misc getters
    virtual const Def* arity() const;
    std::optional<u64> has_constant_arity() const;
    const BitSet& free_vars() const { return free_vars_; }
    uint32_t fields() const { return uint32_t(num_ops_) << 8_u32 | uint32_t(tag()); }
    uint32_t gid() const { return gid_; }
    static uint32_t gid_counter() { return gid_counter_; }
    /// A nominal Def is always different from each other Def.
    bool is_nominal() const { return nominal_; }
    Tag tag() const { return Tag(tag_); }
    bool is_dependent() const { return is_dependent_; }
    //@}

    //@{ type checking
    void check() const;
    virtual void check(TypeCheck&, DefVector&) const;
    virtual bool assignable(const Def* def) const;
    bool subtype_of(const Def* def) const;
    //@}

    //@{ Lambda-related stuff
    Lambda* isa_lambda() const;
    Lambda* as_lambda() const;
    bool contains_lambda() const { return contains_lambda_; }
    //@}

    //@{ stuff to rebuild a Def
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
    virtual Def* vstub(World&, const Def*, Debug) const { THORIN_UNREACHABLE; }
    Def* stub(World& world, const Def* type, Debug dbg) const { return vstub(world, type, dbg); }
    Def* stub(World& world, const Def* type) const { return vstub(world, type, debug_history()); }
    Def* stub(const Def* type) const { return vstub(world(), type, debug_history()); }
    //@}

    //@{ replace
    void replace(Tracker) const;
    bool is_replaced() const { return substitute_ != nullptr; }
    //@}

    //@{ stream
    virtual Printer& name_stream(Printer&) const;
    Printer& qualifier_stream(Printer&) const;
    Printer& stream(Printer&) const override;
    std::ostream& stream_out(std::ostream&) const override;
    //@}

protected:
    //@{ hash and equal
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    //@}

    char* extra_ptr() { return reinterpret_cast<char*>(this) + sizeof(Def) + sizeof(const Def*)*num_ops(); }
    const char* extra_ptr() const { return const_cast<Def*>(this)->extra_ptr(); }

private:
    const Def** ops_ptr() const { return reinterpret_cast<const Def**>(reinterpret_cast<char*>(const_cast<Def*>(this)) + sizeof(Def)); }
    /// The qualifier of values inhabiting either this kind itself or inhabiting types within this kind.
    virtual const Def* kind_qualifier() const;
    virtual bool vsubtype_of(const Def*) const { return false; }
    virtual Printer& vstream(Printer& os) const = 0;

    static uint32_t gid_counter_;

protected:
    BitSet free_vars_;

private:
    struct Extra {};

    mutable Uses uses_;
    mutable uint64_t hash_ = 0;
    mutable Debug debug_;
    union {
        const Def* type_;
        mutable World* world_;
    };
    mutable const Def* substitute_ = nullptr;
    uint32_t num_ops_;
    union {
        struct {
            unsigned gid_             : 23;
            unsigned tag_             :  6;
            unsigned nominal_         :  1;
            unsigned contains_lambda_ :  1;
            unsigned is_dependent_    :  1;
            // this sum must be 32  ^^^
        };
    };

    static_assert(int(Tag::Num) <= 64, "you must increase the number of bits in tag_");

    friend class App;
    friend class Tracker;
    friend class World;
    friend void swap(World&, World&);
};

class Tracker {
public:
    Tracker()
        : def_(nullptr)
    {}
    Tracker(const Def* def)
        : def_(def)
    {}

    operator const Def*() { return def(); }
    const Def* operator->() { return def(); }
    const Def* def() {
        if (def_ != nullptr) {
            while (auto repr = def_->substitute_)
                def_ = repr;
        }
        return def_;
    }

private:
    const Def* def_;
};

uint64_t UseHash::hash(Use use) {
    return murmur3(uint64_t(use.index()) << 48_u64 | uint64_t(use->gid()));
}

//------------------------------------------------------------------------------

class ArityKind : public Def {
private:
    ArityKind(World& world, const Def* qualifier);

public:
    const Def* arity() const override;
    Printer& name_stream(Printer& os) const override { return vstream(os); }
    const Def* kind_qualifier() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    bool vsubtype_of(const Def*) const override;
    Printer& vstream(Printer&) const override;

    friend class World;
};

class MultiArityKind : public Def {
private:
    MultiArityKind(World& world, const Def* qualifier);

public:
    const Def* arity() const override;
    Printer& name_stream(Printer& os) const override { return vstream(os); }
    const Def* kind_qualifier() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    bool vsubtype_of(const Def*) const override;
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Arity : public Def {
private:
    struct Extra { u64 arity_; };

    Arity(const ArityKind* type, u64 arity, Debug dbg)
        : Def(Tag::Arity, type, Defs(), dbg)
    {
        extra().arity_ = arity;
    }

public:
    const ArityKind* type() const { return Def::type()->as<ArityKind>(); }
    u64 value() const { return extra().arity_; }
    const Def* arity() const override;
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    Printer& vstream(Printer&) const override;
    Extra& extra() { return reinterpret_cast<Extra&>(*extra_ptr()); }
    const Extra& extra() const { return reinterpret_cast<const Extra&>(*extra_ptr()); }

    friend class World;
};

//------------------------------------------------------------------------------

class Pi : public Def {
private:
    Pi(const Def* type, const Def* domain, const Def* body, Debug dbg)
        : Def(Tag::Pi, type, {domain, body}, dbg)
    {}

public:
    const Def* domain() const { return op(0); }
    const Def* codomain() const { return op(1); }
    const Def* apply(const Def*) const;

    const Def* arity() const override;
    bool has_values() const override;
    void check(TypeCheck&, DefVector&) const override;
    const Def* kind_qualifier() const override;
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;


private:
    bool vsubtype_of(const Def* def) const override;
    Printer& vstream(Printer&) const override;

    friend class World;
};

typedef std::vector<Lambda*> Lambdas;

class Lambda : public Def {
private:
    /// @em structural Lambda
    Lambda(const Pi* type, const Def* filter, const Def* body, Debug dbg)
        : Def(Tag::Lambda, type, {filter, body}, dbg)
    {}
    /// @em nominal Lambda
    Lambda(const Pi* type, Debug dbg)
        : Def(Tag::Lambda, type, 2, dbg)
    {}

public:
    //@{ getters
    const Def* filter() const { return op(0); }
    const Def* body() const { return op(1); }
    const Pi* type() const { return Def::type()->as<Pi>(); }
    const Def* domain() const { return type()->domain(); }
    const Def* codomain() const { return type()->codomain(); }
    /**
     * Since @p Param%s are @em structural, this getter simply creates a @p Param with itself as operand.
     * Due to hash-consing there will only be maximal one @p Param object.
     */
    const Param* param(Debug dbg = {}) const;
    /// @p Extract%s the @c i th element from @p Param.
    const Def* param(u64 i, Debug dbg = {}) const;
    //@}

    //@{ set body -- nominal Lambda%s only
    Lambda* set(const Def* filter, const Def* body) { return Def::set(0, filter)->set(1, body)->as<Lambda>(); }
    /// Uses @c false as filter.
    Lambda* set(const Def* body);
    Lambda* jump(const Def* callee, const Def* arg, Debug dbg = {});
    Lambda* jump(const Def* callee, Defs args, Debug dbg = {});
    Lambda* br(const Def* cond, const Def* t, const Def* f, Debug dbg = {});
    //@}

    void check(TypeCheck&, DefVector&) const override;
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    Lambda* vstub(World&, const Def*, Debug) const override;
    const Def* apply(const Def*) const;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

template<class To>
using LambdaMap = GIDMap<Lambda*, To>;
using LambdaSet = GIDSet<Lambda*>;
using Lambda2Lambda = LambdaMap<Lambda*>;

//------------------------------------------------------------------------------

class SigmaBase : public Def {
protected:
    SigmaBase(Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(tag, type, ops, dbg)
    {}
    SigmaBase(Tag tag, const Def* type, size_t num_ops, Debug dbg)
        : Def(tag, type, num_ops, dbg)
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
    const Def* arity() const override;
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    bool assignable(const Def* def) const override;
    bool is_unit() const { return ops().empty(); }
    Sigma* set(size_t i, const Def* def) { return Def::set(i, def)->as<Sigma>(); };
    Sigma* set(Defs defs) { return Def::set(defs)->as<Sigma>(); }

    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    Sigma* vstub(World&, const Def*, Debug) const override;
    void check(TypeCheck&, DefVector&) const override;

private:
    static const Def* max_type(Defs ops, const Def* qualifier);
    bool vsubtype_of(const Def* def) const override;
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Variadic : public SigmaBase {
private:
    Variadic(const Def* type, const Def* arity, const Def* body, Debug dbg)
        : SigmaBase(Tag::Variadic, type, {arity, body}, dbg)
    {}

public:
    const Def* arity() const override { return op(0); }
    const Def* body() const { return op(1); }
    const Def* kind_qualifier() const override;
    bool is_homogeneous() const { return !body()->free_vars().test(0); };
    bool has_values() const override;
    bool assignable(const Def* def) const override;
    void check(TypeCheck&, DefVector&) const override;
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class TupleBase : public Def {
protected:
    TupleBase(Tag tag, const Def* type, Defs ops, Debug dbg)
        : Def(tag, type, ops, dbg)
    {}
};

class Tuple : public TupleBase {
private:
    Tuple(const SigmaBase* type, Defs ops, Debug dbg)
        : TupleBase(Tag::Tuple, type, ops, dbg)
    {}

public:
    void check(TypeCheck&, DefVector&) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Pack : public TupleBase {
private:
    Pack(const Def* type, const Def* body, Debug dbg)
        : TupleBase(Tag::Pack, type, {body}, dbg)
        {}

public:
    const Def* body() const { return op(0); }
    void check(TypeCheck&, DefVector&) const override;
    size_t shift(size_t) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Extract : public Def {
private:
    Extract(const Def* type, const Def* tuple, const Def* index, Debug dbg)
        : Def(Tag::Extract, type, {tuple, index}, dbg)
    {}

public:
    const Def* scrutinee() const { return op(0); }
    const Def* index() const { return op(1); }
    void check(TypeCheck&, DefVector&) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Insert : public Def {
private:
    Insert(const Def* type, const Def* tuple, const Def* index, const Def* value, Debug dbg)
        : Def(Tag::Insert, type, {tuple, index, value}, dbg)
    {}

public:
    const Def* scrutinee() const { return op(0); }
    const Def* index() const { return op(1); }
    const Def* value() const { return op(2); }
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Intersection : public Def {
private:
    /// @em structural Intersection
    Intersection(const Def* type, const SortedDefSet& ops, Debug dbg)
        : Def(Tag::Intersection, type, range(ops), dbg)
    {}
    /// @em nominal Intersection
    Intersection(const Def* type, size_t num_ops, Debug dbg)
        : Def(Tag::Intersection, type, num_ops, dbg)
    {}

public:
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

    struct Qualifier {
        static constexpr auto min = QualifierTag::l;
        static constexpr auto max = QualifierTag::u;
        static constexpr auto join = glb;
    };
    static constexpr auto op_name = "intersection";
private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Variant : public Def {
private:
    /// @em structural Variant
    Variant(const Def* type, const SortedDefSet& ops, Debug dbg)
        : Def(Tag::Variant, type, range(ops), dbg)
    {}
    /// @em nominal Variant
    Variant(const Def* type, size_t num_ops, Debug dbg)
        : Def(Tag::Variant, type, num_ops, dbg)
    {}

public:
    const Def* arity() const override;
    bool contains(const Def* def) const;
    Variant* set(size_t i, const Def* def) { return Def::set(i, def)->as<Variant>(); };
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    Variant* vstub(World&, const Def*, Debug) const override;

    struct Qualifier {
        static constexpr auto min = QualifierTag::u;
        static constexpr auto max = QualifierTag::l;
        static constexpr auto join = lub;
    };
    static constexpr auto op_name = "variant";
private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Pick : public Def {
private:
    Pick(const Def* type, const Def* def, Debug dbg)
        : Def(Tag::Pick, type, {def}, dbg)
    {}

public:
    const Def* destructee() const { return op(0); }
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Match : public Def {
private:
    Match(const Def* type, const Def* def, const Defs handlers, Debug dbg)
        : Def(Tag::Match, type, concat(def, handlers), dbg)
    {}

public:
    const Def* destructee() const { return op(0); }
    Defs handlers() const { return ops().skip_front(); }
    const Def* handler(size_t i) const { return handlers()[i]; }
    size_t num_handlers() const { return handlers().size(); }
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Singleton : public Def {
private:
    Singleton(const Def* def, Debug dbg)
        : Def(Tag::Singleton, def->type()->type(), {def}, dbg)
    {
        assert((def->is_term() || def->is_type()) && "No singleton type universes allowed.");
    }

public:
    const Def* arity() const override;
    const Def* kind_qualifier() const override;
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Qualifier : public Def {
private:
    struct Extra { QualifierTag qualifier_tag_; };

    Qualifier(World&, QualifierTag q);

public:
    QualifierTag qualifier_tag() const { return extra().qualifier_tag_; }
    const Def* arity() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;
    Extra& extra() { return reinterpret_cast<Extra&>(*extra_ptr()); }
    const Extra& extra() const { return reinterpret_cast<const Extra&>(*extra_ptr()); }

    friend class World;
};

class QualifierType : public Def {
private:
    QualifierType(World& world);

public:
    const Def* arity() const override;
    bool has_values() const override;
    const Def* kind_qualifier() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world, const Def* qualifier);

public:
    const Def* arity() const override;
    bool assignable(const Def* def) const override;
    Printer& name_stream(Printer& os) const override { return vstream(os); }
    const Def* kind_qualifier() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Universe : public Def {
private:
    Universe(World& world)
        : Def(Tag::Universe, reinterpret_cast<const Def*>(&world), 0, {"□"})
    {}

public:
    const Def* arity() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Var : public Def {
private:
    struct Extra { u64 index_; };

    Var(const Def* type, u64 index, Debug dbg)
        : Def(Tag::Var, type, Defs(), dbg)
    {
        assert(!type->is_universe());
        extra().index_ = index;
        free_vars_.set(index);
    }

public:
    const Def* arity() const override;
    bool has_values() const override;
    u64 index() const { return extra().index_; }
    /// Do not print variable names as they aren't bound in the output without analysing DeBruijn-Indices.
    Printer& name_stream(Printer& os) const override { return vstream(os); }
    void check(TypeCheck&, DefVector&) const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    Printer& vstream(Printer&) const override;
    Extra& extra() { return reinterpret_cast<Extra&>(*extra_ptr()); }
    const Extra& extra() const { return reinterpret_cast<const Extra&>(*extra_ptr()); }

    friend class World;
};

// TODO remember which field in the box was actually used to have a better output
class Lit : public Def {
private:
    struct Extra { Box box_; };

    Lit(const Def* type, Box box, Debug dbg)
        : Def(Tag::Lit, type, Defs(), dbg)
    {
        extra().box_ = box;
    }

public:
    const Def* arity() const override;
    Box box() const { return extra().box_; }
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    uint64_t vhash() const override;
    bool equal(const Def*) const override;
    Printer& vstream(Printer&) const override;
    Extra& extra() { return reinterpret_cast<Extra&>(*extra_ptr()); }
    const Extra& extra() const { return reinterpret_cast<const Extra&>(*extra_ptr()); }

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
inline bool is_fzero(int64_t w, const Lit* lit) {
    return (lit->box().get_u64() & ~(1_u64 << (u64(w)-1_u64))) == 0_u64;
}

class Bottom : public Def {
private:
    Bottom(const Def* type)
        : Def(Tag::Bottom, type, Defs(), {"⊥"})
    {}

public:
    const Def* arity() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

class Top : public Def {
private:
    Top(const Def* type)
        : Def(Tag::Top, type, Defs(), {"⊤"})
    {}

public:
    const Def* arity() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

//------------------------------------------------------------------------------

/**
 * The @p Param%eter associated to a @em nominal @p Lambda.
 * The parameter has a single operands: its associated @p Lambda.
 * This way parameters are actualy @em structural.
 */
class Param : public Def {
private:
    Param(const Def* type, const Lambda* lambda, Debug dbg)
        : Def(Tag::Param, type, Defs{lambda}, dbg)
    {
        assert(lambda->is_nominal());
    }

public:
    Lambda* lambda() const { return const_cast<Lambda*>(op(0)->as<Lambda>()); }
    const Def* arity() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;

    friend class World;
};

template<class To>
using ParamMap    = GIDMap<const Param*, To>;
using ParamSet    = GIDSet<const Param*>;
using Param2Param = ParamMap<const Param*>;

//------------------------------------------------------------------------------

class Axiom : public Def {
private:
    struct Extra { Normalizer normalizer_; };

    Axiom(const Def* type, Normalizer normalizer, Debug dbg)
        : Def(Tag::Axiom, type, 0, dbg)
    {
        extra().normalizer_ = normalizer;
        assert(type->free_vars().none());
    }

public:
    Normalizer normalizer() const { return extra().normalizer_; }

    const Def* arity() const override;
    Axiom* vstub(World&, const Def*, Debug) const override;
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Printer& vstream(Printer&) const override;
    Extra& extra() { return reinterpret_cast<Extra&>(*extra_ptr()); }
    const Extra& extra() const { return reinterpret_cast<const Extra&>(*extra_ptr()); }

    friend class World;
};

class App : public Def {
private:
    struct Extra {
        mutable TaggedPtr<const Def, bool> cache_; // true if Axiom
    };

    App(const Def* type, const Def* callee, const Def* arg, Debug dbg)
        : Def(Tag::App, type, {callee, arg}, dbg)
    {
        if (auto axiom = get_axiom(callee); axiom && type->isa<Pi>())
            extra().cache_ = TaggedPtr<const Def, bool>(axiom, true);
        else
            extra().cache_ = TaggedPtr<const Def, bool>(nullptr, false);
    }

public:
    const Def* callee() const { return op(0); }
    const Def* arg() const { return op(1); }
    bool has_axiom() const { return extra().cache_.index(); }
    const Axiom* axiom() const { assert(has_axiom()); return extra().cache_->as<Axiom>(); }

    /**
     * Forces an unfold of this App if possible - may diverge.
     * Also unfolds recursively @p App%s that occur in callee position.
     */
    const Def* unfold() const;

    const Def* arity() const override;
    bool has_values() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;

private:
    Extra& extra() { return reinterpret_cast<Extra&>(*extra_ptr()); }
    const Extra& extra() const { return reinterpret_cast<const Extra&>(*extra_ptr()); }
    const Def* cache() const { assert(!has_axiom()); return extra().cache_; }
    Printer& vstream(Printer&) const override;

    friend const Axiom* get_axiom(const Def*);
    friend class World;
};

/// A value that is unkown during IR construction.
/// Thorin tries to infer this value later on in a dedicated pass.
class Unknown : public Def {
private:
    Unknown(Debug dbg, World& world)
        : Def(Tag::Unknown, reinterpret_cast<const Def*>(&world), 0, dbg)
    {}

public:
    const Def* arity() const override;
    const Def* rebuild(World&, const Def*, Defs) const override;
    Unknown* vstub(World&, const Def*, Debug) const override;
    Printer& vstream(Printer&) const override;

    friend class World;
};

//------------------------------------------------------------------------------

}

#endif
