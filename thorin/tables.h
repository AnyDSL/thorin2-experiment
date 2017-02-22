#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#include <boost/preprocessor/seq.hpp>

#define HALF_ENABLE_CPP11_USER_LITERALS 1

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include <half.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace thorin {

using namespace half_float::literal;

}

#include "thorin/qualifier.h"

#define T_STR(x) #x

#define T_CAT_1(a)               BOOST_PP_SEQ_CAT(a)
#define T_CAT_2(a,b)             T_CAT__(a,b)
#define T_CAT__(a,b)             a##b
#define T_CAT_3(a,b,c)           T_CAT_2(T_CAT_2(a,b),c)
#define T_CAT_4(a,b,c,d)         T_CAT_2(T_CAT_3(a,b,c),d)
#define T_CAT_5(a,b,c,d,e)       T_CAT_2(T_CAT_4(a,b,c,d),e)
#define T_CAT_6(a,b,c,d,e,f)     T_CAT_2(T_CAT_5(a,b,c,d,e),f)
#define T_CAT_7(a,b,c,d,e,f,g)   T_CAT_2(T_CAT_6(a,b,c,d,e,f),g)
#define T_CAT_8(a,b,c,d,e,f,g,h) T_CAT_2(T_CAT_7(a,b,c,d,e,f,g),h)

#define T_GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8, f, ...) f
#define T_CAT(...) T_GET_MACRO(__VA_ARGS__,T_CAT_8,T_CAT_7,T_CAT_6,T_CAT_5,T_CAT_4,T_CAT_3,T_CAT_2,T_CAT_1)(__VA_ARGS__)

#define T_ELEM(i,seq)                 BOOST_PP_SEQ_ELEM(i,seq)
#define T_ENUM(seq)                   BOOST_PP_SEQ_ENUM(seq)
#define T_FOR_EACH(macro,data,seq)    BOOST_PP_SEQ_FOR_EACH(macro,data,seq)
#define T_FOR_EACH_PRODUCT(macro,seq) BOOST_PP_SEQ_FOR_EACH_PRODUCT(macro,seq)

#define T_HEAD(seq) T_ELEM(0,seq)
#define T_TAIL(seq) T_TAIL_ seq
#define T_TAIL_(seq)

#define THORIN_Q (u)(r)(a)(l)

#define THORIN_I_WIDTH (1)(8)(16)(32)(64)
#define THORIN_R_WIDTH       (16)(32)(64)

#define THORIN_I_FLAGS (uo) /* o o */ \
                       (uw) /* o x */ \
                       (so) /* x o */ \
                       (sw) /* x x */
#define THORIN_R_FLAGS (f)(p) // fast, precise

#define THORIN_I_ARITHOP (iadd)(isub)(imul)(idiv)(imod)(ishl)(ishr)(iand)(ior)(ixor)
#define THORIN_R_ARITHOP (radd)(rsub)(rmul)(rdiv)(rmod)

#define THORIN_I_REL /*     E G L                         */ \
                    (t)  /* o o o - always true           */ \
                    (lt) /* o o x - less than             */ \
                    (gt) /* o x o - greater than          */ \
                    (ne) /* o x x - not equal             */ \
                    (eq) /* x o o - equal                 */ \
                    (le) /* x o x - less than or equal    */ \
                    (ge) /* x x o - greater than or equal */ \
                    (f)  /* x x x - always false          */

#define THORIN_R_REL /*       O E G L                                      */ \
                     (t)   /* o o o o - always true                        */ \
                     (ult) /* o o o x - unordered or less than             */ \
                     (ugt) /* o o x o - unordered or greater than          */ \
                     (une) /* o o x x - unordered or not equal             */ \
                     (ueq) /* o x o o - unordered or equal                 */ \
                     (ule) /* o x o x - unordered or less than or equal    */ \
                     (uge) /* o x x o - unordered or greater than or equal */ \
                     (uno) /* o x x x - unordered (either NaNs)            */ \
                     (ord) /* x o o o - ordered (no NaNs)                  */ \
                     (olt) /* x o o x - ordered and less than              */ \
                     (ogt) /* x o x o - ordered and greater than           */ \
                     (one) /* x o x x - ordered and not equal              */ \
                     (oeq) /* x x o o - ordered and equal                  */ \
                     (ole) /* x x o x - ordered and less than or equal     */ \
                     (oge) /* x x x o - ordered and greater than or equal  */ \
                     (f)   /* x x x x - always false                       */

namespace thorin {

enum class iflags {
    T_ENUM(THORIN_I_FLAGS),
    Num
};

enum class rflags {
    T_ENUM(THORIN_R_FLAGS),
    Num
};

#define CODE(r, ir, x) T_CAT(w, x),
enum class iwidth {
    T_FOR_EACH(CODE, _, THORIN_I_WIDTH)
    Num
};

enum class rwidth {
    T_FOR_EACH(CODE, _, THORIN_R_WIDTH)
    Num
};
#undef CODE

inline size_t iwidth2index(size_t i) { return i == 1 ? 0 : log2(i)-2; }
inline size_t rwidth2index(size_t i) { return log2(i)-4; }
inline size_t index2iwidth(size_t i) { return i == 0 ? 1 : 1 << (i+2); }
inline size_t index2rwidth(size_t i) { return 1 << (i+4); }

enum class iarithop {
    T_ENUM(THORIN_I_ARITHOP),
    Num
};

enum class rarithop {
    T_ENUM(THORIN_R_ARITHOP),
    Num
};

enum class irel {
    T_ENUM(THORIN_I_REL),
    Num
};

enum class rrel {
    T_ENUM(THORIN_R_REL),
    Num
};

typedef bool u1; typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef bool s1; typedef  int8_t s8; typedef  int16_t s16; typedef  int32_t s32; typedef  int64_t s64;
/*                        */ typedef half_float::half r16; typedef    float r32; typedef   double r64;
typedef Qualifier qualifier;

#define CODE(r, x) \
    typedef T_CAT(T_ELEM(0, x), T_ELEM(2, x)) T_CAT(x);
    T_FOR_EACH_PRODUCT(CODE, ((u)(s))((o)(w))(THORIN_I_WIDTH))
#undef CODE

#define CODE(rest, x) \
    typedef T_CAT(r, T_ELEM(1, x)) T_CAT(x);
    T_FOR_EACH_PRODUCT(CODE, (THORIN_R_FLAGS)(THORIN_R_WIDTH))
#undef CODE

#define THORIN_TYPES(m) \
    m(s8)   m(s16) m(s32) m(s64) \
    m(u8)   m(u16) m(u32) m(u64) \
    m(bool) m(r16) m(r32) m(r64) \
    m(qualifier)

union Box {
public:
    Box() { u64_ = 0; }

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
