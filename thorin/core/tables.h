#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#include "thorin/util/utility.h"

namespace thorin::core {

enum class WFlags : int64_t {
    none = 0,
    nsw  = 1 << 0,
    nuw  = 1 << 1,
};

enum class RFlags : int64_t {
    none     = 0,
    nnan     = 1 << 0, ///< No NaNs - Allow optimizations to assume the arguments and result are not NaN. Such optimizations are required to retain defined behavior over NaNs, but the value of the result is undefined.
    ninf     = 1 << 1, ///< No Infs - Allow optimizations to assume the arguments and result are not +/-Inf. Such optimizations are required to retain defined behavior over +/-Inf, but the value of the result is undefined.
    nsz      = 1 << 2, ///< No Signed Zeros - Allow optimizations to treat the sign of a zero argument or result as insignificant.
    arcp     = 1 << 3, ///< Allow Reciprocal - Allow optimizations to use the reciprocal of an argument rather than perform division.
    contract = 1 << 4, ///< Allow floating-point contraction (e.g. fusing a multiply followed by an addition into a fused multiply-and-add).
    afn      = 1 << 5, ///< Approximate functions - Allow substitution of approximate calculations for functions (sin, log, sqrt, etc). See floating-point intrinsic definitions for places where this can apply to LLVM’s intrinsic math functions.
    reassoc  = 1 << 6, ///< Allow reassociation transformations for floating-point instructions. This may dramatically change results in floating point.
    fast = nnan | ninf | nsz | arcp | contract | afn | reassoc,
};

constexpr WFlags operator|(WFlags a, WFlags b) { return WFlags(int64_t(a) | int64_t(b)); }
constexpr RFlags operator|(RFlags a, RFlags b) { return RFlags(int64_t(a) | int64_t(b)); }

/// Integer instructions that might wrap and, hence, take @p WFlags.
#define THORIN_W_OP(m) m(wadd) m(wsub) m(wmul) m(wshl)
/// Integer instructions that might produce a side effect (division by zero).
#define THORIN_M_OP(m) m(sdiv) m(udiv) m(smod) m(umod)
/// Integer instructions that neither take wflags nor do they produce a side effect.
#define THORIN_I_OP(m) m(ashr) m(lshr) m(iand) m(ior) m(ixor)
/// Floating point (real) instructions that take @p RFlags.
#define THORIN_R_OP(m) m(radd) m(rsub) m(rmul) m(rdiv) m(rmod)

#define THORIN_I_CMP(m)\
    m(i_eq)  /* equal */ \
    m(i_ne)  /* not equal */ \
    m(iugt) /* unsigned greater than */ \
    m(iuge) /* unsigned greater or equal */ \
    m(iult) /* unsigned less than */ \
    m(iule) /* unsigned less or equal */ \
    m(isgt) /* signed greater than */ \
    m(isge) /* signed greater or equal */ \
    m(islt) /* signed less than */ \
    m(isle) /* signed less or equal */

#define THORIN_R_CMP(m)      /* O E G L                                      */ \
                     m(rt)   /* o o o o - always true                        */ \
                     m(rult) /* o o o x - unordered or less than             */ \
                     m(rugt) /* o o x o - unordered or greater than          */ \
                     m(rune) /* o o x x - unordered or not equal             */ \
                     m(rueq) /* o x o o - unordered or equal                 */ \
                     m(rule) /* o x o x - unordered or less than or equal    */ \
                     m(ruge) /* o x x o - unordered or greater than or equal */ \
                     m(runo) /* o x x x - unordered (either NaNs)            */ \
                     m(rord) /* x o o o - ordered (no NaNs)                  */ \
                     m(rolt) /* x o o x - ordered and less than              */ \
                     m(rogt) /* x o x o - ordered and greater than           */ \
                     m(rone) /* x o x x - ordered and not equal              */ \
                     m(roeq) /* x x o o - ordered and equal                  */ \
                     m(role) /* x x o x - ordered and less than or equal     */ \
                     m(roge) /* x x x o - ordered and greater than or equal  */ \
                     m(rf)   /* x x x x - always false                       */

enum WOp : size_t {
#define CODE(O) O,
    THORIN_W_OP(CODE)
#undef CODE
    Num_WOp
};

enum MOp : size_t {
#define CODE(O) O,
    THORIN_M_OP(CODE)
#undef CODE
    Num_MOp
};

enum IOp : size_t {
#define CODE(O) O,
    THORIN_I_OP(CODE)
#undef CODE
    Num_IOp
};

enum ROp : size_t {
#define CODE(O) O,
    THORIN_R_OP(CODE)
#undef CODE
    Num_ROp
};

enum ICmp : size_t {
#define CODE(f) f,
    THORIN_I_CMP(CODE)
#undef CODE
    Num_ICmp
};

enum RCmp : size_t {
#define CODE(f) f,
    THORIN_R_CMP(CODE)
#undef CODE
    Num_RCmp
};

constexpr const char* op2str(WOp o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_W_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(MOp o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_M_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(IOp o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_I_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(ROp o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_R_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* cmp2str(ICmp o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_I_CMP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* cmp2str(RCmp o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_R_CMP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

}

#endif
