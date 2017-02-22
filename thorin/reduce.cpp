#include "thorin/reduce.h"

#include "thorin/world.h"

namespace thorin {

typedef TaggedPtr<const Def, size_t> DefIndex;

struct DefIndexHash {
    static uint64_t hash(DefIndex s) { return hash_combine(hash_begin(s->gid()), uint32_t(s.index())); }
    static bool eq(DefIndex a, DefIndex b) { return a == b; }
    static DefIndex sentinel() { return DefIndex(nullptr, 0); }
};

//------------------------------------------------------------------------------

class Reducer {
public:
    Reducer(WorldBase& world, Defs args)
        : world_(world)
        , args_(args)
    {}

    size_t num_args() const { return args_.size(); }
    WorldBase& world() const { return world_; }
    const Def* reduce_structurals(const Def* def, size_t index = 0);
    void reduce_nominals();

private:
    WorldBase& world_;
    DefArray args_;
    thorin::HashMap<DefIndex, const Def*, DefIndexHash> map_;
    std::stack<DefIndex> nominals_;
};

void Reducer::reduce_nominals() {
    while (!nominals_.empty()) {
        auto subst = nominals_.top();
        nominals_.pop();
        if (auto new_def = find(map_, subst)) {
            if (new_def == subst || new_def->is_closed())
                continue;

            for (size_t i = 0, e = subst->num_ops(); i != e; ++i) {
                auto new_op = reduce_structurals(subst->op(i), subst.index() + subst->shift(i));
                const_cast<Def*>(new_def)->set(i, new_op);
            }
        }
    }
}

const Def* Reducer::reduce_structurals(const Def* old_def, size_t offset) {
    if (old_def->free_vars().none_begin(offset)) {
        map_[{old_def, offset}] = old_def;
        return old_def;
    }

    if (auto new_def = find(map_, {old_def, offset}))
        return new_def;

    auto new_type = reduce_structurals(old_def->type(), offset);

    if (old_def->is_nominal()) {
        auto new_def = old_def->stub(new_type); // TODO better location debug info for these
        map_[{old_def, offset}] = new_def;
        nominals_.emplace(old_def, offset);
        return new_def;
    }

    if (auto var = old_def->isa<Var>()) {
        // Is var within our args? - Map index() back into the original argument array.
        if (offset <= var->index() && var->index() < offset + num_args()) {
            // remember that the lowest index corresponds to the last element in args due to De Bruijn's way of counting
            size_t arg_index = num_args() - (var->index() - offset) - 1;
            if (new_type != args_[arg_index]->type())
                return world().error(new_type); // use the expected type, not the one provided by the arg
            return args_[arg_index];
        }

        // is var not free? - keep index, substitute type
        if (var->index() < offset)
            return world().var(new_type, var->index(), var->debug());

        // this var is free - shift by num_args but inline a sole tuple arg if applicable
        auto shift = num_args() == 1 && args_[0]->isa<Tuple>() ? args_[0]->num_ops() : num_args();
        return world().var(var->type(), var->index() - shift, var->debug());
    }

    // rebuild all other defs
    DefArray new_ops(old_def->num_ops());
    for (size_t i = 0, e = old_def->num_ops(); i != e; ++i) {
        auto new_op = reduce_structurals(old_def->op(i), offset + old_def->shift(i));
        new_ops[i] = new_op;
    }

    auto new_def = old_def->rebuild(world(), new_type, new_ops);
    return map_[{old_def, offset}] = new_def;
}

//------------------------------------------------------------------------------

const Def* reduce(const Def* def, Defs args, size_t index) {
    return reduce(def, args, [&] (const Def*) {}, index);
}

const Def* reduce(const Def* def, Defs args, std::function<void(const Def*)> f, size_t index) {
    Reducer reducer(def->world(), args);
    auto result = reducer.reduce_structurals(def, index);
    f(result);
    reducer.reduce_nominals();
    return result;
}

const Def* flatten(const Def* body, Defs args) {
    auto& w = body->world();
    auto t = w.tuple(DefArray(args.size(), [&](auto i) { return w.var(args[i], args.size()-1-i); }));
    return reduce(body, {t});
}

}
