#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Parser, SimplePi) {
    WorldBase w;
    auto def = w.pi(w.star(), w.pi(w.var(w.star(), 0), w.var(w.star(), 1)));
    ASSERT_EQ(parse(w, "ΠT:*. ΠU:T. T"), def);
}

TEST(Parser, SimpleLambda) {
    WorldBase w;
    auto def = w.lambda(w.star(), w.lambda(w.var(w.star(), 0), w.var(w.var(w.star(), 1), 0)));
    ASSERT_EQ(parse(w, "λT:*. λx:T. x"), def);
}

TEST(Parser, SigmaVariadic) {
    WorldBase w;

    ASSERT_EQ(parse(w, "[]"), w.unit());

    auto s = w.sigma({w.star(), w.var(w.star(), 0)});
    ASSERT_EQ(parse(w, "[T:*, T]"), s);

    auto v = w.variadic(w.multi_arity_kind(), w.var(w.multi_arity_kind(), 0));
    ASSERT_EQ(parse(w, "[x:𝕄; x]"), v);
}

TEST(Parser, TypeAxiom) {
    WorldBase w;
    auto S = w.star();
    auto N = w.axiom(S, {"nat"});
    auto sig = w.sigma({N, N});
    auto typ = w.axiom(w.pi(sig, S), {"typ"});
    auto typ2 = w.axiom(w.pi(sig, S), {"typ2"});
    Env env;
    env["nat"] = N;
    env["typ"] = typ;
    env["typ2"] = typ2;
    auto def = w.pi(sig, w.pi(w.app(typ, w.tuple({w.extract(w.var(sig, 0), (size_t)1), w.extract(w.var(sig, 0), (size_t)0)})),
                              w.app(typ2, w.var(sig, 1))));
    ASSERT_EQ(parse(w, "Πp:[n: nat, m: nat]. Πtyp(m, n). typ2(p)", env), def);
}

TEST(Parser, IntArithOp) {
    WorldBase w;
    auto Q = w.qualifier_type();
    auto S = w.star();
    auto N = w.axiom(S, {"nat" });
    auto MA = w.multi_arity_kind();
    auto sig = w.sigma({Q, N, N});
    auto type_i = w.axiom(w.pi(sig, w.star(w.extract(w.var(sig, 0), (size_t)0))), {"int"});
    Env env;
    env["nat"]  = N;
    env["int"]  = type_i;
    auto dom = w.sigma({w.app(type_i, w.var(sig, 0)), w.app(type_i, w.var(sig, 1))});
    auto def = w.pi(MA, w.pi(sig, w.pi(dom, w.app(type_i, w.var(sig, 1)))));
    auto i_arithop = parse(w, "Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[int(q, f, w),  int(q, f, w)].  int(q, f, w)", env);
    ASSERT_EQ(i_arithop, def);
}
