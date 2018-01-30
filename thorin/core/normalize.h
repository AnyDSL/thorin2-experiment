#ifndef THORIN_CORE_NORMALIZE_H
#define THORIN_CORE_NORMALIZE_H

#include "thorin/core/tables.h"
#include "thorin/util/location.h"

namespace thorin {

class Def;

namespace core {

#define CODE(T, o) const Def* normalize_ ## o(const Def*, const Def*, Debug);
    THORIN_W_OP(CODE)
    THORIN_M_OP(CODE)
    THORIN_I_OP(CODE)
    THORIN_R_OP(CODE)
    THORIN_CAST(CODE)
#undef CODE

template<ICmp op> const Def* normalize_ICmp(const Def*, const Def*, Debug);
template<RCmp op> const Def* normalize_RCmp(const Def*, const Def*, Debug);

}
}

#endif
