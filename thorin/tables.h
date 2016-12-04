#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#include <half.hpp>

#define THORIN_I_WIDTH(m) m(1) m(8) m(16) m(32) m(64)
#define THORIN_R_WIDTH(m) m(16) m(32) m(64)

#define THORIN_I_ARITHOP(m) \
    m(iadd) m(isub) m(imul) m(idiv) m(imod) m(ishl) m(ishr) m(iand) m(i_or) m(ixor)

#define THORIN_R_ARITHOP(m) \
    m(radd) m(rsub) m(rmul) m(rdiv) m(rmod)

#define THORIN_I_TYPE(m) \
    m(sw1, s1) m(sw8, s8) m(sw16, s16) m(sw32, s32) m(sw64, s64) \
    m(so1, s1) m(so8, s8) m(so16, s16) m(so32, s32) m(so64, s64) \
    m(uw1, u1) m(uw8, u8) m(uw16, u16) m(uw32, u32) m(uw64, u64) \
    m(uo1, u1) m(uo8, u8) m(uo16, u16) m(uo32, u32) m(uo64, u64)

#define THORIN_R_TYPE(m) \
    m(f16, r16) m(f32, r32) m(f64, r64) \
    m(p16, r32) m(p32, r32) m(p64, r64)

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
    f(oeq) /* x x o o - ordered and qual                   */ \
    f(ole) /* x x o x - ordered and less than or equal     */ \
    f(oge) /* x x x o - ordered and greater than or equal  */ \
    f(f)   /* x x x x - always false                       */

namespace thorin {

enum class IType {
#define CODE(x, y) THORIN_I_TYPE(x),
#undef CODE
    Num,
};

enum class RType {
#define CODE(x, y) \
    THORIN_R_ARITHOP(x),
#undef CODE
    Num,
};

enum class IARithOp {
#define CODE(x) \
    THORIN_I_ARITHOP(x),
#undef CODE
    Num,
};

enum class RArithOp {
#define CODE(x) \
    THORIN_R_ARITHOP(x),
#undef CODE
    Num,
};

enum class IRel {
#define CODE(x) \
    THORIN_I_REL(x),
#undef CODE
    Num,
};

enum class RRel {
#define CODE(x) \
    THORIN_R_REL(x),
#undef CODE
    Num,
};

typedef bool s1; typedef  int8_t s8; typedef  int16_t s16; typedef  int32_t s32; typedef  int64_t s64;
typedef bool u1; typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
/*                        */ typedef half_float::half r16; typedef float    r32; typedef double   r64;

#define CODE(x, y) typedef y x;
THORIN_I_TYPE(CODE)
THORIN_R_TYPE(CODE)
#undef CODE


#define THORIN_TYPES(m) \
    m(s8)   m(s16) m(s32) m(s64) \
    m(u8)   m(u16) m(u32) m(u64) \
    m(bool) m(r16) m(r32) m(r64)

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
