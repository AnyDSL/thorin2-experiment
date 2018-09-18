#ifndef THORIN_TRANSFORM_MANGLE_H
#define THORIN_TRANSFORM_MANGLE_H

#include "thorin/analyses/scope.h"

namespace thorin {

class Mangler {
public:
    Mangler(const Scope& scope, Defs args, DefSet lift);

    const Scope& scope() const { return scope_; }
    World& world() const { return scope_.world(); }
    Lambda* mangle();

private:
    World& world() { return world_; }
    //void mangle_body(Lambda* olambda, Lambda* nlambda);
    const Def* mangle(const Def* odef);
    bool within(const Def* def) { return scope().contains(def) || lift_.contains(def); }

    World& world_;
    const Scope& scope_;
    Defs args_;
    DefSet lift_;
    Lambda* old_entry_;
    Lambda* new_entry_;
    Def2Def old2new_;
};


Lambda* mangle(const Scope&, Defs args, DefSet lift);

inline Lambda* drop(const Scope& scope, Defs args) {
    return mangle(scope, args, DefSet());
}

inline Lambda* drop(Lambda* lambda, Defs args) {
    Scope scope(lambda);
    return drop(scope, args);
}

inline Lambda* clone(Lambda* lambda) {
    Scope scope(lambda);
    size_t num = get_constant_arity(lambda->domain()).value_or(1);
    return mangle(scope, DefArray(num), {});
}

//inline Lambda* lift(const Scope& scope, Defs defs) {
    //return mangle(scope, Array<const Def*>(scope.entry()->num_params()), defs);
//}

}

#endif
