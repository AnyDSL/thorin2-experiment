#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#define THORIN_I_ARITHOP(f) \
    f(iadd) f(isub) f(imul) f(idiv) f(imod) f(ishl) f(ishr) f(iand) f(i_or) f(ixor)

#define THORIN_R_ARITHOP(f) \
    f(radd) f(rsub) f(rmul) f(rdiv) f(rmod)

#define THORIN_I_WIDTH(f, x) \
    f(x ## 1) f(x ## 8) f(x ## 16) f(x ## 32) f(x ## 64)

#define THORIN_I_TYPE(f) \
    THORIN_I_WIDTH(f, sw) THORIN_I_WIDTH(f, uw) THORIN_I_WIDTH(f, so) THORIN_I_WIDTH(f, uo)

#define THORIN_R_WIDTH(f, x) \
    f(x ## 16) f(x ## 32) f(x ## 64)

#define THORIN_R_TYPE(g) \
    THORIN_I_WIDTH(g, f) THORIN_I_WIDTH(g, p)

#define THORIN_I_REL(f) \
    /*       E G L                         */ \
    f(T)  /* o o o - always true           */ \
    f(LT) /* o o x - less than             */ \
    f(GT) /* o x o - greater than          */ \
    f(NE) /* o x x - not equal             */ \
    f(EQ) /* x o o - qual                  */ \
    f(LE) /* x o x - less than or equal    */ \
    f(GE) /* x x o - greater than or equal */ \
    f(F)  /* x x x - always false          */

#define THORIN_R_REL(f) \
    /*        O E G L                                      */ \
    f(T)   /* o o o o - always true                        */ \
    f(ULT) /* o o o x - unordered or less than             */ \
    f(UGT) /* o o x o - unordered or greater than          */ \
    f(UNE) /* o o x x - unordered or not equal             */ \
    f(UEQ) /* o x o o - unordered or qual                  */ \
    f(ULE) /* o x o x - unordered or less than or equal    */ \
    f(UGE) /* o x x o - unordered or greater than or equal */ \
    f(UNO) /* o x x x - unordered (either NaNs)            */ \
    f(ORD) /* x o o o - ordered (no NaNs)                  */ \
    f(OLT) /* x o o x - ordered and less than              */ \
    f(OGT) /* x o x o - ordered and greater than           */ \
    f(ONE) /* x o x x - ordered and not equal              */ \
    f(OEQ) /* x x o o - ordered and qual                   */ \
    f(OLE) /* x x o x - ordered and less than or equal     */ \
    f(OGE) /* x x x o - ordered and greater than or equal  */ \
    f(F)   /* x x x x - always false                       */

namespace thorin {

enum class IType {
#define DECL(x) THORIN_I_TYPE(x),
#undef DECL
    Num,
};

enum class RType {
#define DECL(x) \
    THORIN_R_ARITHOP(x),
#undef DECL
    Num,
};

enum class IARithOp {
#define DECL(x) \
    THORIN_I_ARITHOP(x),
#undef DECL
    Num,
};

enum class RArithOp {
#define DECL(x) \
    THORIN_R_ARITHOP(x),
#undef DECL
    Num,
};

enum class IRel {
#define DECL(x) \
    THORIN_I_REL(x),
#undef DECL
    Num,
};

enum class RRel {
#define DECL(x) \
    THORIN_R_REL(x),
#undef DECL
    Num,
};

}

#endif
