#ifndef THORIN_CORE_TABLES_H
#define THORIN_CORE_TABLES_H

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
    reassoc  = 1 << 6, ///< Allow reassociation transformations for floating-point operations. This may dramatically change results in floating point.
    finite   = nnan | ninf,
    unsafe   = nsz | arcp | reassoc,
    fast = nnan | ninf | nsz | arcp | contract | afn | reassoc,
};

/// Integer operations that might wrap and, hence, take @p WFlags.
#define THORIN_W_OP(m) m(WOp, add) m(WOp, sub) m(WOp, mul) m(WOp, shl)
/// Integer operations that might produce a side effect (division by zero).
#define THORIN_M_OP(m) m(MOp, sdiv) m(MOp, udiv) m(MOp, smod) m(MOp, umod)
/// Integer operations that neither take wflags nor do they produce a side effect.
#define THORIN_I_OP(m) m(IOp, ashr) m(IOp, lshr) m(IOp, iand) m(IOp, ior) m(IOp, ixor)
/// Floating point (real) operations that take @p RFlags.
#define THORIN_R_OP(m) m(ROp, radd) m(ROp, rsub) m(ROp, rmul) m(ROp, rdiv) m(ROp, rmod)
/// All cast operations that cast from/to real/signed/unsigned.
#define THORIN_CAST(m) m(Cast, scast) m(Cast, ucast) m(Cast, rcast) m(Cast, s2r) m(Cast, u2r) m(Cast, r2s) m(Cast, r2u)

#define THORIN_I_CMP(m)              /* PM MP G L E                                                   */ \
                     m(ICmp,    f)   /*  o  o o o o - always false                                    */ \
                     m(ICmp,    e)   /*  o  o o o x - equal                                           */ \
                     m(ICmp,    l)   /*  o  o o x o - less (same sign)                                */ \
                     m(ICmp,   le)   /*  o  o o x x - less or equal                                   */ \
                     m(ICmp,    g)   /*  o  o x o o - greater (same sign)                             */ \
                     m(ICmp,   ge)   /*  o  o x o x - greater or equal                                */ \
                     m(ICmp,   gl)   /*  o  o x x o - greater or less                                 */ \
                     m(ICmp,  gle)   /*  o  o x x x - greater or less or equal == same sign           */ \
                     m(ICmp,   mp)   /*  o  x o o o - minus plus                                      */ \
                     m(ICmp,  mpe)   /*  o  x o o x - minus plus or equal                             */ \
                     m(ICmp,   sl)   /*  o  x o x o - signed less                                     */ \
                     m(ICmp,  sle)   /*  o  x o x x - signed less or equal                            */ \
                     m(ICmp,   ug)   /*  o  x x o o - unsigned greater                                */ \
                     m(ICmp,  uge)   /*  o  x x o x - unsigned greater or equal                       */ \
                     m(ICmp, mpgl)   /*  o  x x x o - minus plus or greater or less                   */ \
                     m(ICmp,  npm)   /*  o  x x x x - not plus minus                                  */ \
                     m(ICmp,   pm)   /*  x  o o o o - plus minus                                      */ \
                     m(ICmp,  pme)   /*  x  o o o x - plus minus or equal                             */ \
                     m(ICmp,   ul)   /*  x  o o x o - unsigned less                                   */ \
                     m(ICmp,  ule)   /*  x  o o x x - unsigned less or equal                          */ \
                     m(ICmp,   sg)   /*  x  o x o o - signed greater                                  */ \
                     m(ICmp,  sge)   /*  x  o x o x - signed greater or equal                         */ \
                     m(ICmp, pmgl)   /*  x  o x x o - greater or less or plus minus                   */ \
                     m(ICmp,  nmp)   /*  x  o x x x - not minus plus                                  */ \
                     m(ICmp,   ds)   /*  x  x o o o - different sign                                  */ \
                     m(ICmp,  dse)   /*  x  x o o x - different sign or equal                         */ \
                     m(ICmp,  sul)   /*  x  x o x o - signed or unsigned less                         */ \
                     m(ICmp, sule)   /*  x  x o x x - signed or unsigned less or equal == not greater */ \
                     m(ICmp,  sug)   /*  x  x x o o - signed or unsigned greater                      */ \
                     m(ICmp, suge)   /*  x  x x o x - signed or unsigned greater or equal == not less */ \
                     m(ICmp,   ne)   /*  x  x x x o - not equal                                       */ \
                     m(ICmp,    t)   /*  x  x x x x - always true                                     */

#define THORIN_R_CMP(m)           /* U G L E                                 */ \
                     m(RCmp,   f) /* o o o o - always false                  */ \
                     m(RCmp,   e) /* o o o x - ordered and equal             */ \
                     m(RCmp,   l) /* o o x o - ordered and less              */ \
                     m(RCmp,  le) /* o o x x - ordered and less or equal     */ \
                     m(RCmp,   g) /* o x o o - ordered and greater           */ \
                     m(RCmp,  ge) /* o x o x - ordered and greater or equal  */ \
                     m(RCmp,  ne) /* o x x o - ordered and not equal         */ \
                     m(RCmp,   o) /* o x x x - ordered (no NaNs)             */ \
                     m(RCmp,   u) /* x o o o - unordered (either NaNs)       */ \
                     m(RCmp,  ue) /* x o o x - unordered or equal            */ \
                     m(RCmp,  ul) /* x o x o - unordered or less             */ \
                     m(RCmp, ule) /* x o x x - unordered or less or equal    */ \
                     m(RCmp,  ug) /* x x o o - unordered or greater          */ \
                     m(RCmp, uge) /* x x o x - unordered or greater or equal */ \
                     m(RCmp, une) /* x x x o - unordered or not equal        */ \
                     m(RCmp,   t) /* x x x x - always true                   */

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
    ng = sule,
    nl = suge,
    ss = gle,
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

constexpr WFlags operator|(WFlags a, WFlags b) { return WFlags(int64_t(a) | int64_t(b)); }
constexpr WFlags operator&(WFlags a, WFlags b) { return WFlags(int64_t(a) & int64_t(b)); }

constexpr RFlags operator|(RFlags a, RFlags b) { return RFlags(int64_t(a) | int64_t(b)); }
constexpr RFlags operator&(RFlags a, RFlags b) { return RFlags(int64_t(a) & int64_t(b)); }

constexpr ICmp operator|(ICmp a, ICmp b) { return ICmp(int64_t(a) | int64_t(b)); }
constexpr ICmp operator&(ICmp a, ICmp b) { return ICmp(int64_t(a) & int64_t(b)); }

constexpr RCmp operator|(RCmp a, RCmp b) { return RCmp(int64_t(a) | int64_t(b)); }
constexpr RCmp operator&(RCmp a, RCmp b) { return RCmp(int64_t(a) & int64_t(b)); }

constexpr bool has_feature(WFlags flags, WFlags feature) { return (flags & feature) == feature; }
constexpr bool has_feature(RFlags flags, RFlags feature) { return (flags & feature) == feature; }

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

constexpr const char* op2str(ICmp o) {
    switch (o) {
#define CODE(T, o) case T::o: return "icmp_" #o;
    THORIN_I_CMP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(RCmp o) {
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

namespace thorin {

#define CODE(T, o) + 1_s
template<> constexpr auto Num<core::WOp>  = 0_s THORIN_W_OP (CODE);
template<> constexpr auto Num<core::MOp>  = 0_s THORIN_M_OP (CODE);
template<> constexpr auto Num<core::IOp>  = 0_s THORIN_I_OP (CODE);
template<> constexpr auto Num<core::ROp>  = 0_s THORIN_R_OP (CODE);
template<> constexpr auto Num<core::ICmp> = 0_s THORIN_I_CMP(CODE);
template<> constexpr auto Num<core::RCmp> = 0_s THORIN_R_CMP(CODE);
template<> constexpr auto Num<core::Cast> = 0_s THORIN_CAST (CODE);
#undef CODE

}

#endif
