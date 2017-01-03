#ifndef THORIN_REDUCE_H
#define THORIN_REDUCE_H

#include "thorin/def.h"

namespace thorin {

typedef TaggedPtr<const Def, size_t> DefIndex;

struct DefIndexHash {
    static uint64_t hash(DefIndex s) {
        return hash_combine(hash_begin(), s->gid(), s.index());
    }
    static bool eq(DefIndex a, DefIndex b) { return a == b; }
    static DefIndex sentinel() { return DefIndex(nullptr, 0); }
};

class Reducer {
public:
    Reducer(const Def* def, size_t index, Defs args)
        : world_(def->world())
        , def_(def)
        , index_(index)
        , args_(args)
    {}
    Reducer(const Def* def, Defs args)
        : Reducer(def, 0, args)
    {}

    World& world() const { return world_; }
    const Def* reduce();
    const Def* reduce_up_to_nominals();
    void reduce_nominals();

private:
    const Def* reduce(const Def* def, size_t shift);
    const Def* var_reduce(const Var* var, size_t shift, const Def* new_type);
    const Def* rebuild_reduce(const Def* def, size_t shift, const Def* new_type);

    World& world_;
    const Def* def_;
    size_t index_;
    Array<const Def*> args_;
    thorin::HashMap<DefIndex, const Def*, DefIndexHash> map_;
    std::stack<DefIndex> nominals_;
};

inline const Def* reduce(const Def* def, size_t index, Defs args) {
    return Reducer(def, index, args).reduce();
}

inline const Def* reduce(const Def* def, Defs args) {
    return reduce(def, 0, args);
}

}

#endif
