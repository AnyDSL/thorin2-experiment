#ifndef THORIN_MATCHERS_TYPES_H
#define THORIN_MATCHERS_TYPES_H

#include <experimental/optional>

#include "thorin/world.h"

namespace thorin {

class Matcher {
public:
    Matcher(const Def* def)
        : matches_(true), def_(def)
    {}

    std::experimental::optional<Qualifier> const_qualifier() const {
        if (auto q = qualifier()->isa<Axiom>())
            return q->box().get_qualifier();
        return {};
    }

    const Def* def() const { return def_; }
    bool matches() const { return def() && matches_; }
    size_t num_ops() const { return def()->num_ops(); }
    const Def* op(size_t i) const { return def()->op(i); }
    Defs ops() const { return def()->ops(); }
    const Def* qualifier() const { return def()->qualifier(); }
    const Def* type() const { return def()->type(); }
    World& world() const { return static_cast<World&>(def()->world()); }

    bool operator==(const Def* def) const { return def_ == def; }
    explicit operator bool() const { return matches(); }

protected:
    bool matches_;

private:
    const Def* def_;
};

class Type : public Matcher {
public:
    Type(const Def* def)
        : Matcher(def)
    {
        matches_ = type()->isa<Star>();
    }
    bool is_variadic() const {
        assert(matches());
        return def()->isa<Variadic>();
    }
    bool is_const() const {
        assert(matches());
        auto d = def();
        if (auto var = d->isa<Variadic>()) {
            if(d->op(0)->template isa<Axiom>())
                d = var->body();
            else
                return false;
        }
        return std::all_of(d->ops().begin(), d->ops().end(),
                           [](auto op) { return op->template isa<Axiom>(); }); }
};

class PtrType : public Type {
public:
    PtrType(const Def* def)
        : Type(def)
    {
        auto app = def->isa<App>();
        matches_ &= app && app->callee() == world().type_ptr();
    }
    const Def* pointee() const { return op(1); }
    const Def* addr_space() const { return op(2); }
};

class ArrayType : public Type {
public:
    ArrayType(const Def* def)
        : Type(def)
    {
        auto var = def->isa<Variadic>();
        matches_ &= !var || var->is_homogeneous();
    }
    const Def* elem_type() {
        assert(matches());
        auto body = def();
        while (auto var = body->isa<Variadic>()) {
            body = var->body()->shift_free_vars(1);
        }
        return body;
    }
    const Def* arity() {
        assert(matches());
        std::vector<const Def*> arities;
        auto body = def();
        while (auto var = body->isa<Variadic>()) {
            arities.push_back(var->arity());
            body = var->body()->shift_free_vars(1);
        }
        return arities.empty() ? world().arity(1) : world().sigma(arities);
    }
};

// class PrimitiveType : public ArrayType {
// public:
//     PrimitiveType(const Def* def)
//         : ArrayType(def)
//     {
//         matches_ &= world().is_primitive_type(def);
//     }
// };

#define CODE(T, ir)                                                    \
class T_CAT(T, Type) : public ArrayType {                              \
public:                                                                \
    T_CAT(T, Type)(const Def* def)                                     \
        : ArrayType(def)                                               \
    {                                                                  \
        auto elem_t = elem_type();                                     \
        auto app = elem_t->isa<App>();                                 \
        matches_ &= app && app->callee() == world().T_CAT(type_,ir)(); \
    }                                                                  \
    const Def* flags() const { return op(2); }                         \
    const Def* width() const { return op(3); }                         \
    std::experimental::optional<T_CAT(ir,flags)> const_flags() const { \
        if (auto f = flags()->template isa<Axiom>())                   \
            return (T_CAT(ir,flags)) (f->box().get_s64());             \
        else return {};                                                \
    }                                                                  \
    std::experimental::optional<int64_t> const_width() const {         \
        if (auto w = width()->template isa<Axiom>())                   \
            return w->box().get_s64();                                 \
        else return {};                                                \
    }                                                                  \
};

CODE(Int, i)
CODE(Real, r)
#undef CODE

}

#endif
