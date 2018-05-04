#ifndef THORIN_NORMALIZE_H
#define THORIN_NORMALIZE_H

#include "thorin/tables.h"
#include "thorin/util/array.h"
#include "thorin/util/debug.h"

namespace thorin {

class Def;
typedef ArrayRef<const Def*> Defs;
class Lit;

std::array<const Def*, 2> split(const Def* def);
std::array<const Def*, 2> shrink_shape(const Def* def);
bool is_foldable(const Def* def);
const Lit* foldable_to_left(const Def*& a, const Def*& b);
const Def* commute(const Def* callee, const Def* a, const Def* b, Debug dbg);
const Def* reassociate(const Def* callee, const Def* a, const Def* b, Debug dbg);
const Def* normalize_tuple(const Def* callee, Defs, Debug dbg);
const Def* normalize_arity_succ(const Def* callee, const Def* arg, Debug dbg);
const Def* normalize_arity_eliminator(const Def* callee, const Def* arg, Debug dbg);
const Def* normalize_index_zero(const Def* callee, const Def* arg, Debug dbg);
const Def* normalize_index_succ(const Def* callee, const Def* arg, Debug dbg);
const Def* normalize_index_eliminator(const Def* callee, const Def* arg, Debug dbg);
template<BOp> const Def* normalize_BOp(const Def*, const Def*, Debug);
template<NOp> const Def* normalize_NOp(const Def*, const Def*, Debug);

}

#endif
