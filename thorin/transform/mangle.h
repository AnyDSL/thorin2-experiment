#ifndef THORIN_TRANSFORM_MANGLE_H
#define THORIN_TRANSFORM_MANGLE_H

#include "thorin/analyses/scope.h"

namespace thorin {

class Mangler {
public:
    Mangler(const Scope& scope, Defs args, DefSet lift);

    const Scope& scope() const { return scope_; }
    World& world() const { return scope_.world(); }
    Cn* mangle();

private:
    World& world() { return world_; }
    //void mangle_body(Cn* ocn, Cn* ncn);
    const Def* mangle(const Def* odef);
    bool within(const Def* def) { return scope().contains(def) || lift_.contains(def); }

    World& world_;
    const Scope& scope_;
    Defs args_;
    DefSet lift_;
    Cn* old_entry_;
    Cn* new_entry_;
    Def2Def old2new_;
};


Cn* mangle(const Scope&, Defs args, DefSet lift);

inline Cn* drop(const Scope& scope, Defs args) {
    return mangle(scope, args, DefSet());
}

//inline Cn* lift(const Scope& scope, Defs defs) {
    //return mangle(scope, Array<const Def*>(scope.entry()->num_params()), defs);
//}

}

#endif
