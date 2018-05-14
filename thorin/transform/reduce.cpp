#include "thorin/transform/reduce.h"

#include "thorin/world.h"
#include "thorin/analyses/free_vars_params.h"
#include "thorin/transform/mangle.h"

namespace thorin {

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


    const Def* visit_nominal(const Def* def, size_t offset) { return visit_no_free_vars(def, offset); }
    const Def* visit_no_free_vars(const Def* def, size_t offset) {
        map_[{def, offset}] = def;
        return def;
    }
    std::optional<const Def*> is_visited(const Def* def, size_t offset) {
        if (auto new_def = find(map_, {def, offset}))
            return new_def;
        return std::nullopt;
    }
    const Def* visit_free_var(const Var* var, size_t offset, const Def* new_type) {
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
    const Def* visit_param(const Param* param, size_t, const Def*) { return param; }

    const Def* visit_nonfree_var(const Var* var, size_t, const Def* new_type) {
        // keep index, substitute type
        return world().var(new_type, var->index(), var->debug());
    }
    std::optional<const Def*> stop_recursion(const Def*, size_t, const Def*) { return std::nullopt; }
    DefArray visit_pre_ops(const Def* def, size_t, const Def*) { return DefArray(def->num_ops()); }
    void visit_op(const Def*, size_t, DefArray& new_ops, size_t index, const Def* result) { new_ops[index] = result; }
    const Def* visit_post_ops(const Def* def, size_t offset, const Def* new_type, DefArray& new_ops) {
        auto new_def = def->rebuild(world(), new_type, new_ops);
        return map_[{def, offset}] = new_def;
    }

    size_t shift() const { return shift_; }
    bool is_shift_only() const { return args_.empty(); }
    World& world() const { return world_; }

private:
    World& world_;
    DefArray args_;
    int64_t shift_;
    DefIndexMap<const Def*> map_;
};

//------------------------------------------------------------------------------

const Def* reduce(const Def* def, Defs args, size_t index) {
    if (def->free_vars().none_begin(index))
        return def;

    Reducer reducer(def->world(), args);
    return visit_free_vars_params<Reducer, const Def*>(reducer, def, index);
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
    return visit_free_vars_params<Reducer, const Def*>(reducer, def, 0);
}

}
