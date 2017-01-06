#include "thorin/reduce.h"

#include "thorin/world.h"

namespace thorin {

const Def* Reducer::reduce(size_t index) {
    auto result = reduce_up_to_nominals(index);
    reduce_nominals();
    return result;
}

const Def* Reducer::reduce_up_to_nominals(size_t index) {
    if (def_->free_vars().none_begin(index))
        return def_;
    return reduce(def_, index);
}

void Reducer::reduce_nominals() {
    while (!nominals_.empty()) {
        auto subst = nominals_.top();
        nominals_.pop();
        if (auto new_def = find(map_, subst)) {
            if (new_def == subst || new_def->is_closed())
                continue;

            for (size_t i = 0, e = subst->num_ops(); i != e; ++i) {
                auto new_op = reduce(subst->op(i), subst.index() + subst->shift(i));
                const_cast<Def*>(new_def)->set(i, new_op);
            }
        }
    }
}

const Def* Reducer::reduce(const Def* old_def, size_t offset) {
    if (auto new_def = find(map_, {old_def, offset}))
        return new_def;

    if (old_def->free_vars().none_begin(offset)) {
        map_[{old_def, offset}] = old_def;
        return old_def;
    }

    auto new_type = reduce(old_def->type(), offset);

    if (old_def->is_nominal()) {
        auto new_def = old_def->stub(new_type); // TODO better debug info for these
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

        // Is var not free? - keep index, substitute type
        if (var->index() < offset)
            return world().var(new_type, var->index(), var->debug());

        // This var is free - shift by args.size()
        return world().var(var->type(), var->index() - num_args(), var->debug());
    }

    // rebuild all other defs
    Array<const Def*> new_ops(old_def->num_ops());
    for (size_t i = 0, e = old_def->num_ops(); i != e; ++i) {
        auto new_op = reduce(old_def->op(i), offset + old_def->shift(i));
        new_ops[i] = new_op;
    }

    auto new_def = old_def->rebuild(world(), new_type, new_ops);
    return map_[{old_def, offset}] = new_def;
}

}
