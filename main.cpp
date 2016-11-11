#include "thorin/builder.h"
#include "utils/unicodemanager.h"

using namespace thorin;

void testMatrix();

int main()  {
	prepareUtf8Console();

	testMatrix();

    World w;
    {
        Builder::world_ = &w;
        // λT:*.λx:T.x
        using T = BVar<NAME("T"), BStar>;
        using x = BVar<NAME("x"), T>;
        BLambda<T, BLambda<x, x>>::emit()->dump();
    }

    w.intersection({w.pi(w.nat(), w.nat()), w.pi(w.boolean(), w.boolean())})->dump();

    auto n0 = w.assume(w.nat(), "0");
    auto n1 = w.assume(w.nat(), "1");
    //auto n2 = w.assume(w.nat(), "2");
    //auto n3 = w.assume(w.nat(), "3");
    auto n23 = w.assume(w.nat(), "23");
    auto n42 = w.assume(w.nat(), "42");

    // λT:*.λx:T.x
    auto T_1 = w.var(w.star(), 0, "T");
    auto T_2 = w.var(w.star(), 1, "T");
    auto x = w.var(T_2, 0, "x");
    auto poly_id = w.lambda(T_2->type(), w.lambda(T_1, x));
    poly_id->dump();
    poly_id->type()->dump();

    // λx:w.nat().x
    auto int_id = w.app(poly_id, w.nat());
    int_id->dump();
    int_id->type()->dump();

    auto fst = w.lambda({w.nat(), w.nat()}, w.app(w.var({w.nat(), w.nat()}, 0, "pair"), n0));
    auto snd = w.lambda({w.nat(), w.nat()}, w.app(w.var({w.nat(), w.nat()}, 0, "pair"), n1));
    w.app(fst, {n23, n42})->dump(); // 23
    w.app(snd, {n23, n42})->dump(); // 42


    //// 2
    //w.app(int_id, n2)->dump();

    //// 3
    //w.extract(w.tuple({n1, n2, n3}), 2)->dump();
    ////w.extract(n3, 0)->dump();

    //auto make_pair = w.assume(w.pi(w.unit(), w.sigma({w.nat(), w.nat()})), "make_pair");
	//make_pair->dump();
    //w.app(make_pair, n1)->dump();

    //auto plus = w.assume(w.pi({w.nat(), w.nat()}, w.nat()), "+");
    //plus->type()->dump();
    //w.app(int_id, w.app(plus, {w.app(plus, {n1, n2}), n3}))->dump();

    auto Arr = w.assume(w.pi({w.nat(), w.pi(w.nat(), w.star())}, w.star()), "Arr");
    Defs dom{w.nat(), w.star()};
    auto _Arr = w.lambda(dom, w.app(Arr, {w.extract(w.var(dom, 0), 0), w.lambda(w.nat(), w.extract(w.var(dom, 1), 1))}));
    _Arr->dump();

    auto arr = w.app(_Arr, {n23, w.nat()});
    arr->dump();

    {
#if 0
        BDEF(Nat, w.nat());
        BDEF(N0, n0);
        BDEF(N1, n1);
        BDEF(N23, n23);
        BDEF(ARR, Arr);
        using v = BVar<NAME("v"), Nat, BStar>;
        using _ARR = BLambda<v, BApp<ARR, BApp<v, N0>, BLambda<B_<Nat>, BApp<v, N1>>>>;
        _ARR::emit()->dump();

        BApp<_ARR, N23, Nat>::emit()->dump();
#endif
    }
}
