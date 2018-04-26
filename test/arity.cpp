#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/fe/parser.h"

using namespace thorin;
using namespace thorin::fe;

TEST(Arity, Successor) {
    World w;
    auto a0 = w.arity(0);
    EXPECT_EQ(w.arity(1), w.arity_succ(a0));
    EXPECT_EQ(w.arity(42), w.arity_succ(w.arity(41)));
    EXPECT_TRUE(w.arity_succ(w.var(w.arity_kind(), 0))->isa<App>());
}

TEST(Arity, Sigma) {
    World w;
    auto a0 = w.arity(0);
    auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto a3 = w.arity(3);

    EXPECT_EQ(a0, w.sigma({a0, a0}));
    EXPECT_EQ(a0, w.sigma({a1, a0}));
    EXPECT_EQ(a0, w.sigma({a0, a1}));

    EXPECT_EQ(w.multi_arity_kind(), w.sigma({a2, a3})->type());
    EXPECT_EQ(w.star(), w.sigma({a2, w.sigma({a3, a2}), a3})->type());
}

TEST(Arity, DependentSigma) {
    World w;
    // auto a0 = w.arity(0);
    // auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto a3 = w.arity(3);

    auto arity_dep = w.sigma({a2, w.extract(w.tuple({a3, a2}), w.var(a2, 0))});
    arity_dep->dump();
    arity_dep->type()->dump();
    // TODO actual test on arity_dep
}

TEST(Arity, Variadic) {
    World w;
    auto a0 = w.arity(0);
    auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto unit = w.unit();

    EXPECT_EQ(unit, w.variadic({a0, a1, a2}, unit));
    EXPECT_EQ(unit, w.variadic({a1, a0}, unit));
    EXPECT_EQ(w.unit_kind(), w.variadic({a0, a2}, w.arity_kind()));
    EXPECT_EQ(w.variadic(a2, w.unit_kind()), w.variadic({a2, a0, a2}, w.arity_kind()));
}

TEST(Arity, Pack) {
    World w;
    auto a0 = w.arity(0);
    auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto unit = w.unit();
    auto tuple0t = w.val_unit_kind();

    EXPECT_EQ(tuple0t, w.pack({a0, a1, a2}, unit));
    EXPECT_EQ(tuple0t, w.pack({a1, a0}, unit));
    EXPECT_EQ(w.pack(a2, tuple0t), w.pack({a2, a0, a2}, unit));
}

TEST(Arity, Subkinding) {
    World w;
    auto a0 = w.arity(0);
    auto a3 = w.arity(3);
    auto A = w.arity_kind();
    auto vA = w.var(A, 0);
    auto M = w.multi_arity_kind();

    EXPECT_TRUE(A->assignable(a0));
    EXPECT_TRUE(M->assignable(a0));
    EXPECT_TRUE(A->assignable(a3));
    EXPECT_TRUE(M->assignable(a3));
    EXPECT_TRUE(A->assignable(vA));
    EXPECT_TRUE(M->assignable(vA));

    auto sig = w.sigma({M, w.variadic(w.var(M, 0), w.star())});
    EXPECT_TRUE(sig->assignable(w.tuple({a0, w.val_unit_kind()})));
}

TEST(Arity, PrefixExtract) {
    World w;
    auto a2 = w.arity(2);
    auto i1_2 = w.index(2, 1);
    auto a3 = w.arity(3);
    auto i2_3 = w.index(3, 2);

    auto unit = w.unit();
    auto ma = w.sigma({a2, a3, a2});
    auto marray = w.variadic(ma, unit);
    auto v_marray = w.var(marray, 0);

    EXPECT_EQ(unit, w.extract(v_marray, w.tuple({i1_2, i2_3, i1_2}))->type());
    EXPECT_EQ(w.variadic(a2, unit), w.extract(v_marray, w.tuple({i1_2, i2_3}))->type());
    EXPECT_EQ(w.variadic(a3, w.variadic(a2, unit)), w.extract(v_marray, i1_2)->type());
    EXPECT_EQ(w.variadic(w.sigma({a3, a2}), unit), w.extract(v_marray, i1_2)->type());
}

TEST(Arity, DependentArityEliminator) {
    World w;
    auto lam_bool = w.lambda(w.arity_kind(), w.type_bool());
    auto step_negate = w.lambda(w.arity_kind(), w.lambda(w.type_bool(), w.op_bnot(w.var(w.type_bool(), 0))));
    auto arity_is_even = w.app(w.app(w.app(w.app(w.arity_eliminator(), w.unlimited()), lam_bool), w.lit_true()), step_negate);
    EXPECT_EQ(w.lit_true(), w.app(arity_is_even, w.arity(0)));
    EXPECT_EQ(w.lit_false(), w.app(arity_is_even, w.arity(3)));
}

TEST(Arity, ArityRecursors) {
    World w;
    auto double_step = fe::parse(w, "λcurr:𝔸. λacc:𝔸. ASucc (ᵁ, ASucc (ᵁ, acc))");
    auto arity_double = w.app(w.app(w.app(w.arity_recursor_to_arity(), w.unlimited()), w.arity(0)), double_step);
    EXPECT_EQ(w.arity(0), w.app(arity_double, w.arity(0)));
    EXPECT_EQ(w.arity(8), w.app(arity_double, w.arity(4)));
    auto quad_step = fe::parse(w, "λcurr:𝔸. λacc:𝕄. [ASucc (ᵁ, curr); ASucc (ᵁ, curr)])");
    auto arity_quad = w.app(w.app(w.app(w.arity_recursor_to_multi(), w.unlimited()), w.arity(0)), quad_step);
    EXPECT_EQ(w.arity(0), w.app(arity_quad, w.arity(0)));
    EXPECT_EQ(w.variadic(w.arity(3), w.arity(3)), w.app(arity_quad, w.arity(3)));
    auto nest_step = fe::parse(w, "λcurr:𝔸. λacc:*. [ASucc (ᵁ, curr); acc])");
    auto arity_nest = w.app(w.app(w.app(w.arity_recursor_to_star(), w.unlimited()), w.type_nat()), nest_step);
    EXPECT_EQ(w.type_nat(), w.app(arity_nest, w.arity(0)));
    EXPECT_EQ(w.type_nat(), w.app(arity_nest, w.arity(1)));
    EXPECT_EQ(w.variadic(w.arity(3), w.variadic(w.arity(2), w.type_nat())), w.app(arity_nest, w.arity(3)));
}

TEST(Arity, DependentIndexEliminator) {
    World w;
    auto elim = w.index_eliminator();
    auto u = w.unlimited();
    auto elimu = w.app(elim, u);
    auto to_arity2 = fe::parse(w, "λa:𝔸. λi:a. 2ₐ");
    auto base_is_even = fe::parse(w, "λa:𝔸. 1₂");
    auto step_is_even = fe::parse(w, "λa:𝔸. λi:a. λis_even:2ₐ. (1₂, 0₂)#is_even");
    auto index_is_even = w.app(w.app(w.app(elimu, to_arity2), base_is_even), step_is_even);

    EXPECT_EQ(w.index(2, 1), w.app(w.app(index_is_even, w.arity(3)), w.index(3, 0)));
    EXPECT_EQ(w.index(2, 1), w.app(w.app(index_is_even, w.arity(8)), w.index(8, 4)));
    EXPECT_EQ(w.index(2, 0), w.app(w.app(index_is_even, w.arity(8)), w.index(8, 1)));
}
