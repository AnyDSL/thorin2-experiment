#ifndef THORIN_REDUCE_H
#define THORIN_REDUCE_H

#include "thorin/def.h"

namespace thorin {

class DefIndex {
public:
    DefIndex()
        : def_(nullptr)
        , index_(0)
    {}

    DefIndex(const Def* def, size_t index)
        : def_(def)
        , index_(index)
    {}

    const Def* def() const { return def_; }
    size_t index() const { return index_; }
    bool operator==(const DefIndex& b) const { return def_ == b.def_ && index_ == b.index_; }

private:
    const Def* def_;
    size_t index_;
};

class DefIndexHash {
public:
    static uint64_t hash(const DefIndex& s) {
        return hash_combine(hash_begin(), s.def()->gid(), s.index());
    }
    static bool eq(const DefIndex& a, const DefIndex& b) { return a == b; }
    static DefIndex sentinel() { return DefIndex(); }
};

typedef thorin::HashMap<DefIndex, const Def*, DefIndexHash> Substitutions;
typedef std::stack<DefIndex> NominalTodos;

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
    Substitutions map_;
    NominalTodos nominals_;
};

inline const Def* reduce(const Def* def, size_t index, Defs args) {
    return Reducer(def, index, args).reduce();
}

inline const Def* reduce(const Def* def, Defs args) {
    return reduce(def, 0, args);
}

}

#endif
