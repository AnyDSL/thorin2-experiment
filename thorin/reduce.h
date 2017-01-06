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
    Reducer(const Def* def, Defs args)
        : world_(def->world())
        , def_(def)
        , args_(args)
    {}

    World& world() const { return world_; }
    const Def* reduce(size_t index);
    const Def* reduce_up_to_nominals(size_t index = 0);
    void reduce_nominals();

private:
    const Def* reduce(const Def* def, size_t shift);
    const Def* var_reduce(const Var* var, const Def* new_type, size_t shift);
    const Def* rebuild(const Def* def, const Def* new_type, size_t shift);

    World& world_;
    const Def* def_;
    size_t index_;
    Array<const Def*> args_;
    thorin::HashMap<DefIndex, const Def*, DefIndexHash> map_;
    std::stack<DefIndex> nominals_;
};

inline const Def* reduce(const Def* def, Defs args, size_t index = 0) {
    return Reducer(def, args).reduce(index);
}

}

#endif
