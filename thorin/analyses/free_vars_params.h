#ifndef THORIN_ANALYSES_FREE_VARS_PARAMS_H
#define THORIN_ANALYSES_FREE_VARS_PARAMS_H

#include "thorin/world.h"

namespace thorin {

template<class Visitor, typename R>
R visit_free_vars_params(Visitor& visitor, const Def* def, size_t offset = 0) {
    if (def->is_nominal()) {
        return visitor.visit_nominal(def, offset);
    } else if (def->free_vars().none_begin(offset)) {
        return visitor.visit_no_free_vars(def, offset);
    }

    if (auto opt = visitor.is_visited(def, offset))
        return opt.value();

    auto type_val = visit_free_vars_params<Visitor, R>(visitor, def->type(), offset);

    if (auto var = def->isa<Var>()) {
        if (offset > var->index())
            return visitor.visit_nonfree_var(var, offset, type_val);
        else
            return visitor.visit_free_var(var, offset, type_val);
    } else if (auto param = def->isa<Param>()) {
        // TODO what about continuations and free/bound params?
        return visitor.visit_param(param, offset, type_val);
    }

    if (auto opt = visitor.stop_recursion(def, offset, type_val)) {
        return opt.value();
    }

    auto tmp = visitor.visit_pre_ops(def, offset, type_val);

    for (size_t i = 0, e = def->num_ops(); i != e; ++i) {
        auto result_i = visit_free_vars_params<Visitor, R>(visitor, def->op(i), offset + def->shift(i));
        visitor.visit_op(def, offset, tmp, i, result_i);
    }

    return visitor.visit_post_ops(def, offset, type_val, tmp);
}

}

#endif
