#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#include "thorin/util/utility.h"

namespace thorin {

/// Boolean operations
#define THORIN_B_OP(m) m(BOp, band) m(BOp, bor) m(BOp, bxor)
/// Nat operations
#define THORIN_N_OP(m) m(NOp, nadd) m(NOp, nsub) m(NOp, nmul) m(NOp, ndiv) m(NOp, nmod)

enum class BOp : size_t {
#define CODE(T, o) o,
    THORIN_B_OP(CODE)
#undef CODE
};

enum class NOp : size_t {
#define CODE(T, o) o,
    THORIN_N_OP(CODE)
#undef CODE
};

template<class T> constexpr auto Num = size_t(-1);

#define CODE(T, o) + 1_s
template<> constexpr auto Num<BOp>  = 0_s THORIN_B_OP (CODE);
template<> constexpr auto Num<NOp>  = 0_s THORIN_N_OP (CODE);
#undef CODE

constexpr const char* op2str(BOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_B_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(NOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_N_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

}

#endif
