#ifndef THORIN_UTIL_TRAITS_H
#define THORIN_UTIL_TRAITS_H

namespace thorin {

class Axiom;
class Def;

const Axiom* isa_const_qualifier(const Def* def);
#if 0
bool is_primitive_type(const Def* type);
bool is_primitive_type_constructor(const Def* def);
#endif

}

#endif
