#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include <numeric>

#include "thorin/util/array.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"
#include "thorin/util/types.h"

namespace thorin {

enum {
    Node_All,
    Node_Any,
    Node_App,
    Node_Assume,
    Node_Error,
    Node_Extract,
    Node_Intersection,
    Node_Lambda,
    Node_Match,
    Node_Pi,
    Node_Pick,
    Node_Sigma,
    Node_Star,
    Node_Tuple,
    Node_Universe,
    Node_Var,
    Node_Variant,
    Num_Nodes
};

static_assert(Num_Nodes <= 32, "you must increase the number of bits in Def::tag_");

class Def;
class World;

/**
 * References a user.
 * A @p Def @c u which uses @p Def @c d as @c i^th operand is a @p Use with @p index_ @c i of @p Def @c d.
 */
class Use {
public:
    Use() {}
#if defined(__x86_64__) || (_M_X64)
    Use(size_t index, const Def* def)
        : uptr_(reinterpret_cast<uintptr_t>(def) | (uintptr_t(index) << 48ull))
    {}

    size_t index() const { return uptr_ >> 48ull; }
    const Def* def() const {
        // sign extend to make pointer canonical
        return reinterpret_cast<const Def*>((iptr_  << 16) >> 16) ;
    }
#else
    Use(size_t index, const Def* def)
        : index_(index)
        , def_(def)
    {}

    size_t index() const { return index_; }
    const Def* def() const { return def_; }
#endif
    operator const Def*() const { return def(); }
    const Def* operator->() const { return def(); }
    bool operator==(Use other) const { return this->def() == other.def() && this->index() == other.index(); }

private:
#if defined(__x86_64__) || (_M_X64)
    /// A tagged pointer: first 16 bits is index, remaining 48 bits is the actual pointer.
    union {
        uintptr_t uptr_;
        intptr_t iptr_;
    };
#else
    size_t index_;
    const Def* def_;
#endif
};

//------------------------------------------------------------------------------

struct UseHash {
    inline static uint64_t hash(Use use);
    static bool eq(Use u1, Use u2) { return u1 == u2; }
    static Use sentinel() { return Use(size_t(-1), (const Def*)(-1)); }
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
Array<const Def*> gid_sorted(Defs defs);

//------------------------------------------------------------------------------

namespace Qualifier {
    enum URAL {
        Unrestricted,
        Affine   = 1 << 0,
        Relevant = 1 << 1,
        Linear = Affine | Relevant,
    };

    bool operator==(URAL lhs, URAL rhs);
    bool operator<(URAL lhs, URAL rhs);
    bool operator<=(URAL lhs, URAL rhs);

    std::ostream& operator<<(std::ostream& ostream, const URAL q);

    URAL meet(URAL lhs, URAL rhs);
    URAL meet(const Defs& defs);
}

//------------------------------------------------------------------------------

/// Base class for all @p Def%s.
class Def : public MagicCast<Def>, public Streamable  {
public:
    enum Sort {
        Term, Type, Kind, TypeUniverse
    };

protected:
    Def(const Def&) = delete;
    Def& operator=(const Def&) = delete;

    /// Use for nominal @p Def%s.
    Def(World& world, unsigned tag, const Def* type, size_t num_ops, const std::string& name)
        : name_(name)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , ops_capacity_(num_ops)
        , tag_(tag)
        , nominal_(true)
        , num_ops_(num_ops)
        , ops_(&vla_ops_[0])
    {}

    /// Use for structural @p Def%s.
    Def(World& world, unsigned tag, const Def* type, Defs ops, const std::string& name)
        : name_(name)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , ops_capacity_(ops.size())
        , tag_(tag)
        , nominal_(false)
        , num_ops_(ops.size())
        , ops_(&vla_ops_[0])
    {
        std::copy(ops.begin(), ops.end(), ops_);
    }

    ~Def() override;

    void set(size_t i, const Def*);
    void wire_uses() const;
    void unset(size_t i);
    void unregister_use(size_t i) const;
    void unregister_uses() const;
    void resize(size_t num_ops);

public:
    Sort sort() const;
    unsigned tag() const { return tag_; }
    World& world() const { return *world_; }
    Defs ops() const { return Defs(ops_, num_ops_); }
    const Def* op(size_t i) const { return ops()[i]; }
    size_t num_ops() const { return num_ops_; }
    const Uses& uses() const { return uses_; }
    size_t num_uses() const { return uses().size(); }
    const Def* type() const { return type_; }
    const std::string& name() const { return name_; }
    std::string unique_name() const;
    void replace(const Def*) const;
    /// A nominal @p Def is always different from each other @p Def.
    bool is_nominal() const { return nominal_; }
    bool is_universe() const { return sort() == TypeUniverse; }
    bool is_kind() const { return sort() == Kind; }
    bool is_type() const { return sort() == Type; }
    bool is_term() const { return sort() == Term; }

    Qualifier::URAL qualifier() const { return type() ? type()->qualifier() : Qualifier::URAL(qualifier_); }
    bool is_unrestricted() const { return qualifier() & Qualifier::Unrestricted; }
    bool is_affine() const       { return qualifier() & Qualifier::Affine; }
    bool is_relevant() const     { return qualifier() & Qualifier::Relevant; }
    bool is_linear() const       { return qualifier() & Qualifier::Linear; }

    size_t gid() const { return gid_; }
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }

    const Def* substitute(Def2Def&, size_t, Defs) const;
    const Def* rebuild(const Def* type, Defs defs) const { return rebuild(world(), type, defs); }

    static size_t gid_counter() { return gid_counter_; }

    virtual bool maybe_dependent() const { return true; }
    virtual std::ostream& name_stream(std::ostream& os) const {
        if (name() != "")
            return os << qualifier() << name();
        return stream(os);
    }

protected:
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const = 0;

    union {
        mutable const Def* cache_;  ///< Used by @p App.
        size_t index_;              ///< Used by @p Var, @p Extract.
        Box box_;                   ///< Used by @p Assume.
        Qualifier::URAL qualifier_; ///< Used by @p Universe.
        uint64_t dummy_ = 0;        ///< Used to shut up valgrind.
    };

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const = 0;
    bool on_heap() const { return ops_ != vla_ops_; }
    uint32_t fields() const { return nominal_ << 23u | tag_ << 16u | num_ops_; }

    static size_t gid_counter_;

    std::string name_;
    mutable World* world_;
    const Def* type_;
    mutable uint64_t hash_ = 0;
    struct {
        unsigned gid_           : 24;
        unsigned ops_capacity_  : 16;
        unsigned tag_           :  7;
        unsigned nominal_       :  1;
        unsigned num_ops_       : 16;
        // this sum must be 64   ^^^
    };

    mutable Uses uses_;
    const Def** ops_;
    const Def* vla_ops_[0];

    friend class World;
    friend class Cleaner;
    friend class Scope;
};

uint64_t UseHash::hash(Use use) {
    return uint64_t(use.index() & 0x3) | uint64_t(use->gid()) << 2ull;
}

class Quantifier : public Def {
protected:
    Quantifier(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Def(world, tag, type, num_ops, name)
    {}
    Quantifier(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Def(world, tag, type, ops, name)
    {}

    static const Def* max_type(World&, Defs, Qualifier::URAL = Qualifier::Unrestricted);
};

class Constructor : public Def {
protected:
    Constructor(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Def(world, tag, type, num_ops, name)
    {}
    Constructor(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Def(world, tag, type, ops, name)
    {}

};

class Destructor : public Def {
protected:
    Destructor(World& world, int tag, const Def* type, const Def* op, const std::string& name)
        : Destructor(world, tag, type, Defs({op}), name)
    {}
    Destructor(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Def(world, tag, type, ops, name)
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
    Pi(World& world, Defs domains, const Def* body, Qualifier::URAL q, const std::string& name);

public:
    const Def* domain() const;
    Defs domains() const { return ops().skip_back(); }
    size_t num_domains() const { return domains().size(); }
    const Def* body() const { return ops().back(); }
    const Def* reduce(Defs) const;

    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;
};

class Lambda : public Constructor {
private:
    Lambda(World& world, const Pi* type, const Def* body, const std::string& name);

public:
    const Def* domain() const { return type()->domain(); }
    Defs domains() const { return type()->domains(); }
    size_t num_domains() const { return domains().size(); }
    const Def* body() const { return op(0); }
    const Def* reduce(Defs) const;
    const Pi* type() const { return Constructor::type()->as<Pi>(); }

    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;
};

class App : public Destructor {
private:
    App(World& world, const Def* type, const Def* callee, Defs args, const std::string& name)
        : Destructor(world, Node_App, type, concat(callee, args), name)
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;
};

class Sigma : public Quantifier {
private:
    /// Nominal Sigma kind
    Sigma(World& world, size_t num_ops, Qualifier::URAL q, const std::string& name);
    /// Nominal Sigma type, \a type is some Star/Universe
    Sigma(World& world, const Def* type, size_t num_ops, const std::string& name)
        : Quantifier(world, Node_Sigma, type, num_ops, name)
    {
        assert(false && "TODO");
    }
    Sigma(World& world, Defs ops, Qualifier::URAL q, const std::string& name)
        : Quantifier(world, Node_Sigma, max_type(world, ops, q), ops, name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    bool is_unit() const { return ops().empty(); }

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Tuple : public Constructor {
private:
    Tuple(World& world, const Sigma* type, Defs ops, const std::string& name)
        : Constructor(world, Node_Tuple, type, ops, name)
    {
        assert(type->num_ops() == ops.size());
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Extract : public Destructor {
private:
    Extract(World& world, const Def* type, const Def* tuple, size_t index, const std::string& name)
        : Destructor(world, Node_Extract, type, tuple, name)
    {
        index_ = index;
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

public:
    size_t index() const { return index_; }
    virtual std::ostream& stream(std::ostream&) const override;
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;

    friend class World;
};

class Intersection : public Quantifier {
private:
    Intersection(World& world, Defs ops, Qualifier::URAL q, const std::string& name)
        : Quantifier(world, Node_Intersection, max_type(world, ops, q), gid_sorted(ops), name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class All : public Constructor {
private:
    All(World& world, const Intersection* type, Defs ops, const std::string& name)
        : Constructor(world, Node_All, type, ops, name)
    {
        assert(type->num_ops() == ops.size());
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Pick : public Destructor {
private:
    Pick(World& world, const Def* type, const Def* def, const std::string& name)
        : Destructor(world, Node_Pick, type, def, name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Variant : public Quantifier {
private:
    Variant(World& world, Defs ops, Qualifier::URAL q, const std::string& name)
        : Quantifier(world, Node_Variant, max_type(world, ops, q), gid_sorted(ops), name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Any : public Constructor {
private:
    Any(World& world, const Def* type, const Def* def, const std::string& name)
        : Constructor(world, Node_Any, type, {def}, name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

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

    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Match : public Destructor {
private:
    Match(World& world, const Def* type, const Def* def, const Defs handlers, const std::string& name)
        : Destructor(world, Node_Match, type, concat(def, handlers), name)
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world, Qualifier::URAL q);

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;

public:
    virtual std::ostream& stream(std::ostream&) const override;
};

class Universe : public Def {
private:
    Universe(World& world, Qualifier::URAL q)
        : Def(world, Node_Universe, nullptr, 0, "□")
    {
        qualifier_ = q;
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;

public:
    virtual std::ostream& stream(std::ostream&) const override;
};

class Var : public Def {
private:
    Var(World& world, const Def* type, size_t index, const std::string& name)
        : Def(world, Node_Var, type, Defs(), name)
    {
        index_ = index;
    }

private:
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;

public:
    size_t index() const { return index_; }
    virtual std::ostream& stream(std::ostream&) const override;
    /// Do not print variable names as they aren't bound in the output without analysing DeBruijn-Indices.
    virtual std::ostream& name_stream(std::ostream& os) const override {
        return stream(os);
    }
};

class Assume : public Def {
private:
    Assume(World& world, const Def* type, const std::string& name)
        : Def(world, Node_Assume, type, 0, name)
    {}

public:
    virtual bool maybe_dependent() const { return false; }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;
};

class Error : public Def {
private:
    Error(World& world)
        : Def(world, Node_Error, nullptr, Defs(), "error")
    {
        qualifier_ == Qualifier::Unrestricted;
    }

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, size_t, Defs) const override;

    friend class World;
};

}

#endif
