#ifndef THORIN_CORE_NORMALIZE_H
#define THORIN_CORE_NORMALIZE_H

#include "thorin/core/tables.h"
#include "thorin/util/location.h"

namespace thorin {

class Def;
class World;

namespace core {

#define CODE(o) const Def* normalize_ ## o ## _0(thorin::World& world, const Def*, const Def*, const Def*, Debug);
    THORIN_W_OP(CODE)
    THORIN_M_OP(CODE)
    THORIN_I_OP(CODE)
    THORIN_R_OP(CODE)
#undef CODE

const Def* normalize_icmp_0(thorin::World& world, const Def*, const Def*, const Def*, Debug);
const Def* normalize_rcmp_0(thorin::World& world, const Def*, const Def*, const Def*, Debug);

}
}

#endif
