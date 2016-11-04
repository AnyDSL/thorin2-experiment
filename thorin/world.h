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

    const Star* star() const { return star_; }
    const Var* var(const Def* type, int index, const std::string& name = "") { return unify(alloc<Var>(*this, type, index, name)); }
    const Var* var(Defs types, int index, const std::string& name = "") { return var(sigma(types), index, name); }
    const Assume* assume(const Def* type, const std::string& name = "") { return insert(alloc<Assume>(*this, type, name)); }
    const Lambda* lambda(Defs domains, const Def* body, const std::string& name = "");
    const Pi*     pi    (Defs domains, const Def* body, const std::string& name = "");
    const Lambda* lambda(const Def* domain, const Def* body, const std::string& name = "") { return lambda(Defs({domain}), body, name); }
    const Pi*     pi    (const Def* domain, const Def* body, const std::string& name = "") { return pi    (Defs({domain}), body, name); }
    const Def* app(const Def* callee, Defs args, const std::string& name = "");
    const Def* app(const Def* callee, const Def* arg, const std::string& name = "") { return app(callee, Defs({arg}), name); }
    const Def* tuple(const Def* type, Defs defs, const std::string& name = "");
    const Def* tuple(Defs defs, const std::string& name = "") { return tuple(sigma(types(defs), name), defs, name); }
    const Def* sigma(Defs, const std::string& name = "");
    Sigma* sigma(size_t num_ops, const std::string& name = "") { return insert(alloc<Sigma>(*this, num_ops, name)); }
    const Sigma* unit() { return sigma(Defs())->as<Sigma>(); }
    const Assume* nat() { return nat_; }
    const Def* extract(const Def* def, const Def* i);
    const Def* extract(const Def* def, int i) { return extract(def, assume(nat(), std::to_string(i))); }

    const DefSet& defs() const { return defs_; }

    friend void swap(World& w1, World& w2) {
        using std::swap;
        swap(w1.defs_, w2.defs_);
        w1.fix();
        w2.fix();
    }

private:
    void fix() { for (auto def : defs_) def->world_ = this; }

protected:
    template<class T>
    const T* unify(const T* def) {
        assert(!def->is_nominal());
        auto p = defs_.emplace(def);
        if (p.second)
            return def;

        def->unregister_uses();
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
        static const size_t Size = 1024 * 1024; // 1MB
        std::unique_ptr<Page> next;
        char buffer[Size];
    };

    template<class T, class... Args>
    T* alloc(Args&&... args) {
        size_t num_bytes = sizeof(T);
        assert(num_bytes < (1 << 16));
        assert(num_bytes % alignof(T) == 0);

        if (buffer_index_ + num_bytes >= Page::Size) {
            auto page = new Page;
            cur_page_->next.reset(page);
            cur_page_ = page;
            buffer_index_ = 0;
        }

        assert(buffer_index_ % alignof(T) == 0);
        auto ptr = cur_page_->buffer + buffer_index_;
        buffer_index_ += num_bytes;         // first reserve memory
        auto result = new (ptr) T(args...); // then construct: it may in turn invoke alloc
        return result;
    }

    template<class T>
    void dealloc(const T* def) {
        size_t num_bytes = sizeof(T);
        def->~T();
        if (int(buffer_index_ - num_bytes) > 0) // don't care otherwise
            buffer_index_-= num_bytes;
    }

    std::unique_ptr<Page> root_page_;
    Page* cur_page_;
    size_t buffer_index_ = 0;
    DefSet defs_;
    const Star* star_;
    const Assume* nat_;
};

}

#endif
