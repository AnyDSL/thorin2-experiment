#ifndef THORIN_DEF_H
#define THORIN_DEF_H

#include "thorin/util/array.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"

namespace thorin {

enum {
    Node_App,
    Node_Lambda,
    Node_Pi,
    Node_Sigma,
    Node_Star,
    Node_Tuple,
    Node_Var,
};

class Def;

/**
 * References a user.
 * A \p Def \c u which uses \p Def \c d as \c i^th operand is a \p Use with \p index_ \c i of \p Def \c d.
 */
class Use {
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

private:
    size_t index_;
    const Def* def_;
};

//------------------------------------------------------------------------------

struct UseHash {
    inline uint64_t operator()(Use use) const;
};

typedef HashSet<Use, UseHash> Uses;

class Var;
class World;

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

//------------------------------------------------------------------------------

/// Base class for all @p Def%s.
class Def : public Streamable, public MagicCast<Def> {
protected:
    Def(const Def&) = delete;
    Def& operator=(const Def&) = delete;

    /// Use for nominal @p Def%s.
    Def(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : world_(&world)
        , tag_(tag)
        , type_(type)
        , ops_(num_ops)
        , gid_(gid_counter_++)
        , name_(name)
        , is_nominal_(true)
    {}

    /// Use for structural @p Def%s.
    Def(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : world_(&world)
        , tag_(tag)
        , type_(type)
        , ops_(ops.size())
        , gid_(gid_counter_++)
        , name_(name)
        , is_nominal_(false)
    {
        for (size_t i = 0, e = num_ops(); i != e; ++i) {
            if (auto op = ops[i])
                set(i, op);
        }
    }

    void clear_type() { type_ = nullptr; }
    void set_type(const Def* type) { type_ = type; }
    void unregister_use(size_t i) const;
    void unregister_uses() const;
    void resize(size_t n) { ops_.resize(n, nullptr); }

public:
    enum Sort {
        Term, Type, Kind
    };

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

    int tag() const { return tag_; }
    World& world() const { return *world_; }
    Defs ops() const { return ops_; }
    const Def* op(size_t i) const { return ops()[i]; }
    size_t num_ops() const { return ops_.size(); }
    bool empty() const { return ops_.empty(); }
    const Uses& uses() const { return uses_; }
    size_t num_uses() const { return uses().size(); }
    const Def* type() const { return type_; }
    const std::string& name() const { return name_; }
    std::string unique_name() const;
    void set(Defs);
    void set(size_t i, const Def*);
    void unset(size_t i);
    void replace(const Def*) const;
    bool is_nominal() const { return is_nominal_; }           ///< A nominal @p Def is always different from each other @p Def.
    bool is_structural() const { return !is_nominal(); }      ///< A structural @p Def is always unified with a syntactically equivalent @p Def.
    size_t gid() const { return gid_; }
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual bool equal(const Def*) const;

    const Def* reduce(int, const Def*, Def2Def&) const;
    const Def* rebuild(World& to, Defs ops) const;
    const Def* rebuild(Defs ops) const { return rebuild(world(), ops); }

    static size_t gid_counter() { return gid_counter_; }

protected:
    virtual uint64_t vhash() const;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const = 0;
    Array<const Def*> reduce_ops(int, const Def*, Def2Def&) const;

    mutable uint64_t hash_ = 0;

private:
    virtual const Def* vrebuild(World& to, Defs ops) const = 0;

    static size_t gid_counter_;

    mutable World* world_;
    int tag_;
    const Def* type_;
    std::vector<const Def*> ops_;
    mutable size_t gid_;
    mutable Uses uses_;
    mutable std::string name_;
    mutable bool is_nominal_;

    friend class World;
    friend class Cleaner;
    friend class Scope;
};

uint64_t UseHash::operator()(Use use) const {
    return hash_combine(hash_begin(use->gid()), use.index());
}

class Abs : public Def {
protected:
    Abs(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Def(world, tag, type, num_ops, name)
    {}
    Abs(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Def(world, tag, type, ops, name)
    {}
};

class Connective : public Abs {
protected:
    Connective(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Abs(world, tag, type, num_ops, name)
    {}
    Connective(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Abs(world, tag, type, ops, name)
    {}
};

class Quantifier : public Abs {
protected:
    Quantifier(World& world, int tag, const Def* type, size_t num_ops, const std::string& name)
        : Abs(world, tag, type, num_ops, name)
    {}
    Quantifier(World& world, int tag, const Def* type, Defs ops, const std::string& name)
        : Abs(world, tag, type, ops, name)
    {}
};

class Lambda : public Connective {
private:
    Lambda(World& world, const Def* domain, const Def* body, const std::string& name);

public:
    const Def* domain() const { return op(0); }
    const Def* body() const { return op(1); }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(World& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    friend class World;
};

class Pi : public Quantifier {
private:
    Pi(World& world, const Def* domain, const Def* body, const std::string& name);

public:
    const Def* domain() const { return op(0); }
    const Def* body() const { return op(1); }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(World& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    friend class World;
};

class Tuple : public Connective {
private:
    Tuple(World& world, const Def* type, Defs ops, const std::string& name)
        : Connective(world, Node_Tuple, type, ops, name)
    {}

    Tuple(World& world, Defs ops, const std::string& name);

    static const Def* infer_type(World& world, Defs ops, const std::string& name);

    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;
    virtual const Def* vrebuild(World& to, Defs ops) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Sigma : public Quantifier {
private:
    Sigma(World& world, size_t num_ops, const std::string& name)
        : Quantifier(world, Node_Sigma, nullptr /*TODO*/, num_ops, name)
    {}
    Sigma(World& world, Defs ops, const std::string& name)
        : Quantifier(world, Node_Sigma, nullptr /*TODO*/, ops, name)
    {}

    static const Def* infer_type(World& world, Defs ops, const std::string& name);

    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;
    virtual const Def* vrebuild(World& to, Defs ops) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    friend class World;
};

class Star : public Def {
private:
    Star(World& world)
        : Def(world, Node_Star, nullptr, {}, "type")
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(World& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    friend class World;
};

class Var : public Def {
private:
    Var(World& world, const Def* type, int depth, const std::string& name)
        : Def(world, Node_Var, type, {}, name)
        , depth_(depth)
    {}

public:
    int depth() const { return depth_; }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;
    virtual const Def* vrebuild(World& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    int depth_;

    friend class World;
};

class App : public Def {
private:
    App(World& world, const Def* callee, Defs args, const std::string& name)
        : Def(world, Node_App, infer_type(world, callee, args, name), concat(callee, args), name)
    {}

    static const Def* infer_type(World& world, const Def* callee, const Def* arg, const std::string& name);
    static const Def* infer_type(World& world, const Def* callee, Defs arg, const std::string& name);

public:
    const Def* callee() const { return Def::op(0); }
    const Def* arg() const { return Def::op(1); }
    virtual std::ostream& stream(std::ostream&) const override;
    virtual const Def* vrebuild(World& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

private:
    mutable const Def* cache_ = nullptr;
    friend class World;
};

}

#endif
