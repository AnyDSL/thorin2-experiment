#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << "<" << x->gid() << "> " << (x->name() == "" ? #x : x->name()) << " = " << x << ": " << x->type() << endl; }while(0)

TEST(Qualifiers, Lattice) {
    World w;
    auto U = Qualifier::Unlimited;
    auto R = Qualifier::Relevant;
    auto A = Qualifier::Affine;
    auto L = Qualifier::Linear;

    ASSERT_LT(U, A);
    ASSERT_LT(U, R);
    ASSERT_LT(U, L);
    ASSERT_LT(A, L);
    ASSERT_LT(R, L);

    ASSERT_EQ(join(U, U), U);
    ASSERT_EQ(join(A, U), A);
    ASSERT_EQ(join(R, U), R);
    ASSERT_EQ(join(L, U), L);
    ASSERT_EQ(join(A, A), A);
    ASSERT_EQ(join(A, R), L);
    ASSERT_EQ(join(L, A), L);
    ASSERT_EQ(join(L, R), L);

    ASSERT_EQ(w.qualifier(U)->box().get_qualifier(), U);
    ASSERT_EQ(w.qualifier(R)->box().get_qualifier(), R);
    ASSERT_EQ(w.qualifier(A)->box().get_qualifier(), A);
    ASSERT_EQ(w.qualifier(L)->box().get_qualifier(), L);
}

TEST(Qualifiers, Variants) {
    World w;
    auto u = w.unlimited();
    auto r = w.relevant();
    auto a = w.affine();
    auto l = w.linear();
    auto lub = [&](Defs defs) { return w.variant(defs, w.qualifier_kind()); };

    ASSERT_EQ(u, u->qualifier());
    ASSERT_EQ(u, r->qualifier());
    ASSERT_EQ(u, a->qualifier());
    ASSERT_EQ(u, l->qualifier());

    ASSERT_EQ(u, lub({u}));
    ASSERT_EQ(r, lub({r}));
    ASSERT_EQ(a, lub({a}));
    ASSERT_EQ(l, lub({l}));
    ASSERT_EQ(u, lub({u, u, u}));
    ASSERT_EQ(r, lub({u, r}));
    ASSERT_EQ(a, lub({a, u}));
    ASSERT_EQ(l, lub({a, l}));
    ASSERT_EQ(l, lub({a, r}));
    ASSERT_EQ(l, lub({u, l, r, r}));

    auto v = w.var(w.qualifier_kind(), 0);
    ASSERT_EQ(u, v->qualifier());
    ASSERT_EQ(v, lub({v}));
    ASSERT_EQ(v, lub({u, v, u}));
    ASSERT_EQ(l, lub({v, l}));
    ASSERT_EQ(l, lub({r, v, a}));

}

TEST(Qualifiers, Kinds) {
    World w;
    auto u = w.unlimited();
    auto r = w.relevant();
    auto a = w.affine();
    auto l = w.linear();
    auto v = w.var(w.qualifier_kind(), 0);
    auto lub = [&](Defs defs) { return w.variant(defs, w.qualifier_kind()); };

    auto anat = w.axiom(w.star(a), {"nat"});
    auto rnat = w.axiom(w.star(r), {"nat"});
    auto vtype = w.assume(w.star(v), {0}, {"nat"});
    ASSERT_EQ(w.sigma({anat, w.star()})->qualifier(), a);
    ASSERT_EQ(w.sigma({anat, rnat})->qualifier(), l);
    ASSERT_EQ(w.sigma({vtype, rnat})->qualifier(), lub({v, r}));
    ASSERT_EQ(w.sigma({anat, w.star(l)})->qualifier(), a);

    ASSERT_EQ(a, w.variant({w.star(u), w.star(a)})->qualifier());
    ASSERT_EQ(l, w.variant({w.star(r), w.star(l)})->qualifier());
}

#if 0
TEST(Substructural, Misc) {
    World w;
    //auto R = Qualifier::Relevant;
    auto A = Qualifier::Affine;
    auto L = Qualifier::Linear;
    auto a = w.affine();
    //auto l = w.linear();
    //auto r = w.relevant();
    //auto Star = w.star();
    auto Unit = w.unit();
    auto Nat = w.type_sw64();
    //auto n42 = w.axiom(Nat, {"42"});
    auto ANat = w.type_asw64();
    //auto LNat = w.type_nat(l);
    auto RNat = w.type_rsw64();
    auto an0 = w.val_asw64(0);
    ASSERT_NE(an0, w.val_asw64(0));
    auto l_a0 = w.lambda(Unit, w.val_asw64(0), {"l_a0"});
    auto l_a0_app = w.app(l_a0);
    ASSERT_NE(l_a0_app, w.app(l_a0));
    auto anx = w.var(ANat, 0, {"x"});
    auto anid = w.lambda(ANat, anx, {"anid"});
    w.app(anid, an0);
    // We need to check substructural types later, so building a second app is possible:
    ASSERT_FALSE(is_error(w.app(anid, an0)));

    auto tuple_type = w.sigma({ANat, RNat});
    ASSERT_EQ(tuple_type->qualifier(), w.qualifier(L));
    auto an1 = w.axiom(ANat, {"1"});
    auto rn0 = w.axiom(RNat, {"0"});
    auto tuple = w.tuple({an1, rn0});
    ASSERT_EQ(tuple->type(), tuple_type);
    auto tuple_app0 = w.extract(tuple, 0_s);
    ASSERT_EQ(w.extract(tuple, 0_s), tuple_app0);

    auto a_id_type = w.pi(Nat, Nat, a);
    auto nx = w.var(Nat, 0, {"x"});
    auto a_id = w.lambda(Nat, nx, a, {"a_id"});
    ASSERT_EQ(a_id_type, a_id->type());
    auto n0 = w.axiom(Nat, {"0"});
    //auto a_id_app = w.app(a_id, n0);
    ASSERT_FALSE(is_error(w.app(a_id, n0)));

    // λᴬT:*.λx:ᴬT.x
    auto aT1 = w.var(w.star(A), 0, {"T"});
    auto aT2 = w.var(w.star(A), 1, {"T"});
    auto x = w.var(aT2, 0, {"x"});
    auto poly_aid = w.lambda(aT2->type(), w.lambda(aT1, x));
    std::cout << poly_aid << " : " << poly_aid->type() << endl;

    // λx:ᴬNat.x
    auto anid2 = w.app(poly_aid, ANat);
    std::cout << anid2 << " : " << anid2->type() << endl;
}
#endif

TEST(Substructural, UnlimitedRefs) {
    World w;
    auto Star = w.star();
    auto Unit = w.unit();
    //auto Nat = w.type_nat();
    //auto n42 = w.val_nat(42);
    //auto n0 = w.val_nat_0();
    auto T = [&](int i){ return w.var(Star, i, {"T"}); };

    auto Ref = w.axiom(w.pi(Star, Star), {"Ref"});
    print_value_type(Ref);
    auto app_Ref_T0 = w.app(Ref, T(0));
    auto NewRef = w.axiom(w.pi({Star, T(0)}, w.app(Ref, T(1))), {"NewRef"});
    print_value_type(NewRef);
    auto ReadRef = w.axiom(w.pi({Star, w.app(Ref, T(0))}, T(1)), {"ReadRef"});
    print_value_type(ReadRef);
    auto WriteRef = w.axiom(w.pi({Star, app_Ref_T0, T(0)}, Unit), {"WriteRef"});
    print_value_type(WriteRef);
    auto FreeRef = w.axiom(w.pi({Star, app_Ref_T0}, Unit), {"FreeRef"});
    print_value_type(FreeRef);
    // TODO example use with reductions, etc
}

TEST(Substructural, AffineRefs) {
    World w;
    auto a = w.affine();
    auto Star = w.star();
    auto Unit = w.unit();
    //auto Nat = w.type_nat();
    //auto n42 = w.val_nat(42);
    //auto n0 = w.val_nat_0();
    //auto ANat = w.type_nat(a);
    auto T = [&](int i){ return w.var(Star, i, {"T"}); };

    auto Ref = w.axiom(w.pi(Star, w.star(a)), {"ARef"});
    print_value_type(Ref);
    auto app_Ref_T0 = w.app(Ref, T(0));
    ASSERT_EQ(app_Ref_T0->qualifier(), a);
    auto NewRef = w.axiom(w.pi({Star, T(0)}, w.app(Ref, T(1))), {"NewARef"});
    print_value_type(NewRef);
    // ReadRef : Π(*).Π(ARef[<0:*>]).Σ(<1:*>, ARef[<2:*>])
    auto ReadRef = w.axiom(w.pi({Star, app_Ref_T0}, w.sigma({T(1), w.app(Ref, T(2))})), {"ReadARef"});
    print_value_type(ReadRef);
    auto WriteRef = w.axiom(w.pi({Star, app_Ref_T0, T(0)}, Unit), {"WriteARef"});
    print_value_type(WriteRef);
    auto FreeRef = w.axiom(w.pi({Star, app_Ref_T0}, Unit), {"FreeARef"});
    print_value_type(FreeRef);
    // TODO example use with reductions, etc
}

TEST(Substructural, AffineCapabilityRefs) {
    World w;
    auto a = w.affine();
    auto Star = w.star();
    auto Unit = w.unit();
    auto Nat = w.type_nat();
    auto n42 = w.val_nat(42);
    //auto n0 = w.val_nat_0();
    //auto ANat = w.type_nat(a);
    auto T = [&](int i){ return w.var(Star, i, {"T"}); };

    auto Ref = w.axiom(w.pi({Star, Star}, Star), {"CRef"});
    auto Cap = w.axiom(w.pi(Star, w.star(a)), {"ACap"});
    print_value_type(Ref);
    print_value_type(Cap);
    auto C = [&](int i){ return w.var(Star, i, {"C"}); };
    auto NewRef = w.axiom(w.pi(Star, w.pi(T(0), w.sigma({Star, w.app(Ref, {T(2), C(0)}), w.app(Cap, C(1))}))),
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
                                  Unit), {"FreeCRef"});
    print_value_type(FreeRef);
    auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
    auto phantom = w.extract(ref42, 0_s);
    print_value_type(ref42);
    auto ref = w.extract(ref42, 1, {"ref"});
    print_value_type(ref);
    auto cap = w.extract(ref42, 2, {"cap"});
    print_value_type(cap);
    auto read42 = w.app(w.app(ReadRef, Nat), {phantom, ref, cap});
    print_value_type(read42);
    // TODO asserts to typecheck correct and incorrect usage
}

TEST(Substructural, AffineFractionalCapabilityRefs) {
    World w;
    auto a = w.affine();
    auto Star = w.star();
    auto Unit = w.unit();
    auto Nat = w.type_nat();
    auto n42 = w.val_nat(42);
    auto n0 = w.val_nat_0();
    //auto ANat = w.type_nat(a);
    auto T = [&](int i){ return w.var(Star, i, {"T"}); };

    auto Ref = w.axiom(w.pi({Star, Star}, Star), {"FRef"});
    // TODO Replace Star below by a more precise kind allowing only Wr/Rd
    auto Write = w.sigma_type(0, {"Wr"});
    auto Read = w.axiom(w.pi(Star, Star), {"Rd"});
    print_value_type(Write);
    print_value_type(Read);
    auto Cap = w.axiom(w.pi({Star, Star}, w.star(a)), {"FCap"});
    print_value_type(Cap);
    auto C = [&](int i){ return w.var(Star, i, {"C"}); };
    auto F = [&](int i){ return w.var(Star, i, {"F"}); };
    auto new_ref_ret = w.sigma({Star, w.app(Ref, {T(2), C(0)}), w.app(Cap, {C(1), Write})});
    auto NewRef = w.axiom(w.pi(Star, w.pi(T(0), new_ref_ret), {"NewFRef"}));
    print_value_type(NewRef);
    // ReadRef : Π(T:*).Π(C:*, F:*, FRef[T, C], ᴬFCap[C, F]).ᴬΣ(T, ᴬFCap[C, F])
    auto ReadRef = w.axiom(w.pi(Star,
                                w.pi({Star, Star, w.app(Ref, {T(2), C(1)}), w.app(Cap, {C(2), F(1)})},
                                     w.sigma({T(4), w.app(Cap, {C(4), F(3)})}))), {"ReadFRef"});
    print_value_type(ReadRef);
    // WriteRef : Π(T:*).Π(C:*, FRef[T, C], ᴬFCap[C, Wr], T).ᴬFCap[C, Wr]
    auto WriteRef = w.axiom(w.pi(Star,
                                 w.pi({Star, w.app(Ref, {T(1), C(0)}), w.app(Cap, {C(1), Write}), T(3)},
                                     w.app(Cap, {C(3), Write})), {"WriteFRef"}));
    print_value_type(WriteRef);
    // // FreeRef : Π(T:*).Π(C:*, FRef[T, C], ᴬFCap[C, Wr]).()
    auto FreeRef = w.axiom(w.pi(Star, w.pi({Star, w.app(Ref, {T(1), C(0)}), w.app(Cap, {C(1), Write})},
                                           Unit)), {"FreeFRef"});
    print_value_type(FreeRef);
    // SplitFCap : Π(C:*, F:*, ᴬFCap[C, F]).Σ(ᴬFCap[C, Rd(F)], ᴬFCap[C, Rd(F)])
    auto SplitFCap = w.axiom(w.pi({Star, Star, w.app(Cap, {C(1), F(0)})},
                                  w.sigma({w.app(Cap, {C(2), w.app(Read, F(1))}),
                                  w.app(Cap, {C(3), w.app(Read, F(2))})})));
    print_value_type(SplitFCap);
    // JoinFCap : Π(C:*, F:*, ᴬFCap[C, Rd(F)], ᴬFCap[C, Rd(F)]).ᴬFCap[C, F]
    auto JoinFCap = w.axiom(w.pi({Star, Star, w.app(Cap, {C(1), w.app(Read, F(0))}),
                                  w.app(Cap, {C(2), w.app(Read, F(1))})},
                                 w.app(Cap, {C(3), F(2)})));
    print_value_type(JoinFCap);
    auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
    auto phantom = w.extract(ref42, 0_s);
    print_value_type(ref42);
    auto ref = w.extract(ref42, 1, {"ref"});
    print_value_type(ref);
    auto cap = w.extract(ref42, 2);
    print_value_type(cap);
    auto read42 = w.app(w.app(ReadRef, Nat), {phantom, Write, ref, cap});
    print_value_type(read42);
    auto read_cap = w.extract(read42, 1);
    auto write0 = w.app(w.app(WriteRef, Nat), {phantom, ref, read_cap, n0});
    print_value_type(write0);
    auto split = w.app(SplitFCap, {phantom, Write, write0});
    print_value_type(split);
    auto read0 = w.app(w.app(ReadRef, Nat), {phantom, w.app(Read, Write), ref, w.extract(split, 0_s)});
    print_value_type(read0);
    auto join = w.app(JoinFCap, {phantom, Write, w.extract(split, 1), w.extract(read0, 1)});
    print_value_type(join);
    auto free = w.app(w.app(FreeRef, Nat), {phantom, ref, join});
    print_value_type(free);
    // TODO asserts to typecheck correct and incorrect usage
}
