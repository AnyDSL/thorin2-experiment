#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Matrix, Misc) {
    World w;
    auto Nat = w.type_nat();
    auto Star = w.star();
    auto Float = w.axiom(Star, {"Float"});
    auto opFloatPlus = w.axiom(w.pi(w.sigma({ Float, Float }), Float), {"+"});
    /*auto opFloatMult =*/ w.axiom(w.pi(w.sigma({ Float, Float }), Float), {"x"});
    auto cFloatZero = w.axiom(Float, {"0.0"});

    // Some testing
    auto getSecondValue = w.lambda(w.tuple({ Nat, Nat }), w.extract(w.var({ Nat, Nat }, 0), 1));
    getSecondValue->dump();
    getSecondValue->type()->dump();

    auto ArrTypeFuncT = w.pi(Nat, Star);
    ArrTypeFuncT->dump();
    // Type constructor for arrays
    // axiom ArrT: (Nat, Nat -> *) -> *;
    auto ArrT = w.axiom(w.pi({Nat, w.pi(Nat, Star)}, Star), {"ArrT"});
    ArrT->type()->dump();

    // Array Creator
    // (n: Nat, tf: ArrTypeFuncT) -> (pi i:Nat -> tf i) -> (ArrT (n, tf))
    auto arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 1));
    auto arrcreatorfunctype = w.pi(Nat, w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 1), 1), w.var(Nat, 0)));
    auto ArrCreate = w.axiom(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrcreatorfunctype, arrtype)), {"ArrCreate"});
    ArrCreate->dump();
    ArrCreate->type()->dump();

    // Array Getter
    // (n: Nat, tf: ArrTypeFuncT) -> (ArrT (n, tf)) -> (pi i:Nat -> tf i)
    arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 0));
    auto resulttype = w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 2), 1), w.var(Nat, 0));
    auto ArrGet = w.axiom(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrtype, w.pi(Nat, resulttype))), {"ArrGet"});
    ArrGet->dump();
    ArrGet->type()->dump();

    // Matrix Type
    //auto MatrixType = w.lambda()

    // MapReduce
    auto t = [&](int i) {return w.var(Star, i); };
    auto reduce = w.axiom(w.pi(Star, //t
        w.pi(w.pi(w.sigma({ t(0), t(0) }), t(1)), // (t x t -> t)
                w.pi(t(1), w.pi(Nat, // t startvalue, Nat len
                    w.pi(w.pi(Nat, t(4)), // (Nat -> t)
                        t(4)
                    )
                ))
            )
    ), {"reduce"});
    reduce->type()->dump();
    auto sum = w.app(w.app(w.app(reduce, Float), opFloatPlus), cFloatZero, {"sum"});
    sum->type()->dump();

}
