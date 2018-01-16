#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

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
using half_float::half;

}

#include "thorin/qualifier.h"
#include "thorin/util/utility.h"

#define THORIN_I_FLAGS(m) m(uo) /* o o */ /*unsigned overflow   */ \
                          m(uw) /* o x */ /*unsigned wraparound */ \
                          m(so) /* x o */ /*  signed overflow   */ \
                          m(sw) /* x x */ /*  signed wraparound */
#define THORIN_R_FLAGS(m) m(f) m(p) // fast, precise - more fine-grained flags are planned in the future

#define THORIN_I_ARITHOP(m) m(iadd) m(isub) m(imul) m(idiv) m(imod) m(ishl) m(ishr) m(iand) m(ior) m(ixor)
#define THORIN_R_ARITHOP(m) m(radd) m(rsub) m(rmul) m(rdiv) m(rmod)

#define THORIN_I_REL(m)   /* E G L                         */ \
                    m(t)  /* o o o - always true           */ \
                    m(lt) /* o o x - less than             */ \
                    m(gt) /* o x o - greater than          */ \
                    m(ne) /* o x x - not equal             */ \
                    m(eq) /* x o o - equal                 */ \
                    m(le) /* x o x - less than or equal    */ \
                    m(ge) /* x x o - greater than or equal */ \
                    m(f)  /* x x x - always false          */

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

namespace thorin {

enum class iflags {
#define CODE(f) f,
    THORIN_I_FLAGS(CODE)
#undef CODE
    Num
};

enum class rflags {
#define CODE(f) f,
    THORIN_R_FLAGS(CODE)
#undef CODE
    Num
};

enum iarithop : size_t {
#define CODE(O) O,
    THORIN_I_ARITHOP(CODE)
#undef CODE
    Num_IArithOp
};

enum rarithop : size_t {
#define CODE(O) O,
    THORIN_R_ARITHOP(CODE)
#undef CODE
    Num_RArithOp
};

constexpr const char* iarithop2str(iarithop o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_I_ARITHOP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

constexpr const char* rarithop2str(rarithop o) {
    switch (o) {
#define CODE(O) case O: return #O;
    THORIN_R_ARITHOP(CODE)
#undef CODE
        default: THORIN_UNREACHABLE;
    }
}

enum class irel {
#define CODE(f) f,
    THORIN_I_REL(CODE)
#undef CODE
    Num
};

enum class rrel {
#define CODE(f) f,
    THORIN_R_REL(CODE)
#undef CODE
    Num
};

typedef bool u1; typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef bool s1; typedef  int8_t s8; typedef  int16_t s16; typedef  int32_t s32; typedef  int64_t s64;
/*                        */ typedef half_float::half r16; typedef    float r32; typedef   double r64;
typedef Qualifier qualifier;

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

    bool operator==(Box other) const { return this->u64_ == other.get_u64(); }
    template<typename T> T& get() { return *((T*)this); }

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
