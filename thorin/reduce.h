#ifndef THORIN_REDUCE_H
#define THORIN_REDUCE_H

#include "thorin/def.h"

namespace thorin {

/// Reduces @p def with @p args using @p index to indicate the Var.
const Def* reduce(const Def* def, Defs args, size_t index = 0);

/**
 * Reduces @p def with @p args using @p index to indicate the Var.
 * First, all structural stuff in @p def is reduced.
 * Then, the hook @p f is called with the result of the first reduction run.
 * Finally, a fixed-pointer iteration is performed to reduce the rest.
 */
const Def* reduce(const Def* def, Defs args, std::function<void(const Def*)> f, size_t index = 0);

}

#endif
