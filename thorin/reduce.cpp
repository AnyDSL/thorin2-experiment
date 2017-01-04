#include "thorin/reduce.h"

#include "thorin/world.h"

namespace thorin {

const Def* Reducer::reduce() {
    auto result = reduce_up_to_nominals();
    reduce_nominals();
    return result;
}

const Def* Reducer::reduce_up_to_nominals() {
    if (!def_->has_free_var_ge(index_)) {
        return def_;
    }
    return reduce(def_, index_);
}

void Reducer::reduce_nominals() {
    while (!nominals_.empty()) {
        const auto& subst = nominals_.top();
        nominals_.pop();
        if (auto replacement = find(map_, subst)) {
            if (replacement == subst || replacement->is_closed())
                continue;

            subst->foreach_op_index(subst.index(),
                [&] (size_t op_index, const Def* op, size_t shifted_index) {
                    auto new_op = reduce(op, shifted_index);
                    const_cast<Def*>(replacement)->set(op_index, new_op);
                });
        }
    }
}

const Def* Reducer::reduce(const Def* def, size_t shift) {
    if (auto replacement = find(map_, {def, shift}))
        return replacement;
    if (!def->has_free_var_ge(shift)) {
        map_[{def, shift}] = def;
        return def;
    } else if (def->is_nominal()) {
        auto new_type = reduce(def->type(), shift);
        auto replacement = def->stub(new_type); // TODO better debug info for these
        map_[{def, shift}] = replacement;
        nominals_.emplace(def, shift);
        return replacement;
    }

    auto new_type = reduce(def->type(), shift);

    if (auto var = def->isa<Var>()) {
        return var_reduce(var, shift, new_type);
    }
    return rebuild_reduce(def, shift, new_type);
}

const Def* Reducer::var_reduce(const Var* var, size_t shift, const Def* new_type) {
    // The shift argument always corresponds to args.back() and thus corresponds to args.size() - 1.
    // Map index() back into the original argument array.
    int arg_index = args_.size() - 1 - var->index() + shift;
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

const Def* Reducer::rebuild_reduce(const Def* def, size_t shift, const Def* new_type) {
    Array<const Def*> new_ops(def->num_ops());
    def->foreach_op_index(shift, [&] (size_t op_index, const Def* op, size_t shifted_index) {
            auto new_op = reduce(op, shifted_index);
            new_ops[op_index] = new_op;
        });

    auto new_def = def->rebuild(world(), new_type, new_ops);
    return map_[{def, shift}] = new_def;
}

}
