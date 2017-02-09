#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <memory>
#include <string>

#include "thorin/def.h"
#include "thorin/tables.h"

namespace thorin {

class WorldBase {
public:
    struct DefHash {
        static uint64_t hash(const Def* def) { return def->hash(); }
        static bool eq(const Def* d1, const Def* d2) { return d2->equal(d1); }
        static const Def* sentinel() { return (const Def*)(1); }
    };

    typedef HashSet<const Def*, DefHash> DefSet;

    WorldBase& operator=(const WorldBase&) = delete;
    WorldBase(const WorldBase&) = delete;

    WorldBase();
    ~WorldBase();

    const Universe* universe(Qualifier q = Qualifier::Unrestricted) const { return universe_[size_t(q)]; }
    const Star* star(Qualifier q = Qualifier::Unrestricted) const { return star_[size_t(q)]; }
    const Axiom* arity_kind() const { return arity_kind_; }
    const Axiom* arity(size_t a, Debug dbg = {}) { return assume(arity_kind(), {u64(a)}, dbg); }
    const Def* index(size_t index, size_t arity, Debug dbg = {});
    const Def* variadic(const Def* arity, const Def* body, Debug dbg = {});
    /// @em nominal Axiom
    const Axiom* axiom(const Def* type, Debug dbg = {}) { return insert<Axiom>(0, *this, type, dbg); }
    /// @em structural Axiom
    const Axiom* assume(const Def* type, Box box, Debug dbg = {}) {
        return unify<Axiom>(0, *this, type, box, dbg);
    }
    const Def* dimension(const Def* def, Debug dbg = {});
    const Error* error(const Def* type) { return unify<Error>(0, *this, type); }
    const Var* var(Defs types, size_t index, Debug dbg = {}) { return var(sigma(types), index, dbg); }
    const Var* var(const Def* type, size_t index, Debug dbg = {}) {
        return unify<Var>(0, *this, type, index, dbg);
    }

    const Pi* pi(const Def* domain, const Def* body, Debug dbg = {}) {
        return pi(Defs({domain}), body, dbg);
    }
    const Pi* pi(Defs domains, const Def* body, Debug dbg = {}) {
        return pi(domains, body, Qualifier::Unrestricted, dbg);
    }
    const Pi* pi(const Def* domain, const Def* body, Qualifier q, Debug dbg = {}) {
        return pi(Defs({domain}), body, q, dbg);
    }
    const Pi* pi(Defs domains, const Def* body, Qualifier q, Debug dbg = {});
    const Lambda* lambda(const Def* domain, const Def* body, Debug dbg = {}) {
        return lambda(domain, body, Qualifier::Unrestricted, dbg);
    }
    const Lambda* lambda(const Def* domain, const Def* body, Qualifier type_q,
                         Debug dbg = {}) {
        return lambda(Defs({domain}), body, type_q, dbg);
    }
    const Lambda* lambda(Defs domains, const Def* body, Debug dbg = {}) {
        return lambda(domains, body, Qualifier::Unrestricted, dbg);
    }
    const Lambda* lambda(Defs domains, const Def* body, Qualifier type_q, Debug dbg = {}) {
        return pi_lambda(pi(domains, body->type(), type_q), body, dbg);
    }
    Lambda* pi_lambda(const Pi* pi, Debug dbg = {}) {
        return insert<Lambda>(1, *this, pi, dbg);
    }
    const Lambda* pi_lambda(const Pi* pi, const Def* body, Debug dbg = {});
    const Def* app(const Def* callee, Defs args, Debug dbg = {});
    const Def* app(const Def* callee, const Def* arg, Debug dbg = {}) {
        return app(callee, Defs({arg}), dbg);
    }
    const Def* app(const Def* callee, Debug dbg = {}) {
        return app(callee, tuple0(), dbg);
    }

    const Sigma* unit(Qualifier q = Qualifier::Unrestricted) { return unit_[size_t(q)]; }
    const Def* sigma(Defs defs, Debug dbg = {}) {
        return sigma(defs, meet(defs), dbg);
    }
    const Def* sigma(Defs, Qualifier q, Debug dbg = {});
    Sigma* sigma(size_t num_ops, const Def* type, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, type, num_ops, dbg);
    }
    Sigma* sigma_type(size_t num_ops, Debug dbg = {}) {
        return sigma_type(num_ops, Qualifier::Unrestricted, dbg);
    }
    Sigma* sigma_type(size_t num_ops, Qualifier q, Debug dbg = {}) {
        return sigma(num_ops, star(q), dbg);
    }
    Sigma* sigma_kind(size_t num_ops, Debug dbg = {}) {
        return sigma_kind(num_ops, Qualifier::Unrestricted, dbg);
    }
    Sigma* sigma_kind(size_t num_ops, Qualifier q, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, num_ops, q, dbg);
    }
    const Tuple* tuple0(Qualifier q = Qualifier::Unrestricted) { return tuple0_[size_t(q)]; }
    const Def* tuple(Defs defs, Debug dbg = {}) {
        return tuple(sigma(types(defs), dbg), defs, dbg);
    }
    const Def* tuple(Defs defs, Qualifier type_q, Debug dbg = {}) {
        return tuple(sigma(types(defs), type_q, dbg), defs, dbg);
    }
    const Def* tuple(const Def* type, Defs defs, Debug dbg = {});
    const Def* extract(const Def* def, const Def* index, Debug dbg = {});
    const Def* extract(const Def* def, size_t index, Debug dbg = {});

    const Def* intersection(Defs defs, Debug dbg = {}) {
        return intersection(defs, meet(defs), dbg);
    }
    const Def* intersection(Defs defs, Qualifier q, Debug dbg = {});
    const Def* pick(const Def* type, const Def* def, Debug dbg = {});

    const Def* variant(Defs defs, Debug dbg = {}) {
        return variant(defs, meet(defs), dbg);
    }
    const Def* variant(Defs defs, Qualifier q, Debug dbg = {});
    Variant* variant(size_t num_ops, const Def* type, Debug dbg = {}) {
        assert(num_ops > 1 && "It should not be necessary to build empty/unary variants.");
        return insert<Variant>(num_ops, *this, type, num_ops, dbg);
    }
    const Def* any(const Def* type, const Def* def, Debug dbg = {});
    const Def* match(const Def* def, Defs handlers, Debug dbg = {});

    const Def* singleton(const Def* def, Debug dbg = {});

    const DefSet& defs() const { return defs_; }

    friend void swap(WorldBase& w1, WorldBase& w2) {
        using std::swap;
        swap(w1.defs_, w2.defs_);
        w1.fix();
        w2.fix();
    }

private:
    void fix() { for (auto def : defs_) def->world_ = this; }

    bool too_many_affine_uses(Defs defs) {
        for (auto def : defs) {
            if (def->is_term() && def->type()->is_affine() && def->num_uses() > 0)
                return true;
        }
        return false;
    }

protected:
    template<class T, class... Args>
    const T* unify(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        assert(!def->is_nominal());
        auto p = defs_.emplace(def);
        if (p.second) {
            def->wire_uses();
            return def;
        }

        --Def::gid_counter_;
        dealloc(def);
        return static_cast<const T*>(*p.first);
    }

    template<class T, class... Args>
    T* insert(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        auto p = defs_.emplace(def);
        assert_unused(p.second);
        return def;
    }

    struct Page {
        static const size_t Size = 1024 * 1024 - sizeof(std::unique_ptr<int>); // 1MB - sizeof(next)
        std::unique_ptr<Page> next;
        char buffer[Size];
    };

    static bool alloc_guard_;

    template<class T, class... Args>
    T* alloc(size_t num_ops, Args&&... args) {
        static_assert(sizeof(Def) == sizeof(T), "you are not allowed to introduce any additional data in subclasses of Def");
#ifndef NDEBUG
        assert(alloc_guard_ = !alloc_guard_ && "you are not allowed to recursively invoke alloc");
#endif
        size_t num_bytes = sizeof(T) + sizeof(const Def*) * num_ops;
        assert(num_bytes < Page::Size);

        if (buffer_index_ + num_bytes >= Page::Size) {
            auto page = new Page;
            cur_page_->next.reset(page);
            cur_page_ = page;
            buffer_index_ = 0;
        }

        auto result = new (cur_page_->buffer + buffer_index_) T(args...);
        buffer_index_ += num_bytes;
        assert(buffer_index_ % alignof(T) == 0);

#ifndef NDEBUG
        alloc_guard_ = !alloc_guard_;
#endif
        return result;
    }

    template<class T>
    void dealloc(const T* def) {
        size_t num_bytes = sizeof(T) + def->num_ops() * sizeof(const Def*);
        def->~T();
        if (ptrdiff_t(buffer_index_ - num_bytes) > 0) // don't care otherwise
            buffer_index_-= num_bytes;
        assert(buffer_index_ % alignof(T) == 0);
    }

    std::unique_ptr<Page> root_page_;
    Page* cur_page_;
    size_t buffer_index_ = 0;
    DefSet defs_;
    std::array<const Universe*, 4> universe_;
    std::array<const Star*, 4> star_;
    std::array<const Sigma*, 4> unit_;
    std::array<const Tuple*, 4> tuple0_;
    const Axiom* arity_kind_;
};

class World : public WorldBase {
public:
    World();

    //@{ types and type constructors
    const Axiom* type_bool(Qualifier q = Qualifier::Unrestricted) { return type_bool_[size_t(q)]; }
    const Axiom* type_nat(Qualifier q = Qualifier::Unrestricted) { return type_nat_[size_t(q)]; }
    const Axiom* type_int(Qualifier q = Qualifier::Unrestricted) { return type_int_[size_t(q)]; }
    const Def* type_int(const Def* width, const Def* flags, Qualifier q = Qualifier::Unrestricted) {
        return app(type_int(q), {width, flags});
    }
    const Def* type_int(int64_t width, ITypeFlags flags, Qualifier q = Qualifier::Unrestricted) {
        auto f = val_nat(int64_t(flags));
        return app(type_int(q), {val_nat(width), f});
    }

    const Axiom* type_real() { return type_real_; }
    const Def* type_real(const Def* width, const Def* fast) { return app(type_real(), {width, fast}); }

    const Axiom* type_mem() { return type_mem_; }

    const Axiom* type_ptr() { return type_ptr_; }
    const Def* type_ptr(const Def* pointee, const Def* addr_space, Debug dbg = {}) {
        return app(type_ptr_, {pointee, addr_space}, dbg);
    }
    const Def* type_ptr(const Def* pointee, Debug dbg = {}) { return type_ptr(pointee, val_nat_0(), dbg); }

#define CODE(x, y) \
    const App* type_ ## x(Qualifier q = Qualifier::Unrestricted) { return type_ ## x ## _[size_t(q)]; }
    THORIN_I_TYPE(CODE)
    //THORIN_R_TYPE(CODE)
#undef CODE
    //@}

    //@{ values
    const Axiom* val_nat(int64_t val, Qualifier q = Qualifier::Unrestricted) {
        return assume(type_nat(q), {val}, {std::to_string(val)});
    }
    const Axiom* val_nat_0(Qualifier q = Qualifier::Unrestricted) { return val_nat_0_[size_t(q)]; }
    const Axiom* val_nat_1(Qualifier q = Qualifier::Unrestricted) { return val_nat_[0][size_t(q)]; }
    const Axiom* val_nat_2(Qualifier q = Qualifier::Unrestricted) { return val_nat_[1][size_t(q)]; }
    const Axiom* val_nat_4(Qualifier q = Qualifier::Unrestricted) { return val_nat_[2][size_t(q)]; }
    const Axiom* val_nat_8(Qualifier q = Qualifier::Unrestricted) { return val_nat_[3][size_t(q)]; }
    const Axiom* val_nat_16(Qualifier q = Qualifier::Unrestricted) { return val_nat_[4][size_t(q)]; }
    const Axiom* val_nat_32(Qualifier q = Qualifier::Unrestricted) { return val_nat_[5][size_t(q)]; }
    const Axiom* val_nat_64(Qualifier q = Qualifier::Unrestricted) { return val_nat_[6][size_t(q)]; }

    const Axiom* val_bool(bool val, Qualifier q = Qualifier::Unrestricted) { return val_bool_[size_t(val)][size_t(q)]; }
    const Axiom* val_bool_bot(Qualifier q = Qualifier::Unrestricted) { return val_bool_[0][size_t(q)]; }
    const Axiom* val_bool_top(Qualifier q = Qualifier::Unrestricted) { return val_bool_[1][size_t(q)]; }

#define CODE(x, y) \
    const Axiom* val_ ## x(y val, Qualifier q = Qualifier::Unrestricted) { \
        return assume(type_ ## x(q), {val}, {std::to_string(val)}); \
    }
    THORIN_I_TYPE(CODE)
    //THORIN_R_TYPE(CODE)
#undef CODE
    //@}

    //@{ arithmetic operations
#define CODE(x) \
    const Axiom* op_ ## x() { return op_ ## x ## _; } \
    const App* op_ ## x(const Def*, const Def*) { return nullptr; }
    THORIN_I_ARITHOP(CODE)
    //THORIN_R_ARITHOP(CODE)
#undef CODE
    //@}

    //@{ relational operations
    // TODO
    //@}

    //@{ tuple operations
    const Axiom* op_insert() { return op_insert_; }
    const Def* op_insert(const Def* def, const Def* index, const Def* val, Debug dbg = {});
    const Def* op_insert(const Def* def, size_t index, const Def* val, Debug dbg = {});
    const Axiom* op_lea() { return op_lea_; }
    const Def* op_lea(const Def* ptr, const Def* index, Debug dbg = {});
    const Def* op_lea(const Def* ptr, size_t index, Debug dbg = {});
    //@}

    //@{ memory operations
    // TODO
    //@}
private:
    std::array<const Axiom*, 4> type_nat_;
    std::array<const Axiom*, 4> val_nat_0_;
    std::array<std::array<const Axiom*, 4>, 7> val_nat_;
    std::array<const Axiom*, 4> type_bool_;
    std::array<std::array<const Axiom*, 4>, 2> val_bool_;
    std::array<const Axiom*, 4> type_int_;
    const Axiom* type_real_;
    const Axiom* type_mem_;
    const Axiom* type_frame_;
    const Axiom* type_ptr_;
    const Pi* type_icmpop_;
    const Pi* type_rcmpop_;
    const Pi* type_iarithop_;
    const Pi* type_rarithop_;
    const Axiom* op_lea_;
    const Axiom* op_insert_;

#define CODE(x) \
    const Axiom* op_ ## x ## _; \
    const App* op_ ## x ## s_[size_t(IType::Num)];
    THORIN_I_ARITHOP(CODE)
#undef CODE

#define CODE(x) \
    const Axiom* op_ ## x ## _; \
    const App* op_ ## x ## s_[size_t(IType::Num)];
    THORIN_R_ARITHOP(CODE)
#undef CODE

    union {
        struct {
#define CODE(x, y) \
            const std::array<const App*, 4> type_ ## x ## _;
            THORIN_I_TYPE(CODE)
#undef CODE
        };
        std::array<const std::array<const App*,  size_t(IType::Num)>, 4> type_ints_;
    };

    union {
        struct {
#define CODE(x, y) \
            const App* type_ ## x ## _;
            THORIN_R_TYPE(CODE)
#undef CODE
        };
        const App* type_reals_[size_t(RType::Num)];
    };

    const Axiom* op_icmp_;
    union {
        struct {
#define CODE(x) \
            const App* op_ ## icmp_ ## x ## _;
            THORIN_I_REL(CODE)
#undef CODE
        };
        const App* op_icmps_[size_t(IRel::Num)];
    };

    const Axiom* op_rcmp_;
    union {
        struct {
#define CODE(x) \
            const App* op_ ## rcmp_ ## x ## _;
            THORIN_I_REL(CODE)
#undef CODE
        };
        const App* op_rcmps_[size_t(IRel::Num)];
    };
};

}

#endif

