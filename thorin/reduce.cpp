#include "thorin/reduce.h"

#include "thorin/world.h"

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
    DefMap<const Def*> nominals_;
};

const Def* Reducer::reduce(const Def* old_def, size_t offset) {
    if (old_def->free_vars().none_begin(offset)) {
        map_[{old_def, offset}] = old_def;
        return old_def;
    }

    if (auto new_def = find(map_, {old_def, offset}))
        return new_def;

    if (auto new_def = find(nominals_, old_def))
        return new_def;

    auto new_type = reduce(old_def->type(), offset);

    if (auto var = old_def->isa<Var>()) {
        if (!is_shift_only()) {
            // Is var within our args? - Map index() back into the original argument array.
            if (offset <= var->index() && var->index() < offset + shift()) {
                // remember that the lowest index corresponds to the last element in args due to De Bruijn's
                // way of counting
                size_t arg_index = shift() - (var->index() - offset) - 1;
                if (!new_type->assignable(world(), args_[arg_index]))
                    return world().error(new_type); // use the expected type, not the one provided by the arg
                return args_[arg_index]->shift_free_vars(world(), -offset);
            }
        }

        // is var not free? - keep index, substitute type
        if (var->index() < offset)
            return world().var(new_type, var->index(), var->debug());

        // this var is free - shift by shift but inline a sole tuple arg if applicable
        auto total_shift = shift() == 1 && !is_shift_only()
                && args_[0]->isa<Tuple>() ? -args_[0]->num_ops()+1 : shift();
        return world().var(var->type(), var->index() - total_shift, var->debug());
    }

    Def* new_nominal = nullptr;
    if (old_def->is_nominal()) {
        new_nominal = old_def->stub(world(), new_type);
        nominals_.emplace(old_def, new_nominal);
    }

    // rebuild all other defs
    DefArray new_ops(old_def->num_ops());
    for (size_t i = 0, e = old_def->num_ops(); i != e; ++i)
        new_ops[i] = reduce(old_def->op(i), offset + old_def->shift(i));

    if (new_nominal != nullptr) {
        for (size_t i = 0, e = new_ops.size(); i != e; ++i)
            new_nominal->set(world(), i, new_ops[i]);
        return new_nominal;
    }

    auto new_def = old_def->rebuild(world(), new_type, new_ops);
    return map_[{old_def, offset}] = new_def;
}

//------------------------------------------------------------------------------

const Def* reduce(World& world, const Def* def, Defs args, size_t index) {
    if (def->free_vars().none_begin(index))
        return def;

    Reducer reducer(world, args);
    return reducer.reduce(def, index);
}

const Def* unflatten(World& world, const Def* body, const Def* arg) {
    auto arity = arg->arity(world)->as<Arity>();
    assert(arity != nullptr);
    auto length = arity->value();
    auto extracts = DefArray(length, [&](auto i) { return world.extract(arg, i); });
    return reduce(world, body, extracts);
}

const Def* flatten(World& world, const Def* body, Defs args) {
    auto t = world.tuple(DefArray(args.size(), [&](auto i) { return world.var(args[i], args.size()-1-i); }));
    return reduce(world, body, {t});
}

const Def* shift_free_vars(World& world, const Def* def, int64_t shift) {
    if (shift == 0 || def->free_vars().none())
        return def;
    assertf(shift > 0 || def->free_vars().none_end(-shift),
            "can't shift {} by {}, there are variables with index <= {}", def, shift, -shift);

    Reducer reducer(world, -shift);
    return reducer.reduce(def, 0);
}

}
