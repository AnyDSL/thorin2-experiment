#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/fe/parser.h"

using namespace thorin;
using namespace thorin::fe;

void print_value_type(const Def* def) {
    outln("<{}> {} = {}: {}", def->gid(), def->name(), def, def->type());
}

TEST(Qualifiers, Lattice) {
    World w;
    auto U = QualifierTag::Unlimited;
    auto R = QualifierTag::Relevant;
    auto A = QualifierTag::Affine;
    auto L = QualifierTag::Linear;

    EXPECT_LT(U, A);
    EXPECT_LT(U, R);
    EXPECT_LT(U, L);
    EXPECT_LT(A, L);
    EXPECT_LT(R, L);

    EXPECT_EQ(lub(U, U), U);
    EXPECT_EQ(lub(A, U), A);
    EXPECT_EQ(lub(R, U), R);
    EXPECT_EQ(lub(L, U), L);
    EXPECT_EQ(lub(A, A), A);
    EXPECT_EQ(lub(A, R), L);
    EXPECT_EQ(lub(L, A), L);
    EXPECT_EQ(lub(L, R), L);

    EXPECT_EQ(w.qualifier(U)->qualifier_tag(), U);
    EXPECT_EQ(w.qualifier(R)->qualifier_tag(), R);
    EXPECT_EQ(w.qualifier(A)->qualifier_tag(), A);
    EXPECT_EQ(w.qualifier(L)->qualifier_tag(), L);
}

TEST(Qualifiers, Variants) {
    World w;
    auto u = w.unlimited();
    auto r = w.relevant();
    auto a = w.affine();
    auto l = w.linear();
    auto lub = [&](Defs defs) { return w.variant(w.qualifier_type(), defs); };

    EXPECT_EQ(u, u->qualifier());
    EXPECT_EQ(u, r->qualifier());
    EXPECT_EQ(u, a->qualifier());
    EXPECT_EQ(u, l->qualifier());

    EXPECT_EQ(u, lub({u}));
    EXPECT_EQ(r, lub({r}));
    EXPECT_EQ(a, lub({a}));
    EXPECT_EQ(l, lub({l}));
    EXPECT_EQ(u, lub({u, u, u}));
    EXPECT_EQ(r, lub({u, r}));
    EXPECT_EQ(a, lub({a, u}));
    EXPECT_EQ(l, lub({a, l}));
    EXPECT_EQ(l, lub({a, r}));
    EXPECT_EQ(l, lub({u, l, r, r}));

    auto v = w.var(w.qualifier_type(), 0);
    EXPECT_EQ(u, v->qualifier());
    EXPECT_EQ(v, lub({v}));
    EXPECT_EQ(v, lub({u, v, u}));
    EXPECT_EQ(l, lub({v, l}));
    EXPECT_EQ(l, lub({r, v, a}));
}

TEST(Qualifiers, Kinds) {
    World w;
    auto u = w.unlimited();
    auto r = w.relevant();
    auto a = w.affine();
    auto l = w.linear();
    auto v = w.var(w.qualifier_type(), 0);
    EXPECT_TRUE(w.qualifier_type()->has_values());
    EXPECT_TRUE(w.qualifier_type()->is_kind());
    EXPECT_EQ(u->type(), w.qualifier_type());
    EXPECT_TRUE(u->is_type());
    EXPECT_TRUE(u->is_value());
    EXPECT_FALSE(u->has_values());
    EXPECT_TRUE(r->is_value());
    EXPECT_TRUE(a->is_value());
    EXPECT_TRUE(l->is_value());
    auto lub = [&](Defs defs) { return w.variant(w.qualifier_type(), defs); };

    auto anat = w.axiom(w.star(a), {"anat"});
    auto rnat = w.axiom(w.star(r), {"rnat"});
    auto vtype = w.lit(w.star(v), {0}, {"rnat"});
    EXPECT_EQ(w.sigma({anat, w.star()})->qualifier(), a);
    EXPECT_EQ(w.sigma({anat, rnat})->qualifier(), l);
    EXPECT_EQ(w.sigma({vtype, rnat})->qualifier(), lub({v, r}));
    EXPECT_EQ(w.sigma({anat, w.star(l)})->qualifier(), a);

    EXPECT_EQ(a, w.variant({w.star(u), w.star(a)})->qualifier());
    EXPECT_EQ(l, w.variant({w.star(r), w.star(l)})->qualifier());
}

TEST(Substructural, TypeCheckLambda) {
    World w;
    EXPECT_THROW(parse(w, "λq:ℚ. λt:*q. λa:t. (a, a, ())")->check(), TypeError);

    auto a = w.affine();
    w.axiom(w.star(a), {"atype"});
    EXPECT_THROW(parse(w, "λa:atype. (a, a, ())")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "λa:atype. λi:3ₐ. (a, a, ())#i")->check());

    w.axiom(w.star(w.relevant()), {"rtype"});
    EXPECT_THROW(parse(w, "λr:rtype. ()")->check(), TypeError);
    EXPECT_THROW(parse(w, "λr:rtype. (42ₐ, 12ₐ)")->check(), TypeError);

    w.axiom(w.star(w.linear()), {"ltype"});
    EXPECT_THROW(parse(w, "λl:ltype. ()")->check(), TypeError);
    EXPECT_THROW(parse(w, "λl:ltype. (42ₐ, 12ₐ)")->check(), TypeError);
    EXPECT_THROW(parse(w, "λl:ltype. (l, 42ₐ, l)")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "λl:ltype. λi:3ₐ. (l, (), l)#i")->check());
}

TEST(Substructural, TypeCheckPack) {
    World w;
    w.axiom(w.arity_kind(w.affine()), {"aarity"});
    EXPECT_THROW(parse(w, "(i:aarity; (i, (), i))")->check(), TypeError);
    EXPECT_THROW(parse(w, "λa:𝔸ᴬ.(i:a; (i, i, ()))")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "λi:3ₐ. λa:𝔸ᴬ.(j:a; (j, j, ())#i)")->check());

    w.axiom(w.arity_kind(w.relevant()), {"rarity"});
    EXPECT_THROW(parse(w, "(r:rarity; ())")->check(), TypeError);
    EXPECT_THROW(parse(w, "(r:rarity; (42ₐ, 12ₐ))")->check(), TypeError);

    w.axiom(w.arity_kind(w.linear()), {"larity"});
    EXPECT_THROW(parse(w, "(l:larity; ())")->check(), TypeError);
    EXPECT_THROW(parse(w, "(l:larity; (42ₐ, 12ₐ))")->check(), TypeError);
    EXPECT_THROW(parse(w, "(l:larity; (l, 42ₐ, l))")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "(l:larity; λi:3ₐ. (l, (), l)#i)")->check());
}

TEST(Substructural, TypeCheckSigma) {
    World w;

    EXPECT_THROW(parse(w, "[i:2ₐᴿ, (42ₐ, 12ₐ)#i]")->check(), TypeError);
    EXPECT_THROW(parse(w, "[i:2ₐᴬ, (42ₐ, 12ₐ)#i]")->check(), TypeError);
    EXPECT_THROW(parse(w, "[i:2ₐᴸ, (42ₐ, 12ₐ)#i]")->check(), TypeError);
}

TEST(Substructural, TypeCheckPi) {
    World w;

    EXPECT_THROW(parse(w, "ΠT:*ᴿ. ΠT. *"), TypeError);
    EXPECT_THROW(parse(w, "ΠT:*ᴬ. ΠT. *"), TypeError);
    EXPECT_THROW(parse(w, "ΠT:*ᴸ. ΠT. *"), TypeError);
}

#if 0
TEST(Substructural, Misc) {
    World w;
    //auto R = QualifierTag::Relevant;
    auto A = QualifierTag::Affine;
    auto L = QualifierTag::Linear;
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
    EXPECT_NE(an0, w.val_asw64(0));
    auto l_a0 = w.lambda(Unit, w.val_asw64(0), {"l_a0"});
    auto l_a0_app = w.app(l_a0);
    EXPECT_NE(l_a0_app, w.app(l_a0));
    auto anx = w.var(ANat, 0, {"x"});
    auto anid = w.lambda(ANat, anx, {"anid"});
    w.app(anid, an0);
    // We need to check substructural types later, so building a second app is possible:
    EXPECT_FALSE(is_error(w.app(anid, an0)));

    auto tuple_type = w.sigma({ANat, RNat});
    EXPECT_EQ(tuple_type->qualifier(), w.qualifier(L));
    auto an1 = w.axiom(ANat, {"1"});
    auto rn0 = w.axiom(RNat, {"0"});
    auto tuple = w.tuple({an1, rn0});
    EXPECT_EQ(tuple->type(), tuple_type);
    auto tuple_app0 = w.extract(tuple, 0_s);
    EXPECT_EQ(w.extract(tuple, 0_s), tuple_app0);

    auto a_id_type = w.pi(Nat, Nat, a);
    auto nx = w.var(Nat, 0, {"x"});
    auto a_id = w.lambda(Nat, nx, a, {"a_id"});
    EXPECT_EQ(a_id_type, a_id->type());
    auto n0 = w.axiom(Nat, {"0"});
    //auto a_id_app = w.app(a_id, n0);
    EXPECT_FALSE(is_error(w.app(a_id, n0)));

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
    auto Nat = w.type_nat();

    w.axiom(w.pi(Star, Star), {"Ref"});
    w.axiom(parse(w, "ΠT: *. ΠT. Ref(T)"), {"NewRef"});
    auto ReadRef = w.axiom(parse(w, "ΠT: *. ΠRef(T). T"), {"ReadRef"});
    w.axiom(parse(w, "ΠT: *. Π[Ref(T), T]. []"), {"WriteRef"});
    w.axiom(parse(w, "ΠT: *. ΠRef(T). []"), {"FreeRef"});
    auto ref42 = parse(w, "(NewRef(nat))(42s64::nat)");
    EXPECT_EQ(ref42->type(), parse(w, "Ref(nat)"));
    auto read42 = w.app(w.app(ReadRef, Nat), ref42);
    EXPECT_EQ(read42->type(), Nat);
    // TODO tests for write/free
}

TEST(Substructural, AffineRefs) {
    World w;
    auto a = w.affine();
    auto Star = w.star();

    w.axiom(w.pi(Star, w.star(a)), {"ARef"});
    EXPECT_TRUE(parse(w, "ARef(\\0::*)")->has_values());
    w.axiom(parse(w, "ΠT: *. ΠT. ARef(T)"), {"NewARef"});
    w.axiom(parse(w, "ΠT: *. ΠARef(T). [T, ARef(T)]"), {"ReadARef"});
    w.axiom(parse(w, "ΠT: *. Π[ARef(T), T]. ARef(T)"), {"WriteARef"});
    w.axiom(parse(w, "ΠT: *. ΠARef(T). []"), {"FreeARef"});

    // TODO example use with reductions, etc
}

TEST(Substructural, AffineCapabilityRefs) {
    World w;
    auto a = w.affine();
    auto Star = w.star();
    auto Nat = w.type_nat();
    auto n42 = w.lit(Nat, 42);

    w.axiom(w.pi(w.sigma({Star, Star}), w.star(a)), {"CRef"});
    w.axiom(w.pi(Star, w.star(a)), {"ACap"});

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. [C:*, CRef(T, C), ACap(C)]"), {"NewCRef"});
    auto ReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, CRef(T, C), ACap(C)]. [T, ACap(C)]"), {"ReadCRef"});
    w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C)]. T"), {"AliasReadCRef"});
    w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C), ACap(C), T]. ACap(C)"), {"WriteCRef"});
    w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C), ACap(C)]. []"), {"FreeCRef"});

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
    auto Nat = w.type_nat();
    auto n42 = w.lit(Nat, 42);
    auto n0 = w.lit(Nat, 0);

    w.axiom(w.pi(w.sigma({Star, Star}), Star), {"FRef"});
    auto Write = w.axiom(Star, {"Write"});
    auto Read = w.axiom(Star, {"Read"});
    auto WriteOrRead = w.variant({Write, Read});
    auto Wr = w.axiom(Write, {"Wr"});
    auto Rd = w.axiom(w.pi(WriteOrRead, Read), {"Rd"});
    w.axiom(w.pi(w.sigma({Star, WriteOrRead}), w.star(a)), {"FCap"});

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. [C:*, FRef(T, C), FCap(C, Wr)]"), {"NewFRef"});
    auto ReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, F:{Write, Read}, FRef(T, C), FCap(C, F)]. [T, FCap(C, F)]"), {"ReadFRef"});
    print_value_type(ReadRef);
    auto WriteRef = w.axiom(parse(w, "ΠT: *. Π[C: *, FRef(T, C), FCap(C, Wr), T]. FCap(C, Wr)"), {"WriteFRef"});
    print_value_type(WriteRef);
    auto FreeRef = w.axiom(parse(w, "ΠT: *. Π[C: *, FRef(T, C), FCap(C, Wr)]. []"), {"FreeFRef"});
    print_value_type(FreeRef);
    auto SplitFCap = w.axiom(parse(w, "Π[C:*, F:{Write, Read}, FCap(C, F)]. [FCap(C, Rd(F)), FCap(C, Rd(F))]"), {"SplitFCap"});
    auto JoinFCap = w.axiom(parse(w, "Π[C:*, F:{Write, Read}, FCap(C, Rd(F)), FCap(C, Rd(F))]. FCap(C, F)"), {"JoinFCap"});

    auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
    auto phantom = w.extract(ref42, 0_s);
    print_value_type(ref42);
    auto ref = w.extract(ref42, 1, {"ref"});
    print_value_type(ref);
    auto cap = w.extract(ref42, 2, {"cap"});
    print_value_type(cap);
    auto read42 = w.app(w.app(ReadRef, Nat), {phantom, Wr, ref, cap}, {"read42"});
    print_value_type(read42);
    auto read_cap = w.extract(read42, 1);
    auto write0 = w.app(w.app(WriteRef, Nat), {phantom, ref, read_cap, n0}, {"write0"});
    print_value_type(write0);
    auto split = w.app(SplitFCap, {phantom, Wr, write0}, {"split"});
    print_value_type(split);
    auto read0 = w.app(w.app(ReadRef, Nat), {phantom, w.app(Rd, Wr), ref, w.extract(split, 0_s)}, {"read0"});
    print_value_type(read0);
    auto join = w.app(JoinFCap, {phantom, Wr, w.extract(split, 1), w.extract(read0, 1)}, {"join"});
    print_value_type(join);
    auto free = w.app(w.app(FreeRef, Nat), {phantom, ref, join}, {"free"});
    print_value_type(free);
    // TODO asserts to typecheck correct and incorrect usage
}
