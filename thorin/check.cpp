#include "thorin/check.h"
#include "thorin/world.h"
#include "thorin/transform/reduce.h"

namespace thorin {

std::ostream& operator<<(std::ostream& os, const Occurrences& s) {
    return os << "{" << (s.zero ? "0" : "") << (s.one ? "1" : "") << (s.many ? "M" : "") << "}";
}

void merge_occurrences(std::vector<Occurrences>& tgt, const Array<Occurrences>& occ,
                       std::function<Occurrences(const Occurrences&, const Occurrences&)> both,
                       std::function<Occurrences(const Occurrences&)> rest) {
    auto min_size = std::min(tgt.size(), occ.size());
    for (size_t i = 0; i != min_size; ++i)
        tgt[i] = both(tgt[i], occ[i]);
    if (tgt.size() < occ.size()) {
        tgt.resize(occ.size());
        auto iter = occ.begin() + min_size;
        for (size_t i = min_size; i != occ.size(); ++i, ++iter)
            tgt[i] = rest(*iter);
    } else {
        auto iter = tgt.begin() + min_size;
        for (size_t i = min_size; i != occ.size(); ++i, ++iter)
            tgt[i] = rest(*iter);
    }
}

void add_occurrences(std::vector<Occurrences>& tgt, const Array<Occurrences>& b) {
    merge_occurrences(tgt, b, Occurrences::add, [](auto o) { return Occurrences::add(o, {}); });
}

void join_occurrences(std::vector<Occurrences>& tgt, const Array<Occurrences>& b) {
    merge_occurrences(tgt, b, Occurrences::join, Occurrences::with_zero_set);
}

void TypeCheck::check(const Def* def, DefVector& types) {
    if (def->free_vars().none())
        types = {};
    if (done_.emplace(DefArray(types), def).second)
        def->check(*this, types);
}

void fcheck(TypeCheck& tc, const Def* def, DefVector& types) {
    if (def->free_vars().any()) {
        tc.check(def, types);
    }
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

void Extract::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    fcheck(tc, index(), types);
    fcheck(tc, scrutinee(), types);
    std::vector<Occurrences> occ;
    if (auto tuple = scrutinee()->isa<Tuple>()) {
        auto first = tc.occurrences[tuple->op(0)];
        occ.insert(occ.begin(), first.begin(), first.end());
        for (auto op : tuple->ops().skip_front()) {
            auto curr = tc.occurrences[op];
            join_occurrences(occ, curr);
        }
    } else {
        // scrutinee is a pack, variable or more complex expression
        auto scrutinee_occ = tc.occurrences[scrutinee()];
        occ.insert(occ.begin(), scrutinee_occ.begin(), scrutinee_occ.end());
    }
    add_occurrences(occ, tc.occurrences[index()]);
    tc.occurrences.emplace(this, occ);
}

void Lambda::check(TypeCheck& tc, DefVector& types) const {
    // do Pi type check inline to reuse built up environment
    fcheck(tc, type()->type(), types);
    dependent_check(tc, types, {domain()}, {type()->codomain(), body()});
    auto occ = tc.occurrences[body()];
    auto dom_qual = domain()->qualifier();
    if (!occ.empty()) {
        if (auto q = dom_qual->isa<Qualifier>()) {
            // DO NOT "OPTIMIZE" this condition, qualifiers are partially ordered
            if (!(q->qualifier_tag() <= occ[0].to_qualifier()))
                world().errorf("usage {} of bound variable in {} would imply qualifier {}, but has {}",
                              occ[0], this, occ[0].to_qualifier(), domain()->qualifier());
        } else if (occ[0].to_qualifier() != QualifierTag::Linear) {
            world().errorf("usage {} of bound variable in {} needs to be linear (exactly once), as it has qualifier {}",
                           occ[0], this, domain()->qualifier());

        }
        occ = occ.skip_front();
    } else if (auto q = dom_qual->isa<Qualifier>()) {
        if (q->qualifier_tag() == QualifierTag::Relevant)
            world().errorf("bound variable unused in {}, but needs to be used at least once as it has qualifier {}",
                           this, domain()->qualifier());
        else if (q->qualifier_tag() == QualifierTag::Linear)
            world().errorf("bound variable unused in {}, but needs to be used exactly once as it has qualifier {}",
                           this, domain()->qualifier());
    } else {
        world().errorf("bound variable unused in {}, but needs to be used exactly once as it has qualifier {}",
                       this, domain()->qualifier());
    }
    tc.occurrences.emplace(this, occ);
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

void Tuple::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    std::vector<Occurrences> occ;
    if (num_ops() > 0) {
        fcheck(tc, op(0), types);
        auto first = tc.occurrences[op(0)];
        occ.insert(occ.begin(), first.begin(), first.end());
    }
    for (auto op : ops().skip_front()) {
        fcheck(tc, op, types);
        auto curr = tc.occurrences[op];
        add_occurrences(occ, curr);
    }
    tc.occurrences.emplace(this, Array<Occurrences>(occ));
}

void Var::check(TypeCheck& tc, DefVector& types) const {
    // free variables are always typed correctly / will be checked later
    if (types.size() <= index())
        return;
    auto reverse_index = types.size() - 1 - index();
    auto shifted_type = shift_free_vars(type(), -index() - 1);
    auto env_type = types[reverse_index];
    if (env_type != shifted_type)
        world().errorf("the shifted type {} of variable {} does not match the type {} declared by the binder",
                shifted_type, index(), types[reverse_index]);
    Array<Occurrences> one(index() + 1);
    one[index()] = Occurrences(0, 1, 0);
    tc.occurrences.emplace(this, one);
}

void Variadic::check(TypeCheck& tc, DefVector& types) const {
    fcheck(tc, type(), types);
    dependent_check(tc, types, {arity()}, {body()});
}

}
