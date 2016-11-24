#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;


#define printValue(x) do{ printUtf8(#x " = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(#x ": "); x->type()->dump(); }while(0)



void testMatrix() {
    World w;
    auto Nat = w.nat();
    auto Star = w.star();
    auto Float = w.assume(Star, "Float");
    auto opFloatPlus = w.assume(w.pi(w.sigma({ Float, Float }), Float), "+");
    auto opFloatMult = w.assume(w.pi(w.sigma({ Float, Float }), Float), "x");
    auto cFloatZero = w.assume(Float, "0.0");

    // Some testing
    auto getSecondValue = w.lambda(w.tuple({ Nat, Nat }), w.extract(w.var({ Nat, Nat }, 0), 1));
    printValue(getSecondValue);
    printType(getSecondValue);


    auto ArrTypeFuncT = w.pi(Nat, Star);
    printValue(ArrTypeFuncT);
    // Type constructor for arrays
    // assume ArrT: (Nat, Nat -> *) -> *;
    auto ArrT = w.assume(w.pi({Nat, w.pi(Nat, Star)}, Star), "ArrT");
    printType(ArrT);

    // Array Creator
    // (n: Nat, tf: ArrTypeFuncT) -> (pi i:Nat -> tf i) -> (ArrT (n, tf))
    auto arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 1));
    auto arrcreatorfunctype = w.pi(Nat, w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 1), 1), w.var(Nat, 0)));
    auto ArrCreate = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrcreatorfunctype, arrtype)), "ArrCreate");
    printValue(ArrCreate);
    printType(ArrCreate);

    // Array Getter
    // (n: Nat, tf: ArrTypeFuncT) -> (ArrT (n, tf)) -> (pi i:Nat -> tf i)
    arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 0));
    auto resulttype = w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 2), 1), w.var(Nat, 0));
    auto ArrGet = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrtype, w.pi(Nat, resulttype))), "ArrGet");
    printValue(ArrGet);
    printType(ArrGet);

    // Matrix Type
    //auto MatrixType = w.lambda()

    // MapReduce
    auto t = [&](int i) {return w.var(Star, i); };
    auto reduce = w.assume(w.pi(Star, //t
        w.pi(w.pi(w.sigma({ t(0), t(0) }), t(1)), // (t x t -> t)
                w.pi(t(1), w.pi(Nat, // t startvalue, Nat len
                    w.pi(w.pi(Nat, t(4)), // (Nat -> t)
                        t(4)
                    )
                ))
            )
    ), "reduce");
    printType(reduce);
    auto sum = w.app(w.app(w.app(reduce, Float), opFloatPlus), cFloatZero, "sum");
    printType(sum);

}


