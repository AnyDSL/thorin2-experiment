#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include <numeric>

#include "thorin/util/array.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"

namespace thorin {

enum {
    Node_App,
    Node_Assume,
    Node_Lambda,
    Node_Pi,
    Node_Sigma,
    Node_Star,
    Node_Tuple,
    Node_Var,
    Node_Error,
};

class Def;
class World;

/**
 * References a user.
 * A \p Def \c u which uses \p Def \c d as \c i^th operand is a \p Use with \p index_ \c i of \p Def \c d.
 */
class Use : public Streamable {
public:
    Use() {}
    Use(size_t index, const Def* def)
        : index_(index)
        , def_(def)
    {}

    size_t index() const { return index_; }
    const Def* def() const { return def_; }
    operator const Def*() const { return def_; }
    const Def* operator->() const { return def_; }
    bool operator==(Use other) const { return this->def() == other.def() && this->index() == other.index(); }
    std::ostream& stream(std::ostream& os) const override;

private:
    size_t index_;
    const Def* def_;
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
class Def : public Streamable, public MagicCast<Def> {
public:
    enum Sort {
        Term, Type, Kind
    };

protected:
    Def(const Def&) = delete;
    Def& operator=(const Def&) = delete;

    /// Use for nominal @p Def%s.
    Def(World& world, unsigned tag, const Def* type, size_t num_ops, Qualifier::URAL qualifier,
        const std::string& name)
        : name_(name)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , tag_(tag)
        , nominal_(true)
        , qualifier_(qualifier)
        , num_ops_(num_ops)
        , ops_capacity_(num_ops)
        , ops_(&vla_ops_[0])
    {}

    /// Use for structural @p Def%s.
    Def(World& world, unsigned tag, const Def* type, Defs ops, Qualifier::URAL qualifier,
        const std::string& name)
        : name_(name)
        , world_(&world)
        , type_(type)
        , gid_(gid_counter_++)
        , tag_(tag)
        , nominal_(false)
        , qualifier_(qualifier)
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
    Sort sort() const {
        if (!type())
            return Kind;
        else if (!type()->type())
            return Type;
        else {
            assert(!type()->type()->type());
            return Term;
        }
    }

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
    bool is_kind() const { return sort() == Kind; }
    bool is_type() const { return sort() == Type; }
    bool is_term() const { return sort() == Term; }

    Qualifier::URAL qualifier() const { return Qualifier::URAL(qualifier_); }
    bool is_unrestricted() const { return qualifier() & Qualifier::Unrestricted; }
    bool is_affine() const       { return qualifier() & Qualifier::Affine; }
    bool is_relevant() const     { return qualifier() & Qualifier::Relevant; }
    bool is_linear() const       { return qualifier() & Qualifier::Linear; }

    size_t gid() const { return gid_; }
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual int num_vars() const { return 0; }

    const Def* subst(Def2Def&, int, Defs) const;
    const Def* rebuild(const Def* type, Defs defs) const { return rebuild(world(), type, defs); }

    static size_t gid_counter() { return gid_counter_; }

protected:
    virtual uint64_t vhash() const;
    virtual bool equal(const Def*) const;
    virtual const Def* vsubst(Def2Def&, int, Defs) const = 0;

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
    return use->gid() | (uint64_t(use.index()) << 32);
}

class Abs : public Def {
protected:
    Abs(World& world, int tag, const Def* type, size_t num_ops, Qualifier::URAL q, const std::string& name)
        : Def(world, tag, type, num_ops, q, name)
    {}
    Abs(World& world, int tag, const Def* type, Defs ops, Qualifier::URAL q, const std::string& name)
        : Def(world, tag, type, ops, q, name)
    {}

public:
    virtual const Def* reduce(Defs defs) const = 0;
};

class Quantifier : public Abs {
protected:
    Quantifier(World& world, int tag, const Def* type, size_t num_ops, Qualifier::URAL q, const std::string& name)
        : Abs(world, tag, type, num_ops, q, name)
    {}
    Quantifier(World& world, int tag, const Def* type, Defs ops, Qualifier::URAL q, const std::string& name)
        : Abs(world, tag, type, ops, q, name)
    {}

public:
    virtual const Def* domain() const = 0;
};

class Connective : public Abs {
protected:
    Connective(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Abs(world, tag, type, num_ops, Qualifier::Unrestricted, name)
    {}
    Connective(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Abs(world, tag, type, ops, Qualifier::Unrestricted, name)
    {}

public:
    virtual const Def* domain() const = 0;
};

class Pi : public Quantifier {
private:
    Pi(World& world, Defs domains, const Def* body, Qualifier::URAL q, const std::string& name);

public:
    Defs domains() const { return ops().skip_back(); }
    const Def* body() const { return ops().back(); }
    virtual const Def* reduce(Defs defs) const override { Def2Def map; return body()->subst(map, 1, defs); }
    virtual int num_vars() const override { return 1; }
    virtual const Def* domain() const override;
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

class Lambda : public Connective {
private:
    Lambda(World& world, const Pi* type, const Def* body, const std::string& name);

public:
    Defs domains() const { return ops().skip_back(); }
    const Def* body() const { return ops().back(); }
    virtual int num_vars() const override { return 1; }
    virtual const Def* reduce(Defs defs) const override { Def2Def map; return body()->subst(map, 1, defs); }
    virtual const Def* domain() const override;
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

class Sigma : public Quantifier {
private:
    Sigma(World& world, size_t num_ops, Qualifier::URAL q, const std::string& name)
        : Quantifier(world, Node_Sigma, nullptr /*TODO*/, num_ops, q, name)
    {
        assert(false && "TODO");
    }
    Sigma(World& world, Defs ops, const std::string& name, Qualifier::URAL q)
        : Quantifier(world, Node_Sigma, infer_type(world, ops), ops, q, name)
    {
        assert(sort() != Type || qualifier() <= Qualifier::meet(ops));
    }

    static const Def* infer_type(World&, Defs);

    virtual const Def* reduce(Defs defs) const override;
    virtual int num_vars() const override { return num_ops(); }
    virtual const Def* domain() const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Tuple : public Connective {
private:
    Tuple(World& world, const Sigma* type, Defs ops, const std::string& name)
        : Connective(world, Node_Tuple, type, ops, name)
    {
        assert(type->num_ops() == ops.size());
    }

    virtual const Def* reduce(Defs defs) const override;
    virtual const Def* domain() const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world, Qualifier::URAL q)
        : Def(world, Node_Star, nullptr, Defs(), q, "type")
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

class Var : public Def {
private:
    Var(World& world, const Def* type, int index, const std::string& name)
        : Def(world, Node_Var, type, Defs(), type->qualifier(), name)
    {
        index_ = index;
    }

public:
    int index() const { return index_; }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

class Assume : public Def {
private:
    Assume(World& world, const Def* type, Qualifier::URAL q, const std::string& name)
        : Def(world, Node_Assume, type, 0, q, name)
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

class App : public Def {
private:
    App(World& world, const Def* type, const Def* callee, Defs args, const std::string& name);

    static const Def* infer_type(const Def*, Defs);

public:
    const Def* callee() const { return ops().front(); }
    const Quantifier* quantifier() const { return callee()->type()->as<Quantifier>(); }
    const Def* domain() const { return quantifier()->domain(); }
    Defs args() const { return ops().skip_front(); }
    size_t num_args() const { return args().size(); }
    const Def* arg(size_t i = 0) const { return args()[i]; }
    virtual std::ostream& stream(std::ostream&) const override;
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

class Error : public Def {
private:
    Error(World& world)
        : Def(world, Node_Error, nullptr, Defs(), Qualifier::Unrestricted, "error")
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* rebuild(World&, const Def*, Defs) const override;
    virtual const Def* vsubst(Def2Def&, int, Defs) const override;

    friend class World;
};

}

#endif
