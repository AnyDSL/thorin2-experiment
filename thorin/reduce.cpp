#include "thorin/reduce.h"

#include "thorin/world.h"

namespace thorin {

const Def* Reducer::reduce(size_t index) {
    auto result = reduce_up_to_nominals(index);
    reduce_nominals();
    return result;
}

const Def* Reducer::reduce_up_to_nominals(size_t index) {
    if (def_->free_vars().none_from(index))
        return def_;
    return reduce(def_, index);
}

void Reducer::reduce_nominals() {
    while (!nominals_.empty()) {
        auto subst = nominals_.top();
        nominals_.pop();
        if (auto replacement = find(map_, subst)) {
            if (replacement == subst || replacement->is_closed())
                continue;

            for (size_t i = 0, e = subst->num_ops(); i != e; ++i) {
                auto new_op = reduce(subst->op(i), subst.index() + subst->shift(i));
                const_cast<Def*>(replacement)->set(i, new_op);
            }
        }
    }
}

const Def* Reducer::reduce(const Def* def, size_t offset) {
    if (auto replacement = find(map_, {def, offset}))
        return replacement;

    if (def->free_vars().none_from(offset)) {
        map_[{def, offset}] = def;
        return def;
    } else if (def->is_nominal()) {
        auto new_type = reduce(def->type(), offset);
        auto replacement = def->stub(new_type); // TODO better debug info for these
        map_[{def, offset}] = replacement;
        nominals_.emplace(def, offset);
        return replacement;
    }

    auto new_type = reduce(def->type(), offset);

    if (auto var = def->isa<Var>())
        return var_reduce(var, new_type, offset);
    return rebuild(def, new_type, offset);
}

const Def* Reducer::var_reduce(const Var* var, const Def* new_type, size_t offset) {
    // The offset always corresponds to args_.back() and thus corresponds to args_.size() - 1.
    // Map index() back into the original argument array.
    int arg_index = args_.size() - 1 - var->index() + offset;
    if (arg_index >= 0 && size_t(arg_index) < args_.size()) {
        if (new_type != args_[arg_index]->type())
            // Use the expected type, not the one provided by the arg.
            return world().error(new_type);
        return args_[arg_index];
    } else if (arg_index < 0) {
        // this is a free variable - need to shift by args.size() for the current reduction
        return world().var(var->type(), var->index() - args_.size(), var->debug());
    }
    // this variable is not free - keep index, substitute type
    return world().var(new_type, var->index(), var->debug());
}

const Def* Reducer::rebuild(const Def* def, const Def* new_type, size_t offset) {
    Array<const Def*> new_ops(def->num_ops());
    for (size_t i = 0, e = def->num_ops(); i != e; ++i) {
        auto new_op = reduce(def->op(i), offset + def->shift(i));
        new_ops[i] = new_op;
    }

    auto new_def = def->rebuild(world(), new_type, new_ops);
    return map_[{def, offset}] = new_def;
}

}
