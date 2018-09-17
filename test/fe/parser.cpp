#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/fe/parser.h"

using namespace thorin;
using namespace thorin::fe;

TEST(Parser, Unit) {
    World w;
    EXPECT_EQ(parse(w, "()"), w.val_unit());
    EXPECT_EQ(parse(w, "()")->type(), w.unit());
}

TEST(Parser, Simple) {
    World w;
    EXPECT_EQ(parse(w, "bool"), w.type_bool());
    EXPECT_EQ(parse(w, "nat"), w.type_nat());
}

TEST(Parser, SimplePi) {
    World w;
    auto def = w.pi(w.kind_star(), w.pi(w.var(w.kind_star(), 0), w.var(w.kind_star(), 1)));
    EXPECT_EQ(parse(w, "ΠT:*. ΠU:T. T"), def);
}

TEST(Parser, SimpleLambda) {
    World w;
    auto def = w.lambda(w.kind_star(), w.lambda(w.var(w.kind_star(), 0), w.var(w.var(w.kind_star(), 1), 0)));
    EXPECT_EQ(parse(w, "λT:*. λx:T. x"), def);
}

TEST(Parser, SimpleSigma) {
    World w;

    EXPECT_EQ(parse(w, "[]"), w.unit());

    auto s = w.sigma({w.kind_star(), w.var(w.kind_star(), 0)});
    EXPECT_EQ(parse(w, "[T:*, T]"), s);
}

TEST(Parser, DeBruijn) {
    World w;
    auto S = w.kind_star();
    EXPECT_EQ(parse(w, "λ*.\\0"), w.lambda(S, w.var(S, 0)));
    EXPECT_EQ(parse(w, "λ*.\\1::nat"), w.lambda(S, w.var(w.type_nat(), 1)));
}

TEST(Parser, SimpleVariadic) {
    World w;
    auto S = w.kind_star();
    auto M = w.kind_multi();

    // TODO simplify further once we can parse arity literals
    auto v = w.pi(M, w.pi(w.variadic(w.var(M, 0), S), S));
    EXPECT_EQ(parse(w, "Πa:𝕄. Π«x:a; *». *"), v);
}

TEST(Parser, Arities) {
    World w;
    EXPECT_EQ(parse(w, "0ₐ"), w.arity(0));
    EXPECT_EQ(parse(w, "42ₐ"), w.arity(42));
    EXPECT_EQ(parse(w, "0ₐᵁ"), w.arity(0));
    EXPECT_EQ(parse(w, "1ₐᴿ"), w.arity(w.lit(Qualifier::r), 1));
    EXPECT_EQ(parse(w, "2ₐᴬ"), w.arity(w.lit(Qualifier::a), 2));
    EXPECT_EQ(parse(w, "3ₐᴸ"), w.arity(w.lit(Qualifier::l), 3));
    EXPECT_EQ(parse(w, "Πq:ℚ.42ₐq"), w.pi(w.type_qualifier(), w.arity(w.var(w.type_qualifier(), 0), 42)));
}

TEST(Parser, Indices) {
    World w;
    EXPECT_EQ(parse(w, "0₁"), w.lit_index(1, 0));
    EXPECT_EQ(parse(w, "42₁₉₀"), w.lit_index(190, 42));
    EXPECT_EQ(parse(w, "4₅ᵁ"), w.lit_index(w.arity(w.lit(Qualifier::u), 5), 4));
    EXPECT_EQ(parse(w, "4₅ᴿ"), w.lit_index(w.arity(w.lit(Qualifier::r), 5), 4));
    EXPECT_EQ(parse(w, "4₅ᴬ"), w.lit_index(w.arity(w.lit(Qualifier::a), 5), 4));
    EXPECT_EQ(parse(w, "4₅ᴸ"), w.lit_index(w.arity(w.lit(Qualifier::l), 5), 4));
    EXPECT_EQ(parse(w, "λq:ℚ.4₅q"), w.lambda(w.type_qualifier(), w.lit_index(w.arity(w.var(w.type_qualifier(), 0), 5), 4)));
}

TEST(Parser, Kinds) {
    World w;
    EXPECT_EQ(parse(w, "* "), w.kind_star());
    EXPECT_EQ(parse(w, "*ᵁ"), w.kind_star());
    EXPECT_EQ(parse(w, "*ᴿ"), w.kind_star(Qualifier::r));
    EXPECT_EQ(parse(w, "*ᴬ"), w.kind_star(Qualifier::a));
    EXPECT_EQ(parse(w, "*ᴸ"), w.kind_star(Qualifier::l));
    EXPECT_EQ(parse(w, "Πq:ℚ.*q"), w.pi(w.type_qualifier(), w.kind_star(w.var(w.type_qualifier(), 0))));
    EXPECT_EQ(parse(w, "𝔸 "), w.kind_arity());
    EXPECT_EQ(parse(w, "𝔸ᵁ"), w.kind_arity());
    EXPECT_EQ(parse(w, "𝔸ᴿ"), w.kind_arity(Qualifier::r));
    EXPECT_EQ(parse(w, "𝔸ᴬ"), w.kind_arity(Qualifier::a));
    EXPECT_EQ(parse(w, "𝔸ᴸ"), w.kind_arity(Qualifier::l));
    EXPECT_EQ(parse(w, "Πq:ℚ.𝔸q"), w.pi(w.type_qualifier(), w.kind_arity(w.var(w.type_qualifier(), 0))));
    EXPECT_EQ(parse(w, "𝕄 "), w.kind_multi());
    EXPECT_EQ(parse(w, "𝕄ᵁ"), w.kind_multi());
    EXPECT_EQ(parse(w, "𝕄ᴿ"), w.kind_multi(Qualifier::r));
    EXPECT_EQ(parse(w, "𝕄ᴬ"), w.kind_multi(Qualifier::a));
    EXPECT_EQ(parse(w, "𝕄ᴸ"), w.kind_multi(Qualifier::l));
    EXPECT_EQ(parse(w, "Πq:ℚ.𝕄q"), w.pi(w.type_qualifier(), w.kind_multi(w.var(w.type_qualifier(), 0))));
}

TEST(Parser, ComplexVariadics) {
    World w;
    auto S = w.kind_star();
    auto M = w.kind_multi();

    auto v = w.pi(M, w.pi(w.variadic(w.var(M, 0), S),
                          w.variadic(w.var(M, 1),
                                     w.extract(w.var(w.variadic(w.var(M, 2), S), 1), w.var(w.var(M, 2), 0)))));
    EXPECT_EQ(parse(w, "Πa:𝕄. Πx:«a; *». «i:a; x#i»"), v);
}

TEST(Parser, NestedBinders) {
    World w;
    auto S = w.kind_star();
    auto N = w.type_nat();
    auto sig = w.sigma({N, N});
    auto typ = w.axiom(w.pi(sig, S), {"typ"});
    auto typ2 = w.axiom(w.pi(sig, S), {"typ2"});
    auto def = w.pi(sig, w.pi(w.app(typ, w.tuple({w.extract(w.var(sig, 0), 1), w.extract(w.var(sig, 0), 0_u64)})),
                              w.app(typ2, w.var(sig, 1))));
    EXPECT_EQ(parse(w, "Πp:[n: nat, m: nat]. Πtyp(m, n). typ2(p)"), def);
}

TEST(Parser, NestedBinders2) {
    World w;
    auto S = w.kind_star();
    auto N = w.type_nat();
    auto sig = w.sigma({N, w.sigma({N, N})});
    auto typ = w.axiom(w.pi(w.sigma({N, N, w.sigma({N, N})}), S), {"typ"});
    auto def = w.pi(sig, w.app(typ, w.tuple({w.extract(w.extract(w.var(sig, 0), 1), 1),
                                             w.extract(w.extract(w.var(sig, 0), 1), 0_u64),
                                             w.extract(w.var(sig, 0), 1)})));
    EXPECT_EQ(parse(w, "Π[m: nat, n: [n0 : nat, n1: nat]]. typ(n1, n0, n)"), def);
}

TEST(Parser, NestedDependentBinders) {
    World w;
    auto S = w.kind_star();
    auto N = w.type_nat();
    auto dtyp = w.axiom(w.pi(N, S), {"dt"});
    auto npair = w.sigma({N, N});
    auto sig = w.sigma({npair, w.app(dtyp, w.extract(w.var(npair, 0), 1))});
    auto typ = w.axiom(w.pi(w.sigma({N, w.app(dtyp, w.var(N, 0))}), S), {"typ"});
    auto def = w.pi(sig, w.app(typ, w.tuple({w.extract(w.extract(w.var(sig, 0), 0_u64), 1),
                                             w.extract(w.var(sig, 0), 1)})));
    EXPECT_EQ(parse(w, "Π[[n0 : nat, n1: nat], d: dt(n1)]. typ(n1, d)"), def);

    w.axiom("int", "Πnat. *");
    w.axiom("add", "Πf: nat. Πw: nat. Πs: 𝕄. Π[ «s; int w», «s; int w»]. «s; int w»");
    EXPECT_TRUE(parse(w, "λ[f: nat, w: nat, x: int w]. add f w 1ₐ (0u64::int w, x)")->isa<Lambda>());
}

TEST(Parser, IntArithOp) {
    World w;
    auto Q = w.type_qualifier();
    auto N = w.type_nat();
    auto MA = w.kind_multi();
    auto sig = w.sigma({Q, N, N});
    auto type_i = w.axiom(w.pi(sig, w.kind_star(w.extract(w.var(sig, 0), 0_u64))), {"int"});
    auto dom = w.sigma({w.app(type_i, w.var(sig, 0)), w.app(type_i, w.var(sig, 1))});
    auto def = w.pi(MA, w.pi(sig, w.pi(dom, w.app(type_i, w.var(sig, 1)))));
    auto i_arithop = parse(w, "Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[int(q, f, w),  int(q, f, w)].  int(q, f, w)");
    EXPECT_EQ(i_arithop, def);
}

TEST(Parser, Apps) {
    World w;
    auto a5 = w.arity(Qualifier::a, 5);
    auto i0_5 = w.lit_index(a5, 0);
    auto ma = w.kind_multi(Qualifier::a);
    EXPECT_EQ(parse(w, "(λs: 𝕄ᴬ. λq: ℚ. λi: s. (s, q, i)) 5ₐᴬ ᴿ 0₅ᴬ"), w.tuple({a5, w.lit(Qualifier::r), i0_5}));
    auto ax = w.axiom(w.pi(ma, w.pi(w.type_qualifier(), w.kind_star())), {"ax"});
    EXPECT_EQ(parse(w, "(ax 5ₐᴬ) ᴿ"), w.app(w.app(ax, a5), w.lit(Qualifier::r)));
    EXPECT_EQ(parse(w, "ax 5ₐᴬ ᴿ"), w.app(w.app(ax, a5), w.lit(Qualifier::r)));
}

TEST(Parser, Let) {
    World w;
    auto S = w.kind_star();
    auto nat = w.type_nat();
    EXPECT_EQ(parse(w, "x = nat; (x, x)"), w.tuple({nat, nat}));
    EXPECT_EQ(parse(w, "x = nat; y = x; (x, y)"), w.tuple({nat, nat}));
    EXPECT_EQ(parse(w, "x = *; [x, y = nat; y]"), w.sigma({S, nat}));
    EXPECT_EQ(parse(w, "x = *; x = nat; [x, x]"), w.sigma({nat, nat}));
    auto SS = w.sigma({S, S});
    auto TSS = w.sigma({SS, w.extract(w.var(SS, 0), 1), w.extract(w.var(SS,1), 0_u64)});
    EXPECT_EQ(parse(w, "λp:[k = *; [t1:k, t2:k], x:t2, y:t1].(x, y)"), w.lambda(TSS, w.tuple({w.extract(w.var(TSS, 0), 1), w.extract(w.var(TSS, 0), 2)})));
}

TEST(Parser, LetShadow) {
    World w;
    auto S = w.kind_star();
    auto nat = w.type_nat();
    EXPECT_EQ(parse(w, "λx:*.(x = nat; x, x)"), w.lambda(S, w.tuple({nat, w.var(S, 0)})));
    EXPECT_EQ(parse(w, "λx:*.[x = nat; x, x]"), w.lambda(S, w.sigma({nat, w.var(S, 1)})));
    EXPECT_EQ(parse(w, "x = nat; λx:x.x"), w.lambda(nat, w.var(nat, 0)));
    EXPECT_EQ(parse(w, "λ[x = nat; x:x].x"), w.lambda(nat, w.var(nat, 0)));
}

void check_nominal(const Def* def, Def::Tag tag, const Def* type) {
    EXPECT_EQ(def->tag(), tag);
    EXPECT_EQ(def->type(), type);
}

TEST(Parser, NominalLambda) {
    World w;
    auto S = w.kind_star();
    auto nominal_id = parse(w, "l :: Π*.* := λt.t; l");
    check_nominal(nominal_id, Def::Tag::Lambda, w.pi(S, S));
    // the following does not work, as it will be eta-reduced with the current parsing of nominals,
    // but such nominals are quite useless anyway
    // auto nominal_endless_id = parse(w, "l :: Π*.* := λt:*.l t; l");
    // check_nominal(nominal_id, Def::Tag::Lambda, w.pi(S, S), {w.app(nominal_endless_id, w.var(S, 0))});
    auto ltup = parse(w, "l :: Π*.[*,*] := λt.(t, (l t)#0₂); l");
    check_nominal(ltup, Def::Tag::Lambda, w.pi(S, w.sigma({S, S})));
}

// TODO nominal sigma tests
// TEST(Parser, NominalSigma) {
// }
