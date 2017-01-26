#ifndef THORIN_TYPE_H
#define THORIN_TYPE_H

#include "thorin/world.h"

namespace thorin {

class Op {
public:
    Op(const Def* def)
        : app_(def->isa<App>())
    {}

    World& world() { return static_cast<World&>(app()->world()); }
    const App* app() const { return app_; }
    Defs args() const { return app()->args(); }
    const Def* arg(size_t i) const { return app()->arg(i); }
    size_t num_args() const { return app()->num_args(); }
    bool operator==(const Def* def) const { assert(app_); return app_ == def; }

protected:
    const App* app_;
};

class Type : public Op {
public:
    Type(const Def* def)
        : Op(def)
    {}

    explicit operator bool() { return app() && app()->sort() == Def::Sort::Type; }
};

class PtrType : public Type {
public:
    PtrType(const Def* def)
        : Type(def)
    {}

    const Def* pointee() const { return arg(0); }
    const Def* addr_space() const { return arg(1); }
    explicit operator bool() { return app() && app()->callee() == world().type_ptr(); }
};

}

#endif
