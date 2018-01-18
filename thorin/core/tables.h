#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#include "thorin/util/utility.h"

namespace thorin {

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
#define THORIN_W_ARITHOP(m) m(wadd) m(wsub) m(wmul) m(wshl)
/// Integer instructions that might produce a side effect (division by zero).
#define THORIN_M_ARITHOP(m) m(sdiv) m(udiv) m(smod) m(umod)
/// Integer instructions that neither take wflags nor do they produce a side effect.
#define THORIN_I_ARITHOP(m)  m(ashr) m(lshr) m(iand) m(ior) m(ixor)
/// Floating point (real) instructions that take @p RFlags.
#define THORIN_R_ARITHOP(m)  m(radd) m(rsub) m(rmul) m(rdiv) m(rmod)

#define THORIN_I_REL(m)\
    m(eq)  /* equal */ \
    m(ne)  /* not equal */ \
    m(ugt) /* unsigned greater than */ \
    m(uge) /* unsigned greater or equal */ \
    m(ult) /* unsigned less than */ \
    m(ule) /* unsigned less or equal */ \
    m(sgt) /* signed greater than */ \
    m(sge) /* signed greater or equal */ \
    m(slt) /* signed less than */ \
    m(sle) /* signed less or equal */

#define THORIN_R_REL(m)     /* O E G L                                      */ \
                     m(t)   /* o o o o - always true                        */ \
                     m(ult) /* o o o x - unordered or less than             */ \
                     m(ugt) /* o o x o - unordered or greater than          */ \
                     m(une) /* o o x x - unordered or not equal             */ \
                     m(ueq) /* o x o o - unordered or equal                 */ \
                     m(ule) /* o x o x - unordered or less than or equal    */ \
                     m(uge) /* o x x o - unordered or greater than or equal */ \
                     m(uno) /* o x x x - unordered (either NaNs)            */ \
                     m(ord) /* x o o o - ordered (no NaNs)                  */ \
                     m(olt) /* x o o x - ordered and less than              */ \
                     m(ogt) /* x o x o - ordered and greater than           */ \
                     m(one) /* x o x x - ordered and not equal              */ \
                     m(oeq) /* x x o o - ordered and equal                  */ \
                     m(ole) /* x x o x - ordered and less than or equal     */ \
                     m(oge) /* x x x o - ordered and greater than or equal  */ \
                     m(f)   /* x x x x - always false                       */

enum WArithop : size_t {
#define CODE(O) O,
    THORIN_W_ARITHOP(CODE)
#undef CODE
    Num_WArithOp
};

enum MArithop : size_t {
#define CODE(O) O,
    THORIN_M_ARITHOP(CODE)
#undef CODE
    Num_MArithOp
};

enum IArithop : size_t {
#define CODE(O) O,
    THORIN_I_ARITHOP(CODE)
#undef CODE
    Num_IArithOp
};

enum RArithop : size_t {
#define CODE(O) O,
    THORIN_R_ARITHOP(CODE)
#undef CODE
    Num_RArithOp
};

constexpr const char* arithop2str(WArithop o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_W_ARITHOP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* arithop2str(MArithop o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_M_ARITHOP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* arithop2str(IArithop o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_I_ARITHOP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* arithop2str(RArithop o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_R_ARITHOP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

enum class IRel : int64_t {
#define CODE(f) f,
    THORIN_I_REL(CODE)
#undef CODE
    Num
};

enum class RRel : int64_t {
#define CODE(f) f,
    THORIN_R_REL(CODE)
#undef CODE
    Num
};

}

#endif
