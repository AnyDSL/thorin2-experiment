#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include <half.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "thorin/qualifier.h"

#define THORIN_I_WIDTH(m) m(1) m(8) m(16) m(32) m(64)
#define THORIN_R_WIDTH(m) m(16) m(32) m(64)

#define THORIN_I_ARITHOP(m) \
    m(iadd) m(isub) m(imul) m(idiv) m(imod) m(ishl) m(ishr) m(iand) m(i_or) m(ixor)

#define THORIN_R_ARITHOP(m) \
    m(radd) m(rsub) m(rmul) m(rdiv) m(rmod)

#define THORIN_I_TYPE(m) \
    m(uuo1, u1) m(uuo8, u8) m(uuo16, u16) m(uuo32, u32) m(uuo64, u64) \
    m(uuw1, u1) m(uuw8, u8) m(uuw16, u16) m(uuw32, u32) m(uuw64, u64) \
    m(uso1, s1) m(uso8, s8) m(uso16, s16) m(uso32, s32) m(uso64, s64) \
    m(usw1, s1) m(usw8, s8) m(usw16, s16) m(usw32, s32) m(usw64, s64) \
    m(ruo1, u1) m(ruo8, u8) m(ruo16, u16) m(ruo32, u32) m(ruo64, u64) \
    m(ruw1, u1) m(ruw8, u8) m(ruw16, u16) m(ruw32, u32) m(ruw64, u64) \
    m(rso1, s1) m(rso8, s8) m(rso16, s16) m(rso32, s32) m(rso64, s64) \
    m(rsw1, s1) m(rsw8, s8) m(rsw16, s16) m(rsw32, s32) m(rsw64, s64) \
    m(auo1, u1) m(auo8, u8) m(auo16, u16) m(auo32, u32) m(auo64, u64) \
    m(auw1, u1) m(auw8, u8) m(auw16, u16) m(auw32, u32) m(auw64, u64) \
    m(aso1, s1) m(aso8, s8) m(aso16, s16) m(aso32, s32) m(aso64, s64) \
    m(asw1, s1) m(asw8, s8) m(asw16, s16) m(asw32, s32) m(asw64, s64) \
    m(luo1, u1) m(luo8, u8) m(luo16, u16) m(luo32, u32) m(luo64, u64) \
    m(luw1, u1) m(luw8, u8) m(luw16, u16) m(luw32, u32) m(luw64, u64) \
    m(lso1, s1) m(lso8, s8) m(lso16, s16) m(lso32, s32) m(lso64, s64) \
    m(lsw1, s1) m(lsw8, s8) m(lsw16, s16) m(lsw32, s32) m(lsw64, s64)

#define THORIN_R_TYPE(m) \
    m(uf16, r16) m(uf32, r32) m(uf64, r64) \
    m(up16, r16) m(up32, r32) m(up64, r64) \
    m(rf16, r16) m(rf32, r32) m(rf64, r64) \
    m(rp16, r16) m(rp32, r32) m(rp64, r64) \
    m(af16, r16) m(af32, r32) m(af64, r64) \
    m(ap16, r16) m(ap32, r32) m(ap64, r64) \
    m(lf16, r16) m(lf32, r32) m(lf64, r64) \
    m(lp16, r16) m(lp32, r32) m(lp64, r64)

#define THORIN_I_REL(f) \
    /*       E G L                         */ \
    f(t)  /* o o o - always true           */ \
    f(lt) /* o o x - less than             */ \
    f(gt) /* o x o - greater than          */ \
    f(ne) /* o x x - not equal             */ \
    f(eq) /* x o o - equal                 */ \
    f(le) /* x o x - less than or equal    */ \
    f(ge) /* x x o - greater than or equal */ \
    f(f)  /* x x x - always false          */

#define THORIN_R_REL(f) \
    /*        O E G L                                      */ \
    f(t)   /* o o o o - always true                        */ \
    f(ult) /* o o o x - unordered or less than             */ \
    f(ugt) /* o o x o - unordered or greater than          */ \
    f(une) /* o o x x - unordered or not equal             */ \
    f(ueq) /* o x o o - unordered or equal                 */ \
    f(ule) /* o x o x - unordered or less than or equal    */ \
    f(uge) /* o x x o - unordered or greater than or equal */ \
    f(uno) /* o x x x - unordered (either NaNs)            */ \
    f(ord) /* x o o o - ordered (no NaNs)                  */ \
    f(olt) /* x o o x - ordered and less than              */ \
    f(ogt) /* x o x o - ordered and greater than           */ \
    f(one) /* x o x x - ordered and not equal              */ \
    f(oeq) /* x x o o - ordered and equal                  */ \
    f(ole) /* x x o x - ordered and less than or equal     */ \
    f(oge) /* x x x o - ordered and greater than or equal  */ \
    f(f)   /* x x x x - always false                       */

namespace thorin {

enum class ITypeFlags {
        // S W
    uo, // o o
    uw, // o x
    so, // x o
    sw, // x x
    Num
};

enum class RTypeFlags {
    f, // fast
    p, // precise
    Num
};

constexpr size_t iwidth2index(size_t i) { return i == 1 ? 0 : log2(i)-2; }
constexpr size_t rwidth2index(size_t i) { return log2(i)-4; }
constexpr size_t index2iwidth(size_t i) { return i == 0 ? 1 : 1 << (i+2); }
constexpr size_t index2rwidth(size_t i) { return 1 << (i+4); }

enum class IType {
#define CODE(x, y) THORIN_I_TYPE(x),
#undef CODE
    Num
};

enum class RType {
#define CODE(x, y) \
    THORIN_R_ARITHOP(x),
#undef CODE
    Num
};

enum class IArithop {
#define CODE(x) \
    THORIN_I_ARITHOP(x),
#undef CODE
    Num
};

enum class RArithop {
#define CODE(x) \
    THORIN_R_ARITHOP(x),
#undef CODE
    Num
};

enum class IRel {
#define CODE(x) \
    THORIN_I_REL(x),
#undef CODE
    Num
};

enum class RRel {
#define CODE(x) \
    THORIN_R_REL(x),
#undef CODE
    Num
};

typedef bool s1; typedef  int8_t s8; typedef  int16_t s16; typedef  int32_t s32; typedef  int64_t s64;
typedef bool u1; typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
/*                        */ typedef half_float::half r16; typedef float    r32; typedef double   r64;
typedef Qualifier qualifier;

#define CODE(x, y) typedef y x;
THORIN_I_TYPE(CODE)
THORIN_R_TYPE(CODE)
#undef CODE

#define THORIN_TYPES(m) \
    m(s8)   m(s16) m(s32) m(s64) \
    m(u8)   m(u16) m(u32) m(u64) \
    m(bool) m(r16) m(r32) m(r64) \
    m(qualifier)

union Box {
public:
    Box()        { u64_ = 0; }

#define CODE(T) \
    Box(T val) { u64_ = 0; T ## _ = val; }
    THORIN_TYPES(CODE)
#undef CODE

#define CODE(T) \
    T get_ ## T() const { return T ## _; }
    THORIN_TYPES(CODE)
#undef CODE
    s1 get_s1() const { return s1_; }
    u1 get_u1() const { return u1_; }

    bool operator==(Box other) const { return this->u64_ == other.get_u64(); }

private:
    s1 s1_;
    u1 u1_;
#define CODE(T) \
    T T ## _;
    THORIN_TYPES(CODE)
#undef CODE
};

}

#endif
