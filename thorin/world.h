#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <string>
#include "thorin/def.h"

namespace thorin {

class World {
public:
    struct DefHash { uint64_t operator()(const Def* def) const { return def->hash(); } };
    struct DefEqual { bool operator()(const Def* d1, const Def* d2) const { return d2->equal(d1); } };
    typedef HashSet<const Def*, DefHash, DefEqual> DefSet;

    World& operator=(const World&);
    World(const World&);

    World();
    virtual ~World() { for (auto def : defs_) delete def; }

    const Star* star() const { return star_; }
    const Error* error() const { return error_; }

    const Var* var(const Def* type, int index, const std::string& name = "") {
        return var(type, index, Qualifier::Unrestricted, name);
    }
    const Var* var(const Def* type, int index, Qualifier::URAL q, const std::string& name = "") {
        return unify(new Var(*this, type, index, q, name));
    }
    const Var* var(Defs types, int index, const std::string& name = "") {
        return var(sigma(types), index, Qualifier::Unrestricted, name);
    }
    const Var* var(Defs types, int index, Qualifier::URAL q, const std::string& name = "") {
        return var(sigma(types), index, q, name);
    }

    const Assume* assume(const Def* type, const std::string& name = "") {
        return assume( type, Qualifier::Unrestricted, name);
    }
    const Assume* assume(const Def* type, Qualifier::URAL q, const std::string& name = "") {
        return insert(new Assume(*this, type, q, name));
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
    const Lambda* typed_lambda(const Def* type, const Def* body, const std::string& name = "") {
        return unify(new Lambda(*this, type, body, name));
    }

    const Pi*  pi(const Def* domain, const Def* body, const std::string& name = "") {
        return pi(Defs({domain}), body, name);
    }
    const Pi*  pi(const Def* domain, const Def* body, Qualifier::URAL q,
                  const std::string& name = "") {
        return pi(Defs({domain}), body, q, name);
    }
    const Pi*  pi(Defs domains, const Def* body, const std::string& name = "") {
        return pi(domains, body, Qualifier::Unrestricted, name);
    }
    const Pi*  pi(Defs domains, const Def* body, Qualifier::URAL q, const std::string& name = "");

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
        return insert(new Sigma(*this, num_ops, q, name));
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
            if (def->type()->is_affine() && def->num_uses() > 0)
                return true;
        }
        return false;
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
    const Star* star_;
    const Error* error_;
    const Array<const Assume*> nat_;
};

}

#endif
