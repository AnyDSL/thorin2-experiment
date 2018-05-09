#include "thorin/check.h"
#include "thorin/world.h"
#include "thorin/transform/reduce.h"

namespace thorin {

void TypeCheck::check(const Def* def, DefVector& types) {
    if (def->num_ops() == 0)
        return;
    if (def->free_vars().none())
        types = {};
    if (done_.emplace(DefArray(types), def).second)
        def->check(*this, types);
}

void fcheck(TypeCheck& tc, const Def* def, DefVector& types) {
    if (def->free_vars().any())
        tc.check(def, types);
}

void dependent_check(TypeCheck& tc, DefVector& types, Defs defs, Defs bodies) {
    auto old_size = types.size();
    for (auto def : defs) {
        fcheck(tc, def, types);
        types.emplace_back(def);
    }

    for (auto def : bodies)
        fcheck(tc, def, types);

    types.erase(types.begin() + old_size, types.end());
}

void Def::check() const {
    assert(free_vars().none());
    world().check(this);
}

void Def::check(TypeCheck& tc, DefVector& types) const {
    if (!is_universe())
        fcheck(tc, type(), types);
    for (auto op : ops())
        fcheck(tc, op, types);
}

void Lambda::check(TypeCheck& tc, DefVector& types) const {
    // do Pi type check inline to reuse built up environment
    fcheck(tc, type()->type(), types);
    dependent_check(tc, types, {domain()}, {type()->codomain(), body()});
}

void Pack::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    dependent_check(tc, types, {arity()}, {body()});
}

void Pi::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    dependent_check(tc, types, {domain()}, {codomain()});
}

void Sigma::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    dependent_check(tc, types, ops(), Defs());
}

void Var::check(TypeCheck&, DefVector& types) const {
    // free variables are always typed correctly / will be checked later
    if (types.size() <= index())
        return;
    auto reverse_index = types.size() - 1 - index();
    auto shifted_type = shift_free_vars(type(), -index() - 1);
    auto env_type = types[reverse_index];
    if (env_type != shifted_type)
        world().errorf("the shifted type {} of variable {} does not match the type {} declared by the binder",
                shifted_type, index(), types[reverse_index]);
}

void Variadic::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    dependent_check(tc, types, {arity()}, {body()});
}

}
