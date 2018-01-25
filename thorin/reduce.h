#ifndef THORIN_REDUCE_H
#define THORIN_REDUCE_H

#include "thorin/def.h"

namespace thorin {

/// Reduces @p def with @p args using @p index to indicate the Var.
const Def* reduce(World&, const Def* def, Defs args, size_t index = 0);

/// Flattens the use of Var @c 0 and Extract%s in @p body to directly use @p args instead.
const Def* flatten(World&, const Def* body, Defs args);

/// Unflattens the use of Vars @c 0, ..., n and uses Extract%s in @p body instead.
const Def* unflatten(World&, const Def* body, const Def* arg);

/// Adds @p shift to all free variables in @p def.
const Def* shift_free_vars(World&, const Def* def, int64_t shift);
}

#endif
