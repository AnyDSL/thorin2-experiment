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
constexpr WFlags operator&(WFlags a, WFlags b) { return WFlags(int64_t(a) & int64_t(b)); }

constexpr RFlags operator|(RFlags a, RFlags b) { return RFlags(int64_t(a) | int64_t(b)); }
constexpr RFlags operator&(RFlags a, RFlags b) { return RFlags(int64_t(a) & int64_t(b)); }

constexpr bool has_feature(WFlags flags, WFlags feature) { return (flags & feature) == feature; }
constexpr bool has_feature(RFlags flags, RFlags feature) { return (flags & feature) == feature; }

/// Integer instructions that might wrap and, hence, take @p WFlags.
#define THORIN_W_OP(m) m(WOp, add) m(WOp, sub) m(WOp, mul) m(WOp, shl)
/// Integer instructions that might produce a side effect (division by zero).
#define THORIN_M_OP(m) m(MOp, sdiv) m(MOp, udiv) m(MOp, smod) m(MOp, umod)
/// Integer instructions that neither take wflags nor do they produce a side effect.
#define THORIN_I_OP(m) m(IOp, ashr) m(IOp, lshr) m(IOp, iand) m(IOp, ior) m(IOp, ixor)
/// Floating point (real) instructions that take @p RFlags.
#define THORIN_R_OP(m) m(ROp, radd) m(ROp, rsub) m(ROp, rmul) m(ROp, rdiv) m(ROp, rmod)
/// All cast instructions that cast from/to real/signed/unsigned.
#define THORIN_CAST(m) m(Cast, scast) m(Cast, ucast) m(Cast, rcast) m(Cast, s2r) m(Cast, u2r) m(Cast, r2s) m(Cast, r2u)

#define THORIN_I_CMP(m)\
    m(ICmp, eq)  /* equal */ \
    m(ICmp, ne)  /* not equal */ \
    m(ICmp, ugt) /* unsigned greater than */ \
    m(ICmp, uge) /* unsigned greater or equal */ \
    m(ICmp, ult) /* unsigned less than */ \
    m(ICmp, ule) /* unsigned less or equal */ \
    m(ICmp, sgt) /* signed greater than */ \
    m(ICmp, sge) /* signed greater or equal */ \
    m(ICmp, slt) /* signed less than */ \
    m(ICmp, sle) /* signed less or equal */

#define THORIN_R_CMP(m)           /* O E G L                                      */ \
                     m(RCmp, t)   /* o o o o - always true                        */ \
                     m(RCmp, ult) /* o o o x - unordered or less than             */ \
                     m(RCmp, ugt) /* o o x o - unordered or greater than          */ \
                     m(RCmp, une) /* o o x x - unordered or not equal             */ \
                     m(RCmp, ueq) /* o x o o - unordered or equal                 */ \
                     m(RCmp, ule) /* o x o x - unordered or less than or equal    */ \
                     m(RCmp, uge) /* o x x o - unordered or greater than or equal */ \
                     m(RCmp, uno) /* o x x x - unordered (either NaNs)            */ \
                     m(RCmp, ord) /* x o o o - ordered (no NaNs)                  */ \
                     m(RCmp, olt) /* x o o x - ordered and less than              */ \
                     m(RCmp, ogt) /* x o x o - ordered and greater than           */ \
                     m(RCmp, one) /* x o x x - ordered and not equal              */ \
                     m(RCmp, oeq) /* x x o o - ordered and equal                  */ \
                     m(RCmp, ole) /* x x o x - ordered and less than or equal     */ \
                     m(RCmp, oge) /* x x x o - ordered and greater than or equal  */ \
                     m(RCmp, f)   /* x x x x - always false                       */

enum class WOp : size_t {
#define CODE(T, o) o,
    THORIN_W_OP(CODE)
#undef CODE
};

enum class MOp : size_t {
#define CODE(T, o) o,
    THORIN_M_OP(CODE)
#undef CODE
};

enum class IOp : size_t {
#define CODE(T, o) o,
    THORIN_I_OP(CODE)
#undef CODE
};

enum class ROp : size_t {
#define CODE(T, o) o,
    THORIN_R_OP(CODE)
#undef CODE
};

enum class ICmp : size_t {
#define CODE(T, o) o,
    THORIN_I_CMP(CODE)
#undef CODE
};

enum class RCmp : size_t {
#define CODE(T, o) o,
    THORIN_R_CMP(CODE)
#undef CODE
};

enum class Cast : size_t {
#define CODE(T, o) o,
    THORIN_CAST(CODE)
#undef CODE
};

template<class T> constexpr auto Num = size_t(-1);

#define CODE(T, o) + 1_s
template<> constexpr auto Num<WOp>  = 0_s THORIN_W_OP (CODE);
template<> constexpr auto Num<MOp>  = 0_s THORIN_M_OP (CODE);
template<> constexpr auto Num<IOp>  = 0_s THORIN_I_OP (CODE);
template<> constexpr auto Num<ROp>  = 0_s THORIN_R_OP (CODE);
template<> constexpr auto Num<ICmp> = 0_s THORIN_I_CMP(CODE);
template<> constexpr auto Num<RCmp> = 0_s THORIN_R_CMP(CODE);
template<> constexpr auto Num<Cast> = 0_s THORIN_CAST (CODE);
#undef CODE

THORIN_ENUM_ITERATOR(WOp)
THORIN_ENUM_ITERATOR(MOp)
THORIN_ENUM_ITERATOR(IOp)
THORIN_ENUM_ITERATOR(ROp)
THORIN_ENUM_ITERATOR(ICmp)
THORIN_ENUM_ITERATOR(RCmp)

constexpr const char* op2str(WOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_W_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(MOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_M_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(IOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_I_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(ROp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_R_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* cmp2str(ICmp o) {
    switch (o) {
#define CODE(T, o) case T::o: return "icmp_" #o;
    THORIN_I_CMP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* cmp2str(RCmp o) {
    switch (o) {
#define CODE(T, o) case T::o: return "rcmp_" #o;
    THORIN_R_CMP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* cast2str(Cast o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_CAST(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

}

#endif
