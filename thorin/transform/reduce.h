#ifndef THORIN_REDUCE_H
#define THORIN_REDUCE_H

#include "thorin/def.h"

namespace thorin {

/// Reduces @p def with @p args using @p index to indicate the Var.
const Def* reduce(const Def* def, Defs args, size_t index = 0);
inline const Def* reduce(const Def* def, const Def* arg, size_t index = 0) { return reduce(def, Defs{arg}, index); }

/// Flattens the use of Var @c 0 and Extract%s in @p body to directly use @p args instead.
const Def* flatten(const Def* body, Defs args);

/// Adds @p shift to all free variables in @p def.
const Def* shift_free_vars(const Def* def, int64_t shift);
}

#endif
