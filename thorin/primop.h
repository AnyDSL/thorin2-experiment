#ifndef THORIN_PRIMOP_H
#define THORIN_PRIMOP_H

#include "thorin/type.h"

namespace thorin {

class Primop : public Op {
public:
    Primop(const Def* def)
        : Op(def)
    {}

    Type type() const { return app()->type(); }
    explicit operator bool() { return app() && app()->sort() == Def::Sort::Term; }
};

class LEA : public Op {
public:
    LEA(const Def* def)
        : Op(def)
    {}

    const Def* ptr() const { return arg(0); }
    const Def* index() const { return arg(1); }
    PtrType type() const { return app()->type(); }
    PtrType ptr_type() const { return ptr()->type(); } ///< Returns the PtrType from @p ptr().
    const Def* ptr_pointee() const { return ptr_type().pointee(); } ///< Returns the type referenced by @p ptr().
    explicit operator bool() { return app() && app()->callee() == world().op_lea(); }
};

}

#endif
