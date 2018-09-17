#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/fe/parser.h"

using namespace thorin;
using namespace thorin::fe;

TEST(Qualifiers, Lattice) {
    World w;
    auto U = Qualifier::u;
    auto R = Qualifier::r;
    auto A = Qualifier::a;
    auto L = Qualifier::l;

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

    EXPECT_EQ(*get_qualifier(w.lit(U)), U);
    EXPECT_EQ(*get_qualifier(w.lit(R)), R);
    EXPECT_EQ(*get_qualifier(w.lit(A)), A);
    EXPECT_EQ(*get_qualifier(w.lit(L)), L);
}

TEST(Qualifiers, Variants) {
    World w;
    auto u = w.lit(Qualifier::u);
    auto r = w.lit(Qualifier::r);
    auto a = w.lit(Qualifier::a);
    auto l = w.lit(Qualifier::l);
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
    auto u = w.lit(Qualifier::u);
    auto r = w.lit(Qualifier::r);
    auto a = w.lit(Qualifier::a);
    auto l = w.lit(Qualifier::l);
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

    auto anat = w.axiom(w.kind_star(a), {"anat"});
    auto rnat = w.axiom(w.kind_star(r), {"rnat"});
    auto vtype = w.lit(w.kind_star(v), {0}, {"rnat"});
    EXPECT_EQ(w.sigma({anat, w.kind_star()})->qualifier(), a);
    EXPECT_EQ(w.sigma({anat, rnat})->qualifier(), l);
    EXPECT_EQ(w.sigma({vtype, rnat})->qualifier(), lub({v, r}));
    EXPECT_EQ(w.sigma({anat, w.kind_star(l)})->qualifier(), a);

    EXPECT_EQ(a, w.variant({w.kind_star(u), w.kind_star(a)})->qualifier());
    EXPECT_EQ(l, w.variant({w.kind_star(r), w.kind_star(l)})->qualifier());
}

TEST(Substructural, TypeCheckLambda) {
    World w;
    EXPECT_THROW(parse(w, "λq:ℚ. λt:*q. λa:t. (a, a, ())")->check(), TypeError);

    auto a = w.lit(Qualifier::a);
    w.axiom(w.kind_star(a), {"atype"});
    EXPECT_THROW(parse(w, "λa:atype. (a, a, ())")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "λa:atype. λi:3ₐ. (a, a, ())#i")->check());

    w.axiom(w.kind_star(w.lit(Qualifier::r)), {"rtype"});
    EXPECT_THROW(parse(w, "λr:rtype. ()")->check(), TypeError);
    EXPECT_THROW(parse(w, "λr:rtype. (42ₐ, 12ₐ)")->check(), TypeError);

    w.axiom(w.kind_star(w.lit(Qualifier::l)), {"ltype"});
    EXPECT_THROW(parse(w, "λl:ltype. ()")->check(), TypeError);
    EXPECT_THROW(parse(w, "λl:ltype. (42ₐ, 12ₐ)")->check(), TypeError);
    EXPECT_THROW(parse(w, "λl:ltype. (l, 42ₐ, l)")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "λl:ltype. λi:3ₐ. (l, (), l)#i")->check());
}

TEST(Substructural, TypeCheckPack) {
    World w;
    w.axiom(w.kind_arity(w.lit(Qualifier::a)), {"aarity"});
    EXPECT_THROW(parse(w, "‹i:aarity; (i, (), i)›")->check(), TypeError);
    EXPECT_THROW(parse(w, "λa:𝔸ᴬ.‹i:a; (i, i, ())›")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "λi:3ₐ. λa:𝔸ᴬ.‹j:a; (j, j, ())#i›")->check());

    w.axiom(w.kind_arity(w.lit(Qualifier::r)), {"rarity"});
    EXPECT_THROW(parse(w, "‹r:rarity; ()›")->check(), TypeError);
    EXPECT_THROW(parse(w, "‹r:rarity; (42ₐ, 12ₐ)›")->check(), TypeError);

    w.axiom(w.kind_arity(w.lit(Qualifier::l)), {"larity"});
    EXPECT_THROW(parse(w, "‹l:larity; ()›")->check(), TypeError);
    EXPECT_THROW(parse(w, "‹l:larity; (42ₐ, 12ₐ)›")->check(), TypeError);
    EXPECT_THROW(parse(w, "‹l:larity; (l, 42ₐ, l)›")->check(), TypeError);
    EXPECT_NO_THROW(parse(w, "‹l:larity; λi:3ₐ. (l, (), l)#i›")->check());
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
    //auto R = Qualifier::Relevant;
    auto A = Qualifier::Affine;
    auto L = Qualifier::Linear;
    auto a = w.affine();
    //auto l = w.linear();
    //auto r = w.relevant();
    //auto Star = w.kind_star();
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
    auto aT1 = w.var(w.kind_star(A), 0, {"T"});
    auto aT2 = w.var(w.kind_star(A), 1, {"T"});
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
    auto Star = w.kind_star();
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
    auto a = w.lit(Qualifier::a);
    auto Star = w.kind_star();

    w.axiom(w.pi(Star, w.kind_star(a)), {"ARef"});
    EXPECT_TRUE(parse(w, "ARef(\\0::*)")->has_values());
    w.axiom(parse(w, "ΠT: *. ΠT. ARef(T)"), {"NewARef"});
    w.axiom(parse(w, "ΠT: *. ΠARef(T). [T, ARef(T)]"), {"ReadARef"});
    w.axiom(parse(w, "ΠT: *. Π[ARef(T), T]. ARef(T)"), {"WriteARef"});
    w.axiom(parse(w, "ΠT: *. ΠARef(T). []"), {"FreeARef"});

    // TODO example use with reductions, etc
}

TEST(Substructural, AffineCapabilityRefs) {
    World w;
    auto a = w.lit(Qualifier::a);
    auto Star = w.kind_star();
    auto Nat = w.type_nat();
    auto n42 = w.lit(Nat, 42);

    w.axiom(w.pi(w.sigma({Star, Star}), w.kind_star(a)), {"CRef"});
    w.axiom(w.pi(Star, w.kind_star(a)), {"ACap"});

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. [C:*, CRef(T, C), ACap(C)]"), {"NewCRef"});
    auto ReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, CRef(T, C), ACap(C)]. [T, ACap(C)]"), {"ReadCRef"});
    w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C)]. T"), {"AliasReadCRef"});
    w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C), ACap(C), T]. ACap(C)"), {"WriteCRef"});
    w.axiom(parse(w, "ΠT: *. Π[C: *, CRef(T, C), ACap(C)]. []"), {"FreeCRef"});

    auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
    auto phantom = w.extract(ref42, 0_s);
    ref42->dump_rec();
    auto ref = w.extract(ref42, 1, {"ref"});
    ref->dump_rec();
    auto cap = w.extract(ref42, 2, {"cap"});
    cap->dump_rec();
    auto read42 = w.app(w.app(ReadRef, Nat), {phantom, ref, cap});
    read42->dump_rec();
    // TODO asserts to typecheck correct and incorrect usage
}

TEST(Substructural, AffineFractionalCapabilityRefs) {
    World w;
    auto a = w.lit(Qualifier::a);
    auto Star = w.kind_star();
    auto Nat = w.type_nat();
    auto n42 = w.lit(Nat, 42);
    auto n0 = w.lit(Nat, 0);

    w.axiom(w.pi(w.sigma({Star, Star}), Star), {"FRef"});
    auto Write = w.axiom(Star, {"Write"});
    auto Read = w.axiom(Star, {"Read"});
    auto WriteOrRead = w.variant({Write, Read});
    auto Wr = w.axiom(Write, {"Wr"});
    auto Rd = w.axiom(w.pi(WriteOrRead, Read), {"Rd"});
    w.axiom(w.pi(w.sigma({Star, WriteOrRead}), w.kind_star(a)), {"FCap"});

    auto NewRef = w.axiom(parse(w, "ΠT: *. ΠT. [C:*, FRef(T, C), FCap(C, Wr)]"), {"NewFRef"});
    auto ReadRef = w.axiom(parse(w, "ΠT: *. Π[C:*, F:{Write, Read}, FRef(T, C), FCap(C, F)]. [T, FCap(C, F)]"), {"ReadFRef"});
    ReadRef->dump_rec();
    auto WriteRef = w.axiom(parse(w, "ΠT: *. Π[C: *, FRef(T, C), FCap(C, Wr), T]. FCap(C, Wr)"), {"WriteFRef"});
    WriteRef->dump_rec();
    auto FreeRef = w.axiom(parse(w, "ΠT: *. Π[C: *, FRef(T, C), FCap(C, Wr)]. []"), {"FreeFRef"});
    FreeRef->dump_rec();
    auto SplitFCap = w.axiom(parse(w, "Π[C:*, F:{Write, Read}, FCap(C, F)]. [FCap(C, Rd(F)), FCap(C, Rd(F))]"), {"SplitFCap"});
    auto JoinFCap = w.axiom(parse(w, "Π[C:*, F:{Write, Read}, FCap(C, Rd(F)), FCap(C, Rd(F))]. FCap(C, F)"), {"JoinFCap"});

    auto ref42 = w.app(w.app(NewRef, Nat), n42, {"&42"});
    auto phantom = w.extract(ref42, 0_s);
    ref42->dump_rec();
    auto ref = w.extract(ref42, 1, {"ref"});
    ref->dump_rec();
    auto cap = w.extract(ref42, 2, {"cap"});
    cap->dump_rec();
    auto read42 = w.app(w.app(ReadRef, Nat), {phantom, Wr, ref, cap}, {"read42"});
    read42->dump_rec();
    auto read_cap = w.extract(read42, 1);
    auto write0 = w.app(w.app(WriteRef, Nat), {phantom, ref, read_cap, n0}, {"write0"});
    write0->dump_rec();
    auto split = w.app(SplitFCap, {phantom, Wr, write0}, {"split"});
    split->dump_rec();
    auto read0 = w.app(w.app(ReadRef, Nat), {phantom, w.app(Rd, Wr), ref, w.extract(split, 0_s)}, {"read0"});
    read0->dump_rec();
    auto join = w.app(JoinFCap, {phantom, Wr, w.extract(split, 1), w.extract(read0, 1)}, {"join"});
    join->dump_rec();
    auto free = w.app(w.app(FreeRef, Nat), {phantom, ref, join}, {"free"});
    free->dump_rec();
    // TODO asserts to typecheck correct and incorrect usage
}
