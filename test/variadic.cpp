#include "gtest/gtest.h"

#include "thorin/core/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Variadic, Unit) {
    core::World w;
    auto N = w.type_nat();
    ASSERT_EQ(w.variadic(0_s, N), w.unit());
    ASSERT_EQ(w.variadic(0_s, w.type_i(QualifierTag::Affine, iflags::so, 32)), w.unit(w.affine()));
}

TEST(Variadic, Misc) {
    World w;
    auto A = w.arity_kind();
    auto B = w.type_bool();
    auto N = w.type_nat();
    auto S = w.star();

    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    EXPECT_EQ(p2_4, p2_4b);
    auto v = w.variadic(5, N);
    EXPECT_TRUE(v->arity() == w.arity(5));

    auto t = w.tuple({w.val_nat_2(), w.val_nat_4()});
    EXPECT_TRUE(t->type()->isa<Variadic>());

    EXPECT_EQ(w.variadic(1, N), N);
    EXPECT_EQ(w.variadic({3, 1, 4}, N), w.variadic({3, 4}, N));

    auto sbn = w.sigma({B, N});
    auto list = w.axiom(w.pi(S, S), {"list"});
    auto lb = w.axiom(w.app(list, B), {"lb"});
    auto ln = w.axiom(w.app(list, N ), {"ln"});

    // Πa: A.ΠTs: [    a; *].Π[    a; list[            Ts#i        ]]. list[[    a;             Ts#i        ]]
    // Π   A.Π    [<0:A>; *].Π[<1:A>; list[<1:[<2:A>; *]>#<0:<2:A>>]]. list[[<2:A>; <2:[<3:A>; *]>#<0:<3:A>>]]
    auto ts_var = [&](auto i) { return w.var(w.variadic(w.var(A, i+1), S), i); };
    auto zip = w.axiom(w.pi(A, w.pi(w.variadic(w.var(A, 0), S),
                            w.pi(w.variadic(w.var(A, 1), w.app(list, w.extract(ts_var(1), w.var(w.var(A, 2), 0)))),
                                 w.app(list, w.variadic(w.var(A, 2), w.extract(ts_var(2), w.var(w.var(A, 3), 0))))))), {"zip"});

    EXPECT_EQ(w.app(w.app(w.app(zip, w.arity(2)), w.tuple({B, N})), {lb, ln})->type(), w.app(list, sbn));
    EXPECT_EQ(w.app(w.app(w.app(zip, w.arity(1)), B), lb)->type(), w.app(list, B));

    // Πa: A,Ts: [    a; *].Πlist[[    a;             Ts#i        ]]. [a; list[                Ts#i        ]]
    // Π   A,    [<0:A>; *].Πlist[[<1:A>; <1:[<2:A>; *]>#<0:<2:A>>]]. [<2:A>; list[<2:[<3:A>; *]>#<0:<3:A>>]]
    auto unzip = w.axiom(w.pi(A, w.pi(w.variadic(w.var(A, 0), S),
                              w.pi(w.app(list, w.variadic(w.var(A, 1), w.extract(ts_var(1), w.var(w.var(A, 2), 0)))),
                                   w.variadic(w.var(A, 2), w.app(list, w.extract(ts_var(2), w.var(w.var(A, 3), 0))))))), {"unzip"});
    auto l = w.axiom(w.app(list, sbn), {"l"});
    EXPECT_EQ(w.app(w.app(w.app(unzip, w.arity(2)), w.tuple({B, N})), l)->type(), w.sigma({lb->type(), ln->type()}));

    EXPECT_EQ(w.pi(w.variadic(3, N), B), w.pi(w.sigma({N, N, N}), B));
}

TEST(Variadic, LEA) {
    core::World w;
    auto B = w.type_bool();
    auto N = w.type_nat();

    auto s2 = w.sigma({B, N});
    auto ps2 = w.axiom(w.type_ptr(s2, w.val_nat_2()), {"ptr_s2"});
    ASSERT_EQ(w.op_lea(ps2, 0_s)->type(), w.type_ptr(B, w.val_nat_2()));
    ASSERT_EQ(w.op_lea(ps2, 1_s)->type(), w.type_ptr(N, w.val_nat_2()));

    auto v3n = w.variadic(w.arity(3), N);
    auto pv3n = w.axiom(w.type_ptr(v3n, w.val_nat_2()), {"ptr_v3n"});
    ASSERT_EQ(w.op_lea(pv3n, 0_s)->type(), w.type_ptr(N, w.val_nat_2()));
    ASSERT_EQ(w.op_lea(pv3n, 1_s)->type(), w.type_ptr(N, w.val_nat_2()));
    ASSERT_EQ(w.op_lea(pv3n, 2_s)->type(), w.type_ptr(N, w.val_nat_2()));

    // TODO allow assignability/conversion from values of nominal type to structural type
#if 0
    auto n2 = w.sigma_type(2, {"n2"})->set(0, B)->set(1, N);
    auto pn2 = w.axiom(w.type_ptr(n2, w.val_nat_4()), {"ptr_n2"});
    ASSERT_EQ(w.op_lea(pn2, 0_s)->type(), w.type_ptr(B, w.val_nat_2()));
    ASSERT_EQ(w.op_lea(pn2, 1_s)->type(), w.type_ptr(N, w.val_nat_2()));
#endif
}

#if 0
TEST(Variadic, Multi) {
    World w;
    auto N = w.type_nat();

    auto v = w.variadic({3, 8, 5}, N);
    auto x = w.var(v, 0);
    ASSERT_EQ(v, w.variadic(3, w.variadic(8, w.variadic(5, N))));
    ASSERT_EQ(w.extract(w.extract(w.extract(x, 1), 2), 3)->type(), N);
    auto e1 = w.extract(x, 2);
    auto e2 = w.extract(e1, 2);
    auto e3 = w.extract(e2, 3);
    auto f1 = w.var(w.variadic({8, 5}, N), 0);
    auto f2 = w.extract(f1, 2);
    auto f3 = w.extract(f2, 3);
    ASSERT_EQ(w.lambda(w.variadic(3, w.variadic(8, w.variadic(5, N))), e3),
              w.lambda(w.sigma({w.variadic({8, 5}, N), w.variadic({8, 5}, N), w.variadic({8, 5}, N)}), f3));

    auto A = w.arity_kind();
    auto arity_tuple = [&](auto i) { return w.variadic(w.var(A, i), A); };
    auto a = [&](auto i) { return w.var(A, i); };
    auto build_variadic = w.variadic(a(1), w.extract(w.var(arity_tuple(2), 1), w.var(a(2), 0)));
    ASSERT_TRUE(build_variadic->free_vars().test(0));
    ASSERT_TRUE(build_variadic->free_vars().test(1));
    ASSERT_TRUE(build_variadic->free_vars().none_begin(2));
    auto arity_tuple_to_type = w.lambda(A, w.lambda(arity_tuple(0), build_variadic));
    ASSERT_TRUE(arity_tuple_to_type->free_vars().none());
    ASSERT_TRUE(arity_tuple_to_type->type()->free_vars().none());

    auto args = w.tuple({w.arity(3), w.arity(4)});
    ASSERT_EQ(w.app(w.app(arity_tuple_to_type, w.arity(2)), args), w.sigma({w.arity(3), w.arity(4)}));

    auto l = w.lambda(A, w.lambda(arity_tuple(0),
                                  w.variadic(w.variadic(a(1), w.extract(w.var(arity_tuple(2), 1), w.var(a(2), 0))), N)));
    ASSERT_EQ(w.app(w.app(l, w.arity(2)), args), w.variadic({3, 4}, N));

}
#endif

TEST(Variadic, InlineSigmaInterOp) {
    World w;
    auto star = w.star();
    auto pair = w.sigma({star, star});
    ASSERT_TRUE(pair->isa<Variadic>());

    auto lam = w.lambda(pair, w.type_nat(), {"lam"});
    auto app = w.app(lam, w.var(pair, 1));
    ASSERT_EQ(app, w.type_nat());
}

TEST(Variadic, Nested) {
    World w;
    auto A = w.arity_kind();
    auto N = w.type_nat();
    auto S = w.star();

    ASSERT_EQ(w.variadic(w.sigma({w.arity(3), w.arity(2)}), w.var(S, 1)),
              w.variadic(3, w.variadic(2, w.var(S, 2))));
    ASSERT_EQ(w.variadic(w.variadic(w.arity(3), w.arity(2)), w.var(S, 1)),
              w.variadic(2, w.variadic(2, w.variadic(2, w.var(S, 3)))));
    ASSERT_EQ(w.variadic(w.variadic(3, w.var(A, 1)), N),
              w.variadic(w.var(A, 0), w.variadic(w.var(A, 1), w.variadic(w.var(A, 2), N))));

    auto f = w.axiom(w.pi(w.variadic(3, w.arity(2)), S), {"f"});
    auto v = w.variadic(w.variadic(3, w.arity(2)), w.app(f, w.var(w.variadic(3, w.arity(2)), 0)));
    const Def* outer_sigmas[2];
    for (int z = 0; z != 2; ++z) {
        const Def* inner_sigmas[2];
        for (int y = 0; y != 2; ++y) {
            const Def* args[2];
            for (int x = 0; x != 2; ++x)
                args[x] = w.app(f, {w.index(2, z), w.index(2, y), w.index(2, x)});
            inner_sigmas[y] = w.sigma(args);
        }
        outer_sigmas[z] = w.sigma(inner_sigmas);
    }
    ASSERT_EQ(v, w.sigma(outer_sigmas));
}

#if 0
TEST(XLA, Misc) {
    World w;
    auto N = w.type_nat();
    auto a3 = w.arity(3);
    auto a4 = w.arity(4);
    auto a5 = w.arity(5);

    Env env;
    auto arr = parse(w, "λ[s:𝕄, t:*]. [s;t]");
    env["arr"] = arr;
    auto op = w.axiom(parse(w, "Π[s:𝕄, t:*]. Π[x: arr(s, t), y: arr(s, t)]. arr(s, t).", env), {"op"});
    auto s = w.sigma({a4, a3, a5});
    auto a = w.app(arr, {s, N});
    auto lhs = w.axiom(a, {"lhs"});
    auto rhs = w.axiom(a, {"rhs"});
    w.app(w.app(op, {s, N}), {lhs, rhs})->dump();
    w.app(w.app(op, {s, N}), {lhs, rhs})->type()->dump();

    auto reduce = w.axiom(parse(w, "Π[s:𝕄, t:*, Π[t, t].t]. Π[x: arr(s, t), t]. t"), {"reduce"});
    reduce->type()->dump();
}
#endif

