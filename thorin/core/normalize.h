#ifndef THORIN_CORE_NORMALIZE_H
#define THORIN_CORE_NORMALIZE_H

#include "thorin/core/tables.h"
#include "thorin/util/location.h"

namespace thorin {

class Def;
class World;

namespace core {

#define CODE(o) const Def* normalize_ ## o ## _0(thorin::World& world, const Def*, const Def*, const Def*, Debug);
    THORIN_W_ARITHOP(CODE)
    THORIN_M_ARITHOP(CODE)
    THORIN_I_ARITHOP(CODE)
    THORIN_R_ARITHOP(CODE)
#undef CODE

}
}

#endif
