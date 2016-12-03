#ifndef THORIN_TABLES_H
#define THORIN_TABLES_H

#define THORIN_I_ARITHOP(f) \
    f(iadd) f(isub) f(imul) f(idiv) f(imod) f(ishl) f(ishr) f(iand) f(ior) f(ixor)

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

namespace thorin {

namespace IType {

enum {
#define DECL(x) THORIN_I_TYPE(x),
#undef DECL
    Num,
};

}

namespace RType {

enum {
#define DECL(x) \
    THORIN_R_ARITHOP(x),
#undef DECL
    Num,
};

}

namespace IArithOp {

enum {
#define DECL(x) \
    THORIN_I_ARITHOP(x),
#undef DECL
    Num,
};

}

namespace RArithOp {

enum {
#define DECL(x) \
    THORIN_R_ARITHOP(x),
#undef DECL
    Num,
};

}

enum class IRel {
    T = 0,                  ///< always true
    LT = 1,                 ///< less than
    GT = 2,                 ///< greater than
    EQ = 4,                 ///< qual
    LE = LT | EQ,           ///< less than or equal
    GE = GT | EQ,           ///< greater than or equal
    NE = LT | GT,           ///< not equal
    F = LT | GT | EQ,       ///< always false
};

enum class RRel {
    T = 0,                      ///< always true
    ORD = 1,                    ///< ordered (no NaNs)
    ULT = 2,                    ///< unordered or less than
    UGT = 4,                    ///< unordered or greater than
    UEQ = 8,                    ///< unordered or equal
    ULE = ULT | UEQ,            ///< unordered or less than or equal
    UGE = UGT | UEQ,            ///< unordered or greater than or equal
    UNE = ULT | UGT,            ///< unordered or not equal
    UNO = ULT | UGT | UEQ,      ///< unordered (either NaNs)
    OLT = ORD | ULT,            ///< ordered and less than
    OGT = ORD | UGT,            ///< ordered and greater than
    OEQ = ORD | UEQ,            ///< ordered and equal
    OLE = ORD | ULT | UEQ,      ///< ordered and less than or equal
    ONE = ORD | ULT | UGT,      ///< ordered and not equal
    F = ORD | ULT | UGT | UEQ,  ///< always false
};

}

#endif
