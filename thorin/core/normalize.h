#ifndef THORIN_CORE_NORMALIZE_H
#define THORIN_CORE_NORMALIZE_H

#include "thorin/util/location.h"

namespace thorin {

class Def;
class World;

namespace core {

const Def* normalize_iadd_flags(thorin::World& world, const Def*, const Def*, const Def*, Debug);

}
}

#endif
