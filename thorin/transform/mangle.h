#ifndef THORIN_TRANSFORM_MANGLE_H
#define THORIN_TRANSFORM_MANGLE_H

#include "thorin/analyses/scope.h"

namespace thorin {

struct Rewriter {
    const Def* instantiate(const Def* odef);
    Def2Def old2new;
};

class Mangler {
public:
    Mangler(const Scope& scope, const Def* arg, DefSet lift);

    const Scope& scope() const { return scope_; }
    const Def* def2def(const Def* def) { return find(old2new_, def); }
    Cn* mangle();

private:
    void mangle_body(Cn* ocn, Cn* ncn);
    Cn* mangle_head(Cn* ocn);
    const Def* mangle(const Def* odef);
    bool within(const Def* def) { return scope().contains(def) || defs_.contains(def); }

    World& world_;
    const Scope& scope_;
    const Def* arg_;
    DefSet lift_;
    Cn* old_entry_;
    Cn* new_entry_;
    DefSet defs_;
    Def2Def old2new_;
    std::vector<Cn*> new_cns_;
};


Cn* mangle(const Scope&, const Def* arg, Defs lift);

inline Cn* drop(const Scope& scope, const Def* arg) {
    return mangle(scope, arg, Array<const Def*>());
}

//inline Cn* lift(const Scope& scope, Defs defs) {
    //return mangle(scope, Array<const Def*>(scope.entry()->num_params()), defs);
//}

}

#endif
