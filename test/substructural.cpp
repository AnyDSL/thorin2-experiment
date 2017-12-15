#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << "<" << x->gid() << "> " << (x->name() == "" ? #x : x->name()) << " = " << x << ": " << x->type() << endl; }while(0)

TEST(Qualifiers, Lattice) {
    WorldBase w;
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
    WorldBase w;
    auto u = w.unlimited();
    auto r = w.relevant();
    auto a = w.affine();
    auto l = w.linear();
    auto lub = [&](Defs defs) { return w.variant(w.qualifier_type(), defs); };

    ASSERT_TRUE(u->is_value());
    ASSERT_TRUE(r->is_value());
    ASSERT_TRUE(a->is_value());
    ASSERT_TRUE(l->is_value());

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

    auto v = w.var(w.qualifier_type(), 0);
    ASSERT_EQ(u, v->qualifier());
    ASSERT_EQ(v, lub({v}));
    ASSERT_EQ(v, lub({u, v, u}));
    ASSERT_EQ(l, lub({v, l}));
    ASSERT_EQ(l, lub({r, v, a}));

}

TEST(Qualifiers, Kinds) {
    WorldBase w;
    auto u = w.unlimited();
    auto r = w.relevant();
    auto a = w.affine();
    auto l = w.linear();
    auto v = w.var(w.qualifier_type(), 0);
    auto lub = [&](Defs defs) { return w.variant(w.qualifier_type(), defs); };

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
    WorldBase w;
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
    WorldBase w;
    Env env;
    auto Star = w.star();
    auto Nat = w.axiom(Star, {"Nat"});
    env["Nat"] = Nat;
    auto n42 = w.assume(Nat, 42);
    env["n42"] = n42;

    auto Ref = w.axiom(w.pi(Star, Star), {"Ref"});
    env["Ref"] = Ref;
    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. Ref(T)", env), {"NewRef"});
    env["NewRef"] = NewRef;
    auto ReadRef = w.axiom(parse(w, "ΠT: *. ΠRef(T). T", env), {"ReadRef"});
    env["ReadRef"] = ReadRef;
    auto WriteRef = w.axiom(parse(w, "ΠT: *. Π[Ref(T), T]. []", env), {"WriteRef"});
    env["WriteRef"] = WriteRef;
    auto FreeRef = w.axiom(parse(w, "ΠT: *. ΠRef(T). []", env), {"FreeRef"});
    env["FreeRef"] = FreeRef;
    auto ref42 = parse(w, "(NewRef(Nat))(n42)", env);
    EXPECT_EQ(ref42->type(), parse(w, "Ref(Nat)", env));
    auto read42 = w.app(w.app(ReadRef, Nat), ref42);
    EXPECT_EQ(read42->type(), Nat);
    // TODO tests for write/free
}

TEST(Substructural, AffineRefs) {
    WorldBase w;
    Env env;
    auto a = w.affine();
    auto Star = w.star();

    auto Ref = w.axiom(w.pi(Star, w.star(a)), {"ARef"});
    env["ARef"] = Ref;

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. ARef(T)", env), {"NewARef"});
    env["NewARef"] = NewRef;
    auto ReadRef = w.axiom(parse(w, "ΠT: *. ΠARef(T). [T, ARef(T)]", env), {"ReadARef"});
    env["ReadARef"] = ReadRef;
    auto WriteRef = w.axiom(parse(w, "ΠT: *. Π[ARef(T), T]. ARef(T)", env), {"WriteARef"});
    env["WriteARef"] = WriteRef;
    auto FreeRef = w.axiom(parse(w, "ΠT: *. ΠARef(T). []", env), {"FreeARef"});
    env["FreeARef"] = FreeRef;

    // TODO example use with reductions, etc
}

TEST(Substructural, AffineCapabilityRefs) {
    WorldBase w;
    Env env;
    auto a = w.affine();
    auto Star = w.star();
    auto Nat = w.axiom(Star, {"Nat"});
    auto n42 = w.assume(Nat, 42);

    auto Ref = w.axiom(w.pi(w.sigma({Star, Star}), w.star(a)), {"CRef"});
    env["CRef"] = Ref;
    auto Cap = w.axiom(w.pi(Star, w.star(a)), {"ACap"});
    env["ACap"] = Cap;

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. [C:*, CRef(T, C), ACap(C)]", env), {"NewCRef"});
    env["NewCRef"] = NewRef;
    auto ReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, CRef(T, C), ACap(C)]. [T, ACap(C)]", env), {"ReadCRef"});
    env["ReadCRef"] = ReadRef;
    auto AliasReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, CRef(T, C)]. T", env), {"AliasReadCRef"});
    env["AliasReadCRef"] = AliasReadRef;
    auto WriteRef = w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C), ACap(C), T]. ACap(C)", env), {"WriteCRef"});
    env["WriteCRef"] = WriteRef;
    auto FreeRef = w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C), ACap(C)]. []", env), {"FreeCRef"});
    env["FreeCRef"] = FreeRef;

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
    WorldBase w;
    Env env;
    auto a = w.affine();
    auto Star = w.star();
    auto Nat = w.axiom(Star, {"Nat"});
    auto n42 = w.assume(Nat, 42);
    auto n0 = w.assume(Nat, 0);

    auto Ref = w.axiom(w.pi(w.sigma({Star, Star}), Star), {"FRef"});
    env["FRef"] = Ref;
    auto Write = w.sigma_type(0, {"Wr"});
    env["Wr"] = Write;
    // TODO Replace Star by a more precise kind allowing only Wr/Rd
    auto Read = w.axiom(w.pi(Star, Star), {"Rd"});
    env["Rd"] = Read;
    auto Cap = w.axiom(w.pi(w.sigma({Star, Star}), w.star(a)), {"FCap"});
    env["FCap"] = Cap;

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. [C:*, FRef(T, C), FCap(C, Wr)]", env), {"NewFRef"});
    env["NewFRef"] = NewRef;
    auto ReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, F:*, FRef(T, C), FCap(C, F)]. [T, FCap(C, F)]", env), {"ReadFRef"});
    env["ReadFRef"] = ReadRef;
    print_value_type(ReadRef);
    auto WriteRef = w.axiom(parse(w, "ΠT: *. Π[C: *, FRef(T, C), FCap(C, Wr), T]. FCap(C, Wr)", env), {"WriteFRef"});
    env["WriteFRef"] = WriteRef;
    print_value_type(WriteRef);
    auto FreeRef = w.axiom(parse(w, "ΠT: *. Π[C: *, FRef(T, C), FCap(C, Wr)]. []", env), {"FreeFRef"});
    env["FreeFRef"] = FreeRef;
    print_value_type(FreeRef);
    auto SplitFCap = w.axiom(parse(w, "Π[C:*, F:*, FCap(C, F)]. [FCap(C, Rd(F)), FCap(C, Rd(F))]", env), {"SplitFCap"});
    env["SplitFCap"] = SplitFCap;
    auto JoinFCap = w.axiom(parse(w, "Π[C:*, F:*, FCap(C, Rd(F)), FCap(C, Rd(F))]. FCap(C, F)", env), {"JoinFCap"});
    env["JoinFCap"] = JoinFCap;

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
