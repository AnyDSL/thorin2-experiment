#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

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

}

#endif
