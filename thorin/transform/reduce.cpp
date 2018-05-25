#include "thorin/transform/reduce.h"

#include "thorin/world.h"
#include "thorin/transform/mangle.h"

namespace thorin {

typedef TaggedPtr<const Def, size_t> DefIndex;

struct DefIndexHash {
    static uint64_t hash(DefIndex s) { return murmur3(uint64_t(s->gid()) << 32_u64 | uint64_t(s.index())); }
    static bool eq(DefIndex a, DefIndex b) { return a == b; }
    static DefIndex sentinel() { return DefIndex(nullptr, 0); }
};

//------------------------------------------------------------------------------

class Reducer {
public:
    Reducer(World& world, Defs args)
        : world_(world)
        , args_(args)
        , shift_(args.size())
    {}
    Reducer(World& world, int64_t shift)
        : world_(world)
        , args_()
        , shift_(shift)
    {}

    size_t shift() const { return shift_; }
    bool is_shift_only() const { return args_.empty(); }
    World& world() const { return world_; }
    const Def* reduce(const Def* def, size_t index = 0);

private:
    World& world_;
    DefArray args_;
    int64_t shift_;
    thorin::HashMap<DefIndex, const Def*, DefIndexHash> map_;
};

const Def* Reducer::reduce(const Def* old_def, size_t offset) {
    if (old_def->is_nominal() || old_def->free_vars().none_begin(offset)) {
        map_[{old_def, offset}] = old_def;
        return old_def;
    }

    if (auto new_def = find(map_, {old_def, offset}))
        return new_def;

    auto new_type = reduce(old_def->type(), offset);

    if (auto var = old_def->isa<Var>()) {
        if (offset > var->index())
            // var is not free - keep index, substitute type
            return world().var(new_type, var->index(), var->debug());

        // free variable
        if (!is_shift_only()) {
            // Is var within our args? - Map index() back into the original argument array.
            if (var->index() < offset + shift()) {
                // remember that the lowest index corresponds to the last element in args due to De Bruijn's
                // way of counting
                size_t arg_index = shift() - (var->index() - offset) - 1;
                assertf(new_type->assignable(args_[arg_index]), "cannot assign {} to {}", args_[arg_index], new_type);
                return shift_free_vars(args_[arg_index], offset);
            }
        }
        // shift by shift but inline a sole tuple arg if applicable
        auto total_shift = shift() == 1 && !is_shift_only()
            && args_[0]->isa<Tuple>() ? -args_[0]->num_ops()+1 : shift();
        return world().var(new_type, var->index() - total_shift, var->debug());
    }

    if (auto old_lambda = old_def->isa_lambda()) {
        auto new_lambda = clone(old_lambda);
        return map_[{old_lambda, offset}] = new_lambda;
    }

    DefArray new_ops(old_def->num_ops());
    for (size_t i = 0, e = old_def->num_ops(); i != e; ++i)
        new_ops[i] = reduce(old_def->op(i), offset + old_def->shift(i));

    auto new_def = old_def->rebuild(world(), new_type, new_ops);
    return map_[{old_def, offset}] = new_def;
}

//------------------------------------------------------------------------------

const Def* reduce(const Def* def, Defs args, size_t index) {
    if (def->free_vars().none_begin(index))
        return def;

    Reducer reducer(def->world(), args);
    return reducer.reduce(def, index);
}

const Def* flatten(const Def* body, Defs args) {
    auto& w = body->world();
    auto t = w.tuple(DefArray(args.size(), [&](auto i) { return w.var(args[i], args.size()-1-i); }));
    return reduce(body, t);
}

const Def* shift_free_vars(const Def* def, int64_t shift) {
    if (shift == 0 || def->free_vars().none())
        return def;
    assertf(shift > 0 || def->free_vars().none_end(-shift),
            "can't shift {} by {}, there are variables with index <= {}", def, shift, -shift);

    Reducer reducer(def->world(), -shift);
    return reducer.reduce(def, 0);
}

}
