#ifndef THORIN_ME_TABLES_H
#define THORIN_ME_TABLES_H

#include "thorin/util/utility.h"

namespace thorin::me {

enum class WFlags : int64_t {
    none = 0,
    nsw  = 1 << 0,
    nuw  = 1 << 1,
};

enum class FFlags : int64_t {
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
/// Integer operations that might produce a "division by zero" side effect.
#define THORIN_M_OP(m) m(ZOp, sdiv) m(ZOp, udiv) m(ZOp, smod) m(ZOp, umod)
/// Integer operations that neither take wflags nor do they produce a side effect.
#define THORIN_I_OP(m) m(IOp, ashr) m(IOp, lshr) m(IOp, iand) m(IOp, ior) m(IOp, ixor)
/// Floating point (float) operations that take @p FFlags.
#define THORIN_F_OP(m) m(FOp, fadd) m(FOp, fsub) m(FOp, fmul) m(FOp, fdiv) m(FOp, fmod)
/// All cast operations that cast from/to float/signed/unsigned.
#define THORIN_CAST(m) m(Cast, scast) m(Cast, ucast) m(Cast, fcast) m(Cast, s2f) m(Cast, u2f) m(Cast, f2s) m(Cast, f2u)

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

#define THORIN_F_CMP(m)           /* U G L E                                 */ \
                     m(FCmp,   f) /* o o o o - always false                  */ \
                     m(FCmp,   e) /* o o o x - ordered and equal             */ \
                     m(FCmp,   l) /* o o x o - ordered and less              */ \
                     m(FCmp,  le) /* o o x x - ordered and less or equal     */ \
                     m(FCmp,   g) /* o x o o - ordered and greater           */ \
                     m(FCmp,  ge) /* o x o x - ordered and greater or equal  */ \
                     m(FCmp,  ne) /* o x x o - ordered and not equal         */ \
                     m(FCmp,   o) /* o x x x - ordered (no NaNs)             */ \
                     m(FCmp,   u) /* x o o o - unordered (either NaNs)       */ \
                     m(FCmp,  ue) /* x o o x - unordered or equal            */ \
                     m(FCmp,  ul) /* x o x o - unordered or less             */ \
                     m(FCmp, ule) /* x o x x - unordered or less or equal    */ \
                     m(FCmp,  ug) /* x x o o - unordered or greater          */ \
                     m(FCmp, uge) /* x x o x - unordered or greater or equal */ \
                     m(FCmp, une) /* x x x o - unordered or not equal        */ \
                     m(FCmp,   t) /* x x x x - always true                   */

enum class WOp : size_t {
#define CODE(T, o) o,
    THORIN_W_OP(CODE)
#undef CODE
};

enum class ZOp : size_t {
#define CODE(T, o) o,
    THORIN_M_OP(CODE)
#undef CODE
};

enum class IOp : size_t {
#define CODE(T, o) o,
    THORIN_I_OP(CODE)
#undef CODE
};

enum class FOp : size_t {
#define CODE(T, o) o,
    THORIN_F_OP(CODE)
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

enum class FCmp : size_t {
#define CODE(T, o) o,
    THORIN_F_CMP(CODE)
#undef CODE
};

enum class Cast : size_t {
#define CODE(T, o) o,
    THORIN_CAST(CODE)
#undef CODE
};

constexpr WFlags operator|(WFlags a, WFlags b) { return WFlags(int64_t(a) | int64_t(b)); }
constexpr WFlags operator&(WFlags a, WFlags b) { return WFlags(int64_t(a) & int64_t(b)); }

constexpr FFlags operator|(FFlags a, FFlags b) { return FFlags(int64_t(a) | int64_t(b)); }
constexpr FFlags operator&(FFlags a, FFlags b) { return FFlags(int64_t(a) & int64_t(b)); }

constexpr ICmp operator|(ICmp a, ICmp b) { return ICmp(int64_t(a) | int64_t(b)); }
constexpr ICmp operator&(ICmp a, ICmp b) { return ICmp(int64_t(a) & int64_t(b)); }

constexpr FCmp operator|(FCmp a, FCmp b) { return FCmp(int64_t(a) | int64_t(b)); }
constexpr FCmp operator&(FCmp a, FCmp b) { return FCmp(int64_t(a) & int64_t(b)); }

constexpr bool has_feature(WFlags flags, WFlags feature) { return (flags & feature) == feature; }
constexpr bool has_feature(FFlags flags, FFlags feature) { return (flags & feature) == feature; }

constexpr const char* op2str(WOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_W_OP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* op2str(ZOp o) {
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

constexpr const char* op2str(FOp o) {
    switch (o) {
#define CODE(T, o) case T::o: return #o;
    THORIN_F_OP(CODE)
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

constexpr const char* op2str(FCmp o) {
    switch (o) {
#define CODE(T, o) case T::o: return "rcmp_" #o;
    THORIN_F_CMP(CODE)
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
template<> constexpr auto Num<me::WOp>  = 0_s THORIN_W_OP (CODE);
template<> constexpr auto Num<me::ZOp>  = 0_s THORIN_M_OP (CODE);
template<> constexpr auto Num<me::IOp>  = 0_s THORIN_I_OP (CODE);
template<> constexpr auto Num<me::FOp>  = 0_s THORIN_F_OP (CODE);
template<> constexpr auto Num<me::ICmp> = 0_s THORIN_I_CMP(CODE);
template<> constexpr auto Num<me::FCmp> = 0_s THORIN_F_CMP(CODE);
template<> constexpr auto Num<me::Cast> = 0_s THORIN_CAST (CODE);
#undef CODE

}

#endif
