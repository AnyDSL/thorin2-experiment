#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include "thorin/util/array.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"

namespace thorin {

enum {
    Node_All,
    Node_Any,
    Node_App,
    Node_Assume,
    Node_Extract,
    Node_Intersection,
    Node_Lambda,
    Node_Match,
    Node_Pi,
    Node_Pick,
    Node_Sigma,
    Node_Star,
    Node_Tuple,
    Node_Var,
    Node_Variant,
};

class Def;
class World;

/**
 * References a user.
 * A \p Def \c u which uses \p Def \c d as \c i^th operand is a \p Use with \p index_ \c i of \p Def \c d.
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
    inline uint64_t operator()(Use use) const;
};

typedef HashSet<Use, UseHash> Uses;

template<class T>
struct GIDHash {
    uint64_t operator()(T n) const { return n->gid(); }
};

template<class Key, class Value>
using GIDMap = HashMap<const Key*, Value, GIDHash<const Key*>>;
template<class Key>
using GIDSet = HashSet<const Key*, GIDHash<const Key*>>;

template<class To>
using DefMap  = GIDMap<Def, To>;
using DefSet  = GIDSet<Def>;
using Def2Def = DefMap<const Def*>;

typedef ArrayRef<const Def*> Defs;

Array<const Def*> types(Defs defs);

Array<const Def*> gid_sorted(Defs defs);

//------------------------------------------------------------------------------

/// Base class for all @p Def%s.
class Def : public Streamable, public MagicCast<Def> {
public:
    enum Sort {
        Term, Type, Kind
    };

    enum Qualifier {
        Unrestricted,
        Affine   = 1 << 0,
        Relevant = 1 << 1,
        Linear = Affine | Relevant,
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
        , tag_(tag)
        , nominal_(true)
        , qualifier_(Unrestricted)
        , num_ops_(num_ops)
        , ops_capacity_(num_ops)
        , ops_(&vla_ops_[0])
    {}

    /// Use for structural @p Def%s.
    Def(World& world, unsigned tag, const Def* type, Defs ops, const std::string& name)
        : name_(name)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , tag_(tag)
        , nominal_(false)
        , qualifier_(Unrestricted)
        , num_ops_(ops.size())
        , ops_capacity_(num_ops_)
        , ops_(&vla_ops_[0])
    {
        std::copy(ops.begin(), ops.end(), ops_);
    }

    void clear_type() { type_ = nullptr; }
    void set_type(const Def* type) { type_ = type; }
    void set(size_t i, const Def*);
    void wire_uses() const;
    void unset(size_t i);
    void unregister_use(size_t i) const;
    void unregister_uses() const;
    void resize(size_t num_ops) {
        num_ops_ = num_ops;
        if (num_ops_ > ops_capacity_)
            assert(false && "TODO");
    }

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
    Qualifier qualifier() const { return Qualifier(qualifier_); }
    bool is_unrestricted() const { return qualifier() & Unrestricted; }
    bool is_affine() const       { return qualifier() & Affine; }
    bool is_relevant() const     { return qualifier() & Relevant; }
    bool is_linear() const       { return qualifier() & Linear; }
    size_t gid() const { return gid_; }
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }

    const Def* substitute(Def2Def&, int, Defs) const;
    const Def* rebuild(const Def* type, Defs defs) const { return rebuild(world(), type, defs); }

    static size_t gid_counter() { return gid_counter_; }

    virtual bool maybe_dependent() const { return true; }
    virtual std::ostream& name_stream(std::ostream& os) const {
        if (name() != "")
            return os << name();
        return stream(os);
    }

protected:
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const = 0;

    union {
        mutable const Def* cache_;  ///< Used by @p App.
        size_t index_;              ///< Used by @p Var.
    };

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const = 0;
    uint64_t hash_fields() const {
        // everything except ops_capacity_ (the upper 16 bits) is relevant for hashing
        return fields_ & 0xFFFFFFFFFFFFFF00;
    }

    static size_t gid_counter_;

    std::string name_;
    mutable Uses uses_;
    mutable World* world_;
    const Def* type_;
    mutable uint64_t hash_ = 0;
    union {
        struct {
            unsigned gid_           : 24;
            unsigned tag_           :  5;
            unsigned nominal_       :  1;
            unsigned qualifier_     :  2;
            unsigned num_ops_       : 16;
            unsigned ops_capacity_  : 16;
            // this sum must be 64   ^^^
        };
        uint64_t fields_;
    };

    const Def** ops_;
    const Def* vla_ops_[0];

    friend class World;
    friend class Cleaner;
    friend class Scope;
};

uint64_t UseHash::operator()(Use use) const {
    return uint64_t(use.index()) << 48ull | uint64_t(use->gid());
}

class Quantifier : public Def {
protected:
    Quantifier(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Def(world, tag, type, num_ops, name)
    {}
    Quantifier(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Def(world, tag, type, ops, name)
    {}

    static const Def* max_type(World&, Defs);
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
    Pi(World& world, Defs domains, const Def* body, const std::string& name);

public:
    const Def* domain() const;
    Defs domains() const { return ops().skip_back(); }
    const Def* body() const { return ops().back(); }
    const Def* reduce(Defs defs) const { Def2Def map; return body()->substitute(map, 0, defs); }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;
};

class Lambda : public Constructor {
private:
    Lambda(World& world, const Pi* type, const Def* body, const std::string& name);

public:
    const Def* domain() const;
    Defs domains() const { return type()->domains(); }
    const Def* body() const { return op(0); }
    const Pi* type() const { return Constructor::type()->as<Pi>(); }

    const Def* reduce(Defs defs) const { Def2Def map; return body()->substitute(map, 0, defs); }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;
};

class App : public Destructor {
private:
    App(World& world, const Def* type, const Def* callee, Defs args, const std::string& name)
        : Destructor(world, Node_App, type,  concat(callee, args), name)
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;
};

class Sigma : public Quantifier {
private:
    Sigma(World& world, const Def* type, size_t num_ops, const std::string& name)
        : Quantifier(world, Node_Sigma, type, num_ops, name)
    {
        assert(false && "TODO");
    }
    Sigma(World& world, Defs ops, const std::string& name)
        : Quantifier(world, Node_Sigma, max_type(world, ops), ops, name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Tuple : public Constructor {
private:
    Tuple(World& world, const Def* type, Defs ops, const std::string& name)
        : Constructor(world, Node_Tuple, type, ops, name)
    {
        assert(type->as<Sigma>()->num_ops() == ops.size());
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Extract : public Destructor {
private:
    Extract(World& world, const Def* type, const Def* tuple, int index, const std::string& name)
        : Destructor(world, Node_Extract, type, tuple, name)
    {
        index_ = index;
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    int index() const { return index_; }
    virtual std::ostream& stream(std::ostream&) const override;
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;

    friend class World;
};

class Intersection : public Quantifier {
private:
    Intersection(World& world, Defs ops, const std::string& name)
        : Quantifier(world, Node_Intersection, max_type(world, ops), gid_sorted(ops), name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class All : public Constructor {
private:
    All(World& world, const Def* type, Defs ops, const std::string& name)
        : Constructor(world, Node_All, type, ops, name)
    {
        assert(type->as<Sigma>()->num_ops() == ops.size());
    }

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

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
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Variant : public Quantifier {
private:
    Variant(World& world, Defs ops, const std::string& name)
        : Quantifier(world, Node_Variant, max_type(world, ops), gid_sorted(ops), name)
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Any : public Constructor {
private:
    Any(World& world, const Def* type, int index, const Def* def, const std::string& name)
        : Constructor(world, Node_Any, type, {def}, name)
    {
        index_ = index;
    }

    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

public:
    const Def* def() const { return op(0); }
    int index() const { return index_; }

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
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world)
        : Def(world, Node_Star, nullptr, Defs(), "*")
    {}

    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;

public:
    virtual std::ostream& stream(std::ostream&) const override;
};

class Var : public Def {
private:
    Var(World& world, const Def* type, int index, const std::string& name)
        : Def(world, Node_Var, type, Defs(), name)
    {
        index_ = index;
    }

private:
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;

public:
    int index() const { return index_; }
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
    virtual const Def* vsubstitute(Def2Def&, int, Defs) const override;

    friend class World;
};

}

#endif
