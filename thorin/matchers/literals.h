#ifndef THORIN_MATCHERS_LITERALS_H
#define THORIN_MATCHERS_LITERALS_H

#include "thorin/world.h"
#include "thorin/matchers/types.h"

namespace thorin {

template<typename T>
class Literal : public Matcher {
public:
    Literal(const Def* def)
        : Matcher(def)
    {
        if (!def->is_nominal()) {
            if (auto constant = def->isa<Axiom>()) {
                // TODO check type?
                matches_ = true;
                value_ = constant->box().get<T>();
            }
        }
    }

    bool is(const T& value) { return matches() && value == value_; }
    const T& value() { return value_; }

private:
    T value_;
};

template<typename T>
class IntLiteral : public Literal<T> {
public:
    IntLiteral(const Def* def)
        : Literal<T>(def)
    {}

    bool is_zero() { return this->is(0); }
    bool is_one() { return this->is(1); }
    bool is_allset() { return this->is(-1); }
    bool is_negative() { return this->value() < 0; }
};

template<typename T>
class RealLiteral : public Literal<T> {
public:
    RealLiteral(const Def* def)
        : Literal<T>(def)
    {}

    bool is_zero() { return this->is(0.0); }
    bool is_one() { return this->is(1.0); }
    bool is_negative() { return this->value() < 0; }
};

}

#endif
