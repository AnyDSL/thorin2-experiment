#ifndef THORIN_CORE_NORMALIZE_H
#define THORIN_CORE_NORMALIZE_H

#include "thorin/core/tables.h"
#include "thorin/util/location.h"

namespace thorin {

class Def;
class World;

namespace core {

#define CODE(T, o) const Def* normalize_ ## T ## o ## _0(thorin::World& world, const Def*, const Def*, const Def*, Debug);
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_R_OP (CODE)
    THORIN_I_CMP(CODE)
    THORIN_R_CMP(CODE)
#undef CODE

}
}

#endif
