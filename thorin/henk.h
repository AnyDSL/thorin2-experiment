#ifndef HENK_TABLE_NAME
#error "please define the type table name HENK_TABLE_NAME"
#endif

#ifndef HENK_TABLE_TYPE
#error "please define the type table type HENK_TABLE_TYPE"
#endif

#define HENK_UNDERSCORE(N) THORIN_PASTER(N,_)
#define HENK_TABLE_NAME_ HENK_UNDERSCORE(HENK_TABLE_NAME)

//------------------------------------------------------------------------------

#ifdef HENK_CONTINUATION
class Continuation;
#endif

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

typedef thorin::HashSet<Use, UseHash> Uses;

class Var;
class HENK_TABLE_TYPE;

template<class T>
struct GIDHash {
    uint64_t operator()(T n) const { return n->gid(); }
};

template<class Key, class Value>
using GIDMap = thorin::HashMap<const Key*, Value, GIDHash<const Key*>>;
template<class Key>
using GIDSet = thorin::HashSet<const Key*, GIDHash<const Key*>>;

template<class To>
using DefMap  = GIDMap<Def, To>;
using DefSet  = GIDSet<Def>;
using Def2Def = DefMap<const Def*>;

typedef thorin::ArrayRef<const Def*> Defs;

//------------------------------------------------------------------------------

/// Base class for all @p Def%s.
class Def : public thorin::HasLocation, public thorin::Streamable, public thorin::MagicCast<Def> {
protected:
    Def(const Def&) = delete;
    Def& operator=(const Def&) = delete;

    /// Use for nominal @p Def%s.
    Def(HENK_TABLE_TYPE& table, int tag, const Def* type, size_t num_ops, const thorin::Location& loc, const std::string& name)
        : HasLocation(loc)
        , HENK_TABLE_NAME_(&table)
        , tag_(tag)
        , type_(type)
        , ops_(num_ops)
        , gid_(gid_counter_++)
        , is_nominal_(true)
        , name_(name)
    {}

    /// Use for structural @p Def%s.
    Def(HENK_TABLE_TYPE& table, int tag, const Def* type, Defs ops, const thorin::Location& loc, const std::string& name)
        : HasLocation(loc)
        , HENK_TABLE_NAME_(&table)
        , tag_(tag)
        , type_(type)
        , ops_(ops.size())
        , gid_(gid_counter_++)
        , is_nominal_(false)
        , name_(name)
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
    HENK_TABLE_TYPE& HENK_TABLE_NAME() const { return *HENK_TABLE_NAME_; }
    Defs ops() const { return ops_; }
    const Def* op(size_t i) const;
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
    bool is_known()   const { return known_; }                ///< Does this @p Def depend on any @p Unknown%s?
    bool is_monomorphic() const { return monomorphic_; }      ///< Does this @p Def not depend on any @p Var%s?.
    bool is_polymorphic() const { return !is_monomorphic(); } ///< Does this @p Def depend on any @p Var%s?.
    int order() const { return order_; }
    size_t gid() const { return gid_; }
    uint64_t hash() const { return hash_ == 0 ? hash_ = vhash() : hash_; }
    virtual bool equal(const Def*) const;

    const Def* reduce(int, const Def*, Def2Def&) const;
    const Def* rebuild(HENK_TABLE_TYPE& to, Defs ops) const;
    const Def* rebuild(Defs ops) const { return rebuild(HENK_TABLE_NAME(), ops); }

#ifdef HENK_CONTINUATION
    Continuation* as_continuation() const;
    Continuation* isa_continuation() const;
#endif

    static size_t gid_counter() { return gid_counter_; }

protected:
    virtual uint64_t vhash() const;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const = 0;
    thorin::Array<const Def*> reduce_ops(int, const Def*, Def2Def&) const;

    mutable uint64_t hash_ = 0;
    int order_ = 0;
    mutable bool known_       = true;
    mutable bool monomorphic_ = true;

private:
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const = 0;

    mutable HENK_TABLE_TYPE* HENK_TABLE_NAME_;
    int tag_;
    const Def* type_;
    std::vector<const Def*> ops_;
    mutable size_t gid_;
    static size_t gid_counter_;
    mutable Uses uses_;
    mutable bool is_nominal_;

public:
    mutable std::string name_;

    template<class> friend class TableBase;
    friend class Cleaner;
    friend class Scope;
    friend class Tracker;
};

uint64_t UseHash::operator()(Use use) const {
    return thorin::hash_combine(thorin::hash_begin(use->gid()), use.index());
}

class Abs : public Def {
protected:
    Abs(HENK_TABLE_TYPE& table, int tag, const Def* type, size_t num_ops, const thorin::Location& loc, const std::string& name)
        : Def(table, tag, type, num_ops, loc, name)
    {}
    Abs(HENK_TABLE_TYPE& table, int tag, const Def* type, Defs ops, const thorin::Location& loc, const std::string& name)
        : Def(table, tag, type, ops, loc, name)
    {}
};

class Connective : public Abs {
protected:
    Connective(HENK_TABLE_TYPE& table, int tag, const Def* type, size_t num_ops, const thorin::Location& loc, const std::string& name)
        : Abs(table, tag, type, num_ops, loc, name)
    {}
    Connective(HENK_TABLE_TYPE& table, int tag, const Def* type, Defs ops, const thorin::Location& loc, const std::string& name)
        : Abs(table, tag, type, ops, loc, name)
    {}
};

class Quantifier : public Abs {
protected:
    Quantifier(HENK_TABLE_TYPE& table, int tag, const Def* type, size_t num_ops, const thorin::Location& loc, const std::string& name)
        : Abs(table, tag, type, num_ops, loc, name)
    {}
    Quantifier(HENK_TABLE_TYPE& table, int tag, const Def* type, Defs ops, const thorin::Location& loc, const std::string& name)
        : Abs(table, tag, type, ops, loc, name)
    {}
};

class Lambda : public Connective {
private:
    Lambda(HENK_TABLE_TYPE& table, const Def* domain, const Def* body, const thorin::Location& loc, const std::string& name);

public:
    const Def* domain() const { return op(0); }
    const Def* body() const { return op(1); }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    template<class> friend class TableBase;
};

class Pi : public Quantifier {
private:
    Pi(HENK_TABLE_TYPE& table, const Def* domain, const Def* body, const thorin::Location& loc, const std::string& name);

public:
    const Def* domain() const { return op(0); }
    const Def* body() const { return op(1); }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    template<class> friend class TableBase;
};

class Tuple : public Connective {
private:
    Tuple(HENK_TABLE_TYPE& table, const Def* type, Defs ops, const thorin::Location& loc, const std::string& name)
        : Connective(table, Node_Tuple, type, ops, loc, name)
    {}

    Tuple(HENK_TABLE_TYPE& table, Defs ops, const thorin::Location& loc, const std::string& name);

    static const Def* infer_type(HENK_TABLE_TYPE& table, Defs ops, const thorin::Location& loc, const std::string& name);

    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    template<class> friend class TableBase;
};

class Sigma : public Quantifier {
private:
    Sigma(HENK_TABLE_TYPE& table, size_t num_ops, const thorin::Location& loc, const std::string& name)
        : Quantifier(table, Node_Sigma, nullptr /*TODO*/, num_ops, loc, name)
    {}
    Sigma(HENK_TABLE_TYPE& table, Defs ops, const thorin::Location& loc, const std::string& name)
        : Quantifier(table, Node_Sigma, nullptr /*TODO*/, ops, loc, name)
    {}

    static const Def* infer_type(HENK_TABLE_TYPE& table, Defs ops, const thorin::Location& loc, const std::string& name);

    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;

public:
    virtual std::ostream& stream(std::ostream&) const override;

    template<class> friend class TableBase;
};

class Star : public Def {
private:
    Star(HENK_TABLE_TYPE& table)
        : Def(table, Node_Star, nullptr, {}, thorin::Location(), "type")
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    template<class> friend class TableBase;
};

class Var : public Def {
private:
    Var(HENK_TABLE_TYPE& table, const Def* type, int depth, const thorin::Location& loc, const std::string& name)
        : Def(table, Node_Var, type, {}, loc, name)
        , depth_(depth)
    {
        monomorphic_ = false;
    }

public:
    int depth() const { return depth_; }
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual uint64_t vhash() const override;
    virtual bool equal(const Def*) const override;
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    int depth_;

    template<class> friend class TableBase;
};

class App : public Def {
private:
    App(HENK_TABLE_TYPE& table, const Def* callee, Defs args, const thorin::Location& loc, const std::string& name)
        : Def(table, Node_App, infer_type(table, callee, args, loc, name), concat(callee, args), loc, name)
    {}

    static const Def* infer_type(HENK_TABLE_TYPE& table, const Def* callee, const Def* arg, const thorin::Location& loc, const std::string& name);
    static const Def* infer_type(HENK_TABLE_TYPE& table, const Def* callee, Defs arg, const thorin::Location& loc, const std::string& name);

public:
    const Def* callee() const { return Def::op(0); }
    const Def* arg() const { return Def::op(1); }
    virtual std::ostream& stream(std::ostream&) const override;
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

private:
    mutable const Def* cache_ = nullptr;
    template<class> friend class TableBase;
};

class Error : public Def {
private:
    Error(HENK_TABLE_TYPE& table, const Def* type)
        : Def(table, Node_Error, type, {}, thorin::Location(), "<error>")
    {}

public:
    virtual std::ostream& stream(std::ostream&) const override;

private:
    virtual const Def* vrebuild(HENK_TABLE_TYPE& to, Defs ops) const override;
    virtual const Def* vreduce(int, const Def*, Def2Def&) const override;

    template<class> friend class TableBase;
};

//------------------------------------------------------------------------------

class Tracker {
public:
    Tracker()
        : def_(nullptr)
    {}
    Tracker(const Def* def)
        : def_(def)
    {
        if (def) {
            put(*this);
            verify();
        }
    }
    Tracker(const Tracker& other)
        : def_(other)
    {
        if (other) {
            put(*this);
            verify();
        }
    }
    Tracker(Tracker&& other)
        : def_(*other)
    {
        if (other) {
            other.unregister();
            other.def_ = nullptr;
            put(*this);
            verify();
        }
    }
    ~Tracker() { if (*this) unregister(); }

    const Def* operator*() const { return def_; }
    bool operator==(const Tracker& other) const { return this->def_ == other.def_; }
    bool operator!=(const Tracker& other) const { return this->def_ != other.def_; }
    bool operator==(const Def* def) const { return this->def_ == def; }
    bool operator!=(const Def* def) const { return this->def_ != def; }
    const Def* operator->() const { return **this; }
    operator const Def*() const { return **this; }
    explicit operator bool() { return def_; }
    Tracker& operator=(Tracker other) { swap(*this, other); return *this; }

    friend void swap(Tracker& t1, Tracker& t2) {
        using std::swap;

        if (t1 != t2) {
            if (t1) {
                if (t2) {
                    t1.update(t2);
                    t2.update(t1);
                } else {
                    t1.update(t2);
                }
            } else {
                assert(!t1 && t2);
                t2.update(t1);
            }

            std::swap(t1.def_, t2.def_);
        } else {
            t1.verify();
            t2.verify();
        }
    }

private:
    thorin::HashSet<Tracker*>& trackers(const Def* def);
    void verify() { assert(!def_ || trackers(def_).contains(this)); }
    void put(Tracker& other) {
        auto p = trackers(def_).insert(&other);
        assert_unused(p.second && "couldn't insert tracker");
    }

    void unregister() {
        assert(trackers(def_).contains(this) && "tracker not found");
        trackers(def_).erase(this);
    }

    void update(Tracker& other) {
        unregister();
        put(other);
    }

    mutable const Def* def_;
    friend void Def::replace(const Def*) const;
};

//------------------------------------------------------------------------------

template<class HENK_TABLE_TYPE>
class TableBase {
private:
    HENK_TABLE_TYPE& HENK_TABLE_NAME() { return *static_cast<HENK_TABLE_TYPE*>(this); }

public:
    struct DefHash { uint64_t operator()(const Def* def) const { return def->hash(); } };
    struct DefEqual { bool operator()(const Def* d1, const Def* d2) const { return d2->equal(d1); } };
    typedef thorin::HashSet<const Def*, DefHash, DefEqual> DefSet;

    TableBase& operator=(const TableBase&);
    TableBase(const TableBase&);

    TableBase() {}
    virtual ~TableBase() { for (auto def : defs_) delete def; }

    const Star* star();
    const Var* var(const Def* type, int depth, const thorin::Location& loc, const std::string& name = "") { return unify(new Var(HENK_TABLE_NAME(), type, depth, loc, name)); }
    const Lambda* lambda(const Def* domain, const Def* body, const thorin::Location& loc = thorin::Location(), const std::string& name = "") { return unify(new Lambda(HENK_TABLE_NAME(), domain, body, loc, name)); }
    const Pi*     pi    (const Def* domain, const Def* body, const thorin::Location& loc = thorin::Location(), const std::string& name = "") { return unify(new Pi    (HENK_TABLE_NAME(), domain, body, loc, name)); }
    const Def* app(const Def* callee, const Def* arg, const thorin::Location& loc, const std::string& name = "");
    const Def* app(const Def* callee, Defs args, const thorin::Location& loc, const std::string& name = "");
    const Tuple* tuple(const Def* type, Defs ops, const thorin::Location& loc, const std::string& name = "") { return unify(new Tuple(HENK_TABLE_NAME(), type, ops, loc, name)); }
    const Tuple* tuple(Defs ops, const thorin::Location& loc, const std::string& name = "") {
        Array<const Def*> types(ops.size());
        for (size_t i = 0, e = types.size(); i != e; ++i)
            types[i] = ops[i]->type();
        return tuple(sigma(types, loc, name), ops, loc, name);
    }
    const Sigma* sigma(Defs ops, const thorin::Location& loc = thorin::Location(), const std::string& name = "") { return unify(new Sigma(HENK_TABLE_NAME(), ops, loc, name)); }
    Sigma* sigma(size_t num_ops, const thorin::Location& loc = thorin::Location(), const std::string& name = "") { return insert(new Sigma(HENK_TABLE_NAME(), num_ops, loc, name)); }
    const Error* error(const Def* type) { return unify(new Error(HENK_TABLE_NAME(), type)); }
    const Error* error() { return error(error(star())); }

    const DefSet& defs() const { return defs_; }

    friend void swap(TableBase& t1, TableBase& t2) {
        using std::swap;
        swap(t1.defs_, t2.defs_);
        t1.fix();
        t2.fix();
    }

private:
    void fix() {
        for (auto def : defs_)
            def->HENK_TABLE_NAME_ = &HENK_TABLE_NAME();
    }

protected:
    const Def* unify_base(const Def* type);
    template<class T>
    const T* unify(const T* type) { return unify_base(type)->template as<T>(); }

    template<class T>
    T* insert(T* def) {
        auto p = defs_.emplace(def);
        assert_unused(p.second);
        return def;
    }

    DefSet defs_;

    friend class Lambda;
};

//------------------------------------------------------------------------------

#ifdef HENK_CONTINUATION
#undef HENK_CONTINUATION
#endif
#undef HENK_TABLE_NAME
#undef HENK_TABLE_TYPE
