#ifndef THORIN_ME_NORMALIZE_H
#define THORIN_ME_NORMALIZE_H

#include "thorin/me/tables.h"
#include "thorin/util/debug.h"

namespace thorin {

class Def;

namespace me {

template<WOp > const Def* normalize_WOp (const Def*, const Def*, Debug);
template<MOp > const Def* normalize_MOp (const Def*, const Def*, Debug);
template<IOp > const Def* normalize_IOp (const Def*, const Def*, Debug);
template<FOp > const Def* normalize_FOp (const Def*, const Def*, Debug);
template<ICmp> const Def* normalize_ICmp(const Def*, const Def*, Debug);
template<FCmp> const Def* normalize_FCmp(const Def*, const Def*, Debug);
template<Cast> const Def* normalize_Cast(const Def*, const Def*, Debug);

}
}

#endif
