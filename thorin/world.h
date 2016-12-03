#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <memory>
#include <string>

#include "thorin/def.h"
#include "thorin/tables.h"

namespace thorin {

class World {
public:
    struct DefHash {
        static uint64_t hash(const Def* def) { return def->hash(); }
        static bool eq(const Def* d1, const Def* d2) { return d2->equal(d1); }
        static const Def* sentinel() { return (const Def*)(1); }
    };

    typedef HashSet<const Def*, DefHash> DefSet;

    World& operator=(const World&) = delete;
    World(const World&) = delete;

    World();
    virtual ~World() { for (auto def : defs_) def->~Def(); }

    const Universe* universe(Qualifier::URAL q = Qualifier::Unrestricted) const { return universe_[q]; }
    const Star* star(Qualifier::URAL q = Qualifier::Unrestricted) const { return star_[q]; }

    const Axiom* axiom(const Def* type, Debug dbg = {}) {
        return insert<Axiom>(0, *this, type, dbg);
    }

    const Axiom* assume_(const Def* type, Box box, Debug dbg = {}) {
        return unify<Axiom>(0, *this, type, box, dbg);
    }

    const Error* error(const Def* type) { return unify<Error>(0, *this, type); }
    const Var* var(Defs types, size_t index, Debug dbg = {}) {
        return var(sigma(types), index, dbg);
    }
    const Var* var(const Def* type, size_t index, Debug dbg = {}) {
        return unify<Var>(0, *this, type, index, dbg);
    }

    const Pi* pi(const Def* domain, const Def* body, Debug dbg = {}) {
        return pi(Defs({domain}), body, dbg);
    }
    const Pi* pi(Defs domains, const Def* body, Debug dbg = {}) {
        return pi(domains, body, Qualifier::Unrestricted, dbg);
    }
    const Pi* pi(const Def* domain, const Def* body, Qualifier::URAL q, Debug dbg = {}) {
        return pi(Defs({domain}), body, q, dbg);
    }
    const Pi* pi(Defs domains, const Def* body, Qualifier::URAL q, Debug dbg = {});
    const Lambda* lambda(const Def* domain, const Def* body, Debug dbg = {}) {
        return lambda(domain, body, Qualifier::Unrestricted, dbg);
    }
    const Lambda* lambda(const Def* domain, const Def* body, Qualifier::URAL type_q,
                         Debug dbg = {}) {
        return lambda(Defs({domain}), body, type_q, dbg);
    }
    const Lambda* lambda(Defs domains, const Def* body, Debug dbg = {}) {
        return lambda(domains, body, Qualifier::Unrestricted, dbg);
    }
    const Lambda* lambda(Defs domains, const Def* body, Qualifier::URAL type_q, Debug dbg = {}) {
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

    const Def* sigma(Defs defs, Debug dbg = {}) {
        return sigma(defs, Qualifier::meet(defs), dbg);
    }
    const Def* sigma(Defs, Qualifier::URAL q, Debug dbg = {});
    Sigma* sigma(size_t num_ops, const Def* type, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, type, num_ops, dbg);
    }
    Sigma* sigma_type(size_t num_ops, Debug dbg = {}) {
        return sigma_type(num_ops, Qualifier::Unrestricted, dbg);
    }
    Sigma* sigma_type(size_t num_ops, Qualifier::URAL q, Debug dbg = {}) {
        return sigma(num_ops, star(q), dbg);
    }
    Sigma* sigma_kind(size_t num_ops, Debug dbg = {}) {
        return sigma_kind(num_ops, Qualifier::Unrestricted, dbg);
    }
    Sigma* sigma_kind(size_t num_ops, Qualifier::URAL q, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, num_ops, q, dbg);
    }
    const Def* tuple(Defs defs, Debug dbg = {}) {
        return tuple(sigma(types(defs), dbg), defs, dbg);
    }
    const Def* tuple(Defs defs, Qualifier::URAL type_q, Debug dbg = {}) {
        return tuple(sigma(types(defs), type_q, dbg), defs, dbg);
    }
    const Def* tuple(const Def* type, Defs defs, Debug dbg = {});
    const Def* extract(const Def* def, size_t index, Debug dbg = {});

    const Def* intersection(Defs defs, Debug dbg = {}) {
        return intersection(defs, Qualifier::meet(defs), dbg);
    }
    const Def* intersection(Defs defs, Qualifier::URAL q, Debug dbg = {});
    const Def* all(Defs defs, Debug dbg = {});
    const Def* pick(const Def* type, const Def* def, Debug dbg = {});

    const Def* variant(Defs defs, Debug dbg = {}) {
        return variant(defs, Qualifier::meet(defs), dbg);
    }
    const Def* variant(Defs defs, Qualifier::URAL q, Debug dbg = {});
    const Def* any(const Def* type, const Def* def, Debug dbg = {});
    const Def* match(const Def* def, Defs handlers, Debug dbg = {});

    const Sigma* unit() { return sigma(Defs())->as<Sigma>(); }
    const Axiom* nat(Qualifier::URAL q = Qualifier::Unrestricted) { return nat_[q]; }
    const Axiom* boolean(Qualifier::URAL q = Qualifier::Unrestricted) { return boolean_[q]; }
    const Axiom* integer() { return integer_; }
    const Def* integer(const Def* width, const Def* sign, const Def* wrap) { return app(integer(), {width, sign, wrap}); }
    const Axiom* real() { return real_; }
    const Def* real(const Def* width, const Def* fast) { return app(real(), {width, fast}); }
    const Axiom* mem() const { return mem_; }

    // HACK
    const Axiom* nat0() { return axiom(nat(), {"0"}); }

    const Def* ptr(const Def* referenced_type, const Def* addr_space) {
        return app(ptr_, {referenced_type, addr_space});
    }
    const Def* ptr(const Def* referenced_type) { return ptr(referenced_type, nat0()); }

#define DECL(x) \
    const App* type_ ## x() const { return x ## _; } \
    const Axiom* literal_ ## x(/*x val*/) const { return nullptr; }
    THORIN_I_TYPE(DECL)
    THORIN_R_TYPE(DECL)
#undef DECL

#define DECL(x) \
    const Axiom* x() { return x ## _; } \
    const App* x(const Def*, const Def*) { return nullptr; }
    THORIN_I_ARITHOP(DECL)
    THORIN_R_ARITHOP(DECL)
#undef DECL

    const DefSet& defs() const { return defs_; }

    friend void swap(World& w1, World& w2) {
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

    template<class T, class... Args>
    T* alloc(size_t num_ops, Args&&... args) {
        static_assert(sizeof(Def) == sizeof(T), "you are not allowed to introduce any additional data in subclasses of Def");
#ifndef NDEBUG
        static bool guard = false;
        assert(guard = !guard && "you are not allowed to recursively invoke alloc");
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
        guard = !guard;
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
    const std::array<const Universe*, 4> universe_;
    const std::array<const Star*, 4> star_;
    const std::array<const Axiom*, 4> nat_;
    const std::array<const Axiom*, 4> boolean_;
    const Axiom* integer_;
    const Axiom* real_;
    const Axiom* mem_;
    const Axiom* frame_;
    const Axiom* ptr_;
    const Pi* iarithop_type_;
    const Pi* rarithop_type_;
    const Pi* icmpop_type_;
    const Pi* rcmpop_type_;

#define DECL(x) \
    const Axiom* x ## _; \
    const App* x ## s_[size_t(IType::Num)];
    THORIN_I_ARITHOP(DECL)
#undef DECL

#define DECL(x) \
    const Axiom* x ## _; \
    const App* x ## s_[size_t(IType::Num)];
    THORIN_R_ARITHOP(DECL)
#undef DECL

    union {
        struct {
#define DECL(x) \
            const App* x ## _;
            THORIN_I_TYPE(DECL)
#undef DECL
        };
        const App* integers_[size_t(IType::Num)];
    };

    union {
        struct {
#define DECL(x) \
            const App* x ## _;
            THORIN_R_TYPE(DECL)
#undef DECL
        };
        const App* reals_[size_t(RType::Num)];
    };

    const Axiom* icmp_;
    union {
        struct {
#define DECL(x) \
            const App* icmp_ ## x ## _;
            THORIN_I_REL(DECL)
#undef DECL
        };
        const App* icmps_[size_t(IRel::Num)];
    };

    const Axiom* rcmp_;
    union {
        struct {
#define DECL(x) \
            const App* rcmp_ ## x ## _;
            THORIN_I_REL(DECL)
#undef DECL
        };
        const App* rcmps_[size_t(IRel::Num)];
    };
};

}

#endif
