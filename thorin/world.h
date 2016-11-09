#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <memory>
#include <string>

#include "thorin/def.h"

namespace thorin {

class World {
public:
    struct DefHash { uint64_t operator()(const Def* def) const { return def->hash(); } };
    struct DefEqual { bool operator()(const Def* d1, const Def* d2) const { return d2->equal(d1); } };
    typedef HashSet<const Def*, DefHash, DefEqual> DefSet;

    World& operator=(const World&) = delete;
    World(const World&) = delete;

    World();
    virtual ~World() { for (auto def : defs_) def->~Def(); }

    const Star* star(Qualifier::URAL q = Qualifier::Unrestricted) const { return star_[q]; }
    const Error* error() const { return error_; }

    const Var* var(const Def* type, int index, const std::string& name = "") {
        return unify(alloc<Var>(0, *this, type, index, name));
    }
    const Var* var(Defs types, int index, const std::string& name = "") {
        return var(sigma(types), index, name);
    }

    const Assume* assume(const Def* type, const std::string& name = "") {
        return assume(type, type ? type->qualifier() : Qualifier::Unrestricted, name);
    }
    // Needed for differently qualified *s
    const Assume* assume(const Def* type, Qualifier::URAL q, const std::string& name = "") {
        assert(!type || type->qualifier() == q);
        return insert(alloc<Assume>(0, *this, type, q, name));
    }

    const Lambda* lambda(const Def* domain, const Def* body, const std::string& name = "") {
        return lambda(domain, body, Qualifier::Unrestricted, name);
    }
    const Lambda* lambda(const Def* domain, const Def* body, Qualifier::URAL type_q,
                         const std::string& name = "") {
        return lambda(Defs({domain}), body, type_q, name);
    }
    const Lambda* lambda(Defs domains, const Def* body, const std::string& name = "") {
        return lambda(domains, body, Qualifier::Unrestricted, name);
    }
    const Lambda* lambda(Defs domains, const Def* body, Qualifier::URAL type_q,
                         const std::string& name = "") {
        return typed_lambda(pi(domains, body->type(), type_q), body, name);
    }
    const Lambda* typed_lambda(const Pi* type, const Def* body, const std::string& name = "") {
        return unify(alloc<Lambda>(type->domains().size() + 1, *this, type, body, name));
    }

    const Pi* pi(const Def* domain, const Def* body, const std::string& name = "") {
        return pi(Defs({domain}), body, name);
    }
    const Pi* pi(const Def* domain, const Def* body, Qualifier::URAL q,
                  const std::string& name = "") {
        return pi(Defs({domain}), body, q, name);
    }
    const Pi* pi(Defs domains, const Def* body, const std::string& name = "") {
        return pi(domains, body, Qualifier::Unrestricted, name);
    }
    const Pi* pi(Defs domains, const Def* body, Qualifier::URAL q, const std::string& name = "");

    const Def* app(const Def* callee, Defs args, const std::string& name = "");
    const Def* app(const Def* callee, const Def* arg, const std::string& name = "") {
        return app(callee, Defs({arg}), name);
    }

    const Def* tuple(const Def* type, Defs defs, const std::string& name = "");
    const Def* tuple(Defs defs, const std::string& name = "") {
        return tuple(sigma(types(defs), name), defs, name);
    }

    const Def* sigma(Defs defs, const std::string& name = "") {
        return sigma(defs, Qualifier::meet(defs), name);
    }
    const Def* sigma(Defs, Qualifier::URAL q, const std::string& name = "");
    Sigma* sigma(size_t num_ops, Qualifier::URAL q = Qualifier::Unrestricted,
                 const std::string& name = "") {
        return insert(alloc<Sigma>(num_ops, *this, num_ops, q, name));
    }
    const Sigma* unit() { return sigma(Defs())->as<Sigma>(); }

    const Assume* nat(Qualifier::URAL q = Qualifier::Unrestricted) { return nat_[q]; }
    const Def* extract(const Def* def, const Def* i);
    const Def* extract(const Def* def, int i) {
        return extract(def, assume(nat(), std::to_string(i)));
    }

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
    template<class T>
    const T* unify(const T* def) {
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

    template<class T>
    T* insert(T* def) {
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
        if (intptr_t(buffer_index_ - num_bytes) > 0) // don't care otherwise
            buffer_index_-= num_bytes;
        assert(buffer_index_ % alignof(T) == 0);
    }

    std::unique_ptr<Page> root_page_;
    Page* cur_page_;
    size_t buffer_index_ = 0;
    DefSet defs_;
    const Array<const Star*> star_;
    const Error* error_;
    const Array<const Assume*> nat_;
};

}

#endif
