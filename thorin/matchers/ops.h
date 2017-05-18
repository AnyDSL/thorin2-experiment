#ifndef THORIN_MATCHERS_OPS_H
#define THORIN_MATCHERS_OPS_H

#include "thorin/matchers/types.h"

namespace thorin {

class Primop : public Matcher {
public:
    Primop(const Def* def)
        : Matcher(def)
    {
        matches_ = def->sort() == Def::Sort::Term;
    }

};

class LEA : public Primop {
public:
    LEA(const Def* def)
        : Primop(def)
    {
        auto app = def->isa<App>();
        matches_ &= app && app->callee() == world().op_lea();
    }

    const Def* ptr() const { return op(1); }
    const Def* index() const { return op(2); }
    PtrType type() const { return def()->type(); }
    PtrType ptr_type() const { return ptr()->type(); } ///< Returns the PtrType from @p ptr().
    const Def* ptr_pointee() const { return ptr_type().pointee(); } ///< Returns the type referenced by @p ptr().
};

}

#endif
