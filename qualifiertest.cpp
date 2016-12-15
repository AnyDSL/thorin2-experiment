#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << "<" << x->gid() << "> " << (x->name() == "" ? #x : x->name()) << " = " << x << ": " << x->type() << endl; }while(0)

void testQualifiers() {
    auto U = Qualifier::Unrestricted;
    auto R = Qualifier::Relevant;
    auto A = Qualifier::Affine;
    auto L = Qualifier::Linear;

    assert(A < U);
    assert(R < U);
    assert(L < U);
    assert(L < A);
    assert(L < R);

    assert(meet(U, U) == U);
    assert(meet(A, U) == A);
    assert(meet(R, U) == R);
    assert(meet(L, U) == L);
    assert(meet(A, A) == A);
    assert(meet(A, R) == L);
    assert(meet(L, A) == L);
    assert(meet(L, R) == L);

    World w;
    auto Star = w.star();
    auto Nat = w.nat();
    auto n42 = w.axiom(Nat, {"42"});
    auto ANat = w.nat(A);
    auto RNat = w.nat(R);
    /*auto LNat =*/ w.nat(L);
    auto an0 = w.axiom(ANat, {"0"});
    auto anx = w.var(ANat, 0, {"x"});
    auto anid = w.lambda(ANat, anx, {"anid"});
    /*auto app1 =*/ w.app(anid, an0);
    assert(is_error(w.app(anid, an0)));

    auto tuple_type = w.sigma({ANat, RNat});
    assert(tuple_type->qualifier() == L);
    auto an1 = w.axiom(ANat, {"1"});
    auto rn0 = w.axiom(RNat, {"0"});
    auto tuple = w.tuple({an1, rn0});
    assert(tuple->type() == tuple_type);
    auto tuple_app0 = w.extract(tuple, 0);
    assert(w.extract(tuple, 0) == tuple_app0);

    auto a_id_type = w.pi(Nat, Nat, A);
    auto nx = w.var(Nat, 0, {"x"});
    auto a_id = w.lambda(Nat, nx, A, {"a_id"});
    assert(a_id_type == a_id->type());
    auto n0 = w.axiom(Nat, {"0"});
    /*auto a_id_app =*/ w.app(a_id, n0);
    assert(is_error(w.app(a_id, n0)));

    // λᴬT:*.λx:ᴬT.x
    auto aT1 = w.var(w.star(A), 0, {"T"});
    auto aT2 = w.var(w.star(A), 1, {"T"});
    auto x = w.var(aT2, 0, {"x"});
    auto poly_aid = w.lambda(aT2->type(), w.lambda(aT1, x));
    std::cout << poly_aid << " : " << poly_aid->type() << endl;

    // λx:ᴬNat.x
    auto anid2 = w.app(poly_aid, ANat);
    std::cout << anid2 << " : " << anid2->type() << endl;

    auto T = [&](int i){ return w.var(Star, i, {"T"}); };
    std::cout << "--- Unrestricted Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi(Star, Star), {"Ref"});
        print_value_type(Ref);
        auto app_Ref_T0 = w.app(Ref, T(0));
        auto NewRef = w.axiom(w.pi({Star, T(0)}, w.app(Ref, T(1))), {"NewRef"});
        print_value_type(NewRef);
        auto ReadRef = w.axiom(w.pi({Star, w.app(Ref, T(0))}, T(1)), {"ReadRef"});
        print_value_type(ReadRef);
        auto WriteRef = w.axiom(w.pi({Star, app_Ref_T0, T(0)}, w.unit()), {"WriteRef"});
        print_value_type(WriteRef);
        auto FreeRef = w.axiom(w.pi({Star, app_Ref_T0}, w.unit()), {"FreeRef"});
        print_value_type(FreeRef);
    }
    std::cout << "--- Affine Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi(Star, w.star(A)), {"ARef"});
        print_value_type(Ref);
        auto app_Ref_T0 = w.app(Ref, T(0));
        assert(app_Ref_T0->qualifier() == A);
        auto NewRef = w.axiom(w.pi({Star, T(0)}, w.app(Ref, T(1))), {"NewARef"});
        print_value_type(NewRef);
        // ReadRef : Π(*).Π(ARef[<0:*>]).Σ(<1:*>, ARef[<2:*>])
        auto ReadRef = w.axiom(w.pi({Star, app_Ref_T0}, w.sigma({T(1), w.app(Ref, T(2))})),
                {"ReadARef"});
        print_value_type(ReadRef);
        auto WriteRef = w.axiom(w.pi({Star, app_Ref_T0, T(0)}, w.unit()), {"WriteARef"});
        print_value_type(WriteRef);
        auto FreeRef = w.axiom(w.pi({Star, app_Ref_T0}, w.unit()), {"FreeARef"});
        print_value_type(FreeRef);
    }
    std::cout << "--- Affine Capabilities for Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi({Star, Star}, Star), {"CRef"});
        auto Cap = w.axiom(w.pi(Star, w.star(A)), {"ACap"});
        print_value_type(Ref);
        print_value_type(Cap);
        auto C = [&](int i){ return w.var(Star, i, {"C"}); };
        auto NewRef = w.axiom(w.pi(Star,
                                   w.pi(T(0),
                                        w.sigma({Star,
                                                 w.app(Ref, {T(2), C(0)}),
                                                 w.app(Cap, C(1))}))),
                {"NewCRef"});
        print_value_type(NewRef);
        // ReadRef : Π(T:*).Π(C:*, CRef[T, C], ᴬACap[C]).ᴬΣ(T, ᴬACap[C])
        auto ReadRef = w.axiom(w.pi(Star, w.pi({Star, w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1))},
                                                    w.sigma({T(3), w.app(Cap, C(3))}))), {"ReadCRef"});
        print_value_type(ReadRef);
        // AliasReadRef : Π(T:*, C:*, CRef[T, C]).T
        auto AliasReadRef = w.axiom(w.pi({Star, Star, w.app(Ref, {T(1), C(0)})}, T(2)),
                                    {"AliasReadCRef"});
        print_value_type(AliasReadRef);
        // WriteRef : Π(T:*, C:*, CRef[T, C], ᴬACap[C], T).ᴬACap[C]
        auto WriteRef = w.axiom(w.pi({Star, Star, w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1)), T(3)},
                                      w.app(Cap, C(3))), {"WriteCRef"});
        print_value_type(WriteRef);
        // FreeRef : Π(T:*, C:*, CRef[T, C], ᴬACap[C]).()
        auto FreeRef = w.axiom(w.pi({Star, Star, w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1))},
                                     w.unit()), {"FreeCRef"});
        print_value_type(FreeRef);
        auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
        auto phantom = w.extract(ref42, 0);
        print_value_type(ref42);
        auto ref = w.extract(ref42, 1, {"ref"});
        print_value_type(ref);
        auto cap = w.extract(ref42, 2, {"cap"});
        print_value_type(cap);
        auto read42 = w.app(w.app(ReadRef, Nat), {phantom, ref, cap});
        print_value_type(read42);
    }
    std::cout << "--- Affine Fractional Capabilities for Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi({Star, Star}, Star), {"FRef"});
        // auto Perm = w.pi_lambda(w.pi(star, star), {"Perm"});
        auto Write = w.sigma_type(0, {"Wr"});
        auto Read = w.axiom(w.pi(Star, Star), {"Rd"});
        auto Cap = w.axiom(w.pi({Star, Star}, w.star(A)), {"FCap"});
        // Recursive type for
        auto C = [&](int i){ return w.var(Star, i, {"C"}); };
        auto F = [&](int i){ return w.var(Star, i, {"F"}); };
        auto new_ref_ret = w.sigma({Star, w.app(Ref, {T(2), C(0)}), w.app(Cap, {C(1), Write})});
        auto NewRef = w.axiom(w.pi(Star, w.pi(T(0), new_ref_ret), {"NewFRef"}));
        print_value_type(NewRef);
        // ReadRef : Π(T:*).Π(C:*, F:*, FRef[T, C], ᴬFCap[C, F]).ᴬΣ(T, ᴬFCap[C, F])
        auto ReadRef = w.axiom(w.pi(Star,
                                    w.pi({Star, Star, w.app(Ref, {T(2), C(1)}), w.app(Cap, {C(1), F(0)})},
                                         w.sigma({T(4), w.app(Cap, {C(4), F(3)})}))), {"ReadFRef"});
        print_value_type(ReadRef);
        // WriteRef : Π(T:*).Π(C:*, F:*, FRef[T, C], ᴬFCap[C, F], T).ᴬACap[C, F]
        auto WriteRef = w.axiom(w.pi(Star,
                                     w.pi({Star, Star, w.app(Ref, {T(2), C(1)}), w.app(Cap, {C(2), F(1)}),
                                                 T(4)},
                                         w.app(Cap, {C(4), F(3)})), {"WriteFRef"}));
        print_value_type(WriteRef);
        // // FreeRef : Π(T:*, C:*, CRef[T, C], ᴬACap[C]).()
        // auto FreeRef = w.axiom(w.pi({Star, Star, w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1))},
        //                              w.unit()), {"FreeFRef"});
        // TODO
        auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
        auto phantom = w.extract(ref42, 0);
        print_value_type(ref42);
        auto ref = w.extract(ref42, 1, {"ref"});
        print_value_type(ref);
        auto cap = w.extract(ref42, 2, {"cap"});
        print_value_type(cap);
        auto read42 = w.app(w.app(ReadRef, Nat), {phantom, Write, ref, cap});
        print_value_type(read42);
    }
    std::cout << "--- QualifierTest end ---" << endl;
}
