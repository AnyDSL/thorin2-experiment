#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;

#define printValue(x) do{ printUtf8(x->name() == "" ? #x : x->name()); printUtf8(" = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(x->name() == "" ? #x : x->name()); printUtf8(": "); x->type()->dump(); }while(0)
#define printValueType(x) do{ printUtf8(x->name() == "" ? #x : x->name()); printUtf8(" = "); printUtf8(x->to_string()); printUtf8(": "); x->type()->dump(); }while(0)

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
    cout << poly_aid << " : " << poly_aid->type() << endl;

    // λx:ᴬNat.x
    auto anid2 = w.app(poly_aid, ANat);
    cout << anid2 << " : " << anid2->type() << endl;

    auto T = [&](int i){ return w.var(w.star(), i, {"T"}); };
    cout << "--- Unrestricted Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi(w.star(), w.star()), {"Ref"});
        printType(Ref);
        auto app_Ref_T0 = w.app(Ref, T(0));
        auto NewRef = w.axiom(w.pi({w.star(), T(0)}, w.app(Ref, T(1))), {"NewRef"});
        printType(NewRef);
        auto ReadRef = w.axiom(w.pi({w.star(), w.app(Ref, T(0))}, T(1)), {"ReadRef"});
        printType(ReadRef);
        auto WriteRef = w.axiom(w.pi({w.star(), app_Ref_T0, T(0)}, w.unit()), {"WriteRef"});
        printType(WriteRef);
        auto FreeRef = w.axiom(w.pi({w.star(), app_Ref_T0}, w.unit()), {"FreeRef"});
        printType(FreeRef);
    }
    cout << "--- Affine Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi(w.star(), w.star(A)), {"ARef"});
        printType(Ref);
        auto app_Ref_T0 = w.app(Ref, T(0));
        assert(app_Ref_T0->qualifier() == A);
        auto NewRef = w.axiom(w.pi({w.star(), T(0)}, w.app(Ref, T(1))), {"NewARef"});
        printType(NewRef);
        // ReadRef : Π(*).Π(ARef[<0:*>]).Σ(<1:*>, ARef[<2:*>])
        auto ReadRef = w.axiom(w.pi({w.star(), app_Ref_T0}, w.sigma({T(1), w.app(Ref, T(2))})),
                {"ReadARef"});
        printType(ReadRef);
        auto WriteRef = w.axiom(w.pi({w.star(), app_Ref_T0, T(0)}, w.unit()), {"WriteARef"});
        printType(WriteRef);
        auto FreeRef = w.axiom(w.pi({w.star(), app_Ref_T0}, w.unit()), {"FreeARef"});
        printType(FreeRef);
    }
    cout << "--- Affine Capabilities for Refs ---" << endl;
    {
        auto Ref = w.axiom(w.pi({w.star(), w.star()}, w.star()), {"CRef"});
        auto Cap = w.axiom(w.pi(w.star(), w.star(A)), {"ACap"});
        printType(Ref);
        printType(Cap);
        auto C = [&](int i){ return w.var(w.star(), i, {"C"}); };
        auto NewRef = w.axiom(w.pi(w.star(), w.pi(T(0),
                                    w.sigma({w.star(),
                                             w.app(Ref, {T(2), C(0)}),
                                             w.app(Cap, C(1))}))),
                {"NewCRef"});
        printType(NewRef);
        // ReadRef : Π(T:*, C:*, CRef[T, C], ᴬACap[C]).ᴬΣ(T, ᴬACap[C])
        auto ReadRef = w.axiom(w.pi(w.star(), w.pi({w.star(), w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1))},
                                                    w.sigma({T(3), w.app(Cap, C(3))}))), {"ReadCRef"});
        printType(ReadRef);
        // AliasReadRef : Π(T:*, C:*, CRef[T, C]).T
        auto AliasReadRef = w.axiom(w.pi({w.star(), w.star(), w.app(Ref, {T(1), C(0)})}, T(3)),
                {"AliasReadCRef"});
        printType(AliasReadRef);
        // WriteRef : Π(T:*, C:*, CRef[T, C], ᴬACap[C], T).ᴬACap[C]
        auto WriteRef = w.axiom(w.pi({w.star(), w.star(), w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1)), T(3)},
                                      w.app(Cap, C(3))), {"WriteCRef"});
        printType(WriteRef);
        // FreeRef : Π(T:*, C:*, CRef[T, C], ᴬACap[C]).()
        auto FreeRef = w.axiom(w.pi({w.star(), w.star(), w.app(Ref, {T(1), C(0)}), w.app(Cap, C(1))},
                                     w.unit()), {"FreeCRef"});
        printType(FreeRef);
        auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
        auto phantom = w.extract(ref42, 0);
        printValueType(ref42);
        auto ref = w.extract(ref42, 1, {"ref"});
        printValueType(ref);
        auto cap = w.extract(ref42, 2, {"cap"});
        printValueType(cap);
        auto read42 = w.app(w.app(ReadRef, Nat), {phantom, ref, cap});
        printValueType(read42);
    }
    cout << "--- Affine Fractional Capabilities for Refs ---" << endl;
    {
        // Recursive type for
        // TODO
    }
    cout << "--- QualifierTest end ---" << endl;
}
