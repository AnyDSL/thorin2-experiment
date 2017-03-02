#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Lambda, PolyId) {
    World w;
    auto n16 = w.val_nat_16();
    auto n23 = w.val_nat(23);
    auto n42 = w.val_nat(42);

    ASSERT_EQ(n16, w.val_nat(16));
    ASSERT_EQ(n23, w.val_nat(23));
    ASSERT_EQ(n42, w.val_nat(42));

    auto bb = w.var(w.type_nat(), 0);
    ASSERT_TRUE(bb->free_vars().test(0));
    ASSERT_TRUE(w.lambda(w.type_nat(), bb)->free_vars().none());
    ASSERT_TRUE(w.pi({w.star(), w.var(w.star(), 0)}, w.var(w.star(), 1))->free_vars().none());

    // λT:*.λx:T.x
    auto T_1 = w.var(w.star(), 0, {"T"});
    ASSERT_TRUE(T_1->free_vars().test(0));
    ASSERT_FALSE(T_1->free_vars().test(1));
    auto T_2 = w.var(w.star(), 1, {"T"});
    ASSERT_TRUE(T_2->free_vars().test(1));
    ASSERT_TRUE(T_2->free_vars().any_begin(1));
    auto x = w.var(T_2, 0, {"x"});
    auto poly_id = w.lambda(T_2->type(), w.lambda(T_1, x));
    ASSERT_FALSE(poly_id->free_vars().test(0));
    ASSERT_FALSE(poly_id->free_vars().test(1));
    ASSERT_FALSE(poly_id->free_vars().any_end(2));
    ASSERT_FALSE(poly_id->free_vars().any_begin(0));

    // λx:w.type_nat().x
    auto int_id = w.app(poly_id, w.type_nat());
    ASSERT_EQ(int_id, w.lambda(w.type_nat(), w.var(w.type_nat(), 0)));
    ASSERT_EQ(w.app(int_id, n23), n23);
}

TEST(Lambda, Normalization) {
    World w;
    auto B = w.type_bool();
    auto N = w.type_nat();
    auto I = w.lambda(N, w.var(N, 0));
    auto fNNBNI = w.axiom(w.pi({N, N, B, N, w.pi(N, N)}, w.star()), {"fNNBNI"});
    auto fNNNNI = w.axiom(w.pi({N, N, N, N, w.pi(N, N)}, w.star()), {"fNNNNI"});
    auto gNNBNI = w.axiom(w.pi({N, N, B, N, w.pi(N, N)}, N), {"gNNBNI"});
    auto gNNNNI = w.axiom(w.pi({N, N, N, N, w.pi(N, N)}, N), {"gNNNNI"});
    auto sNNB = w.sigma({N, N, B});
    auto sNNN = w.sigma({N, N, N});
    auto vNNB = w.var(sNNB, 0);
    auto vNNN = w.var(sNNN, 0);

    ASSERT_EQ(w.pi(sNNB,      w.app(fNNBNI, {w.extract(vNNB, 0_s), w.extract(vNNB, 1), w.extract(vNNB, 2), w.var(N, 7), I})),
              w.pi({N, N, B}, w.app(fNNBNI, {w.var(N, 2),          w.var(N, 1),        w.var(B, 0),        w.var(N, 9), I})));
    ASSERT_EQ(w.pi(sNNN,      w.app(fNNNNI, {w.extract(vNNN, 0_s), w.extract(vNNN, 1), w.extract(vNNN, 2), w.var(N, 7), I})),
              w.pi({N, N, N}, w.app(fNNNNI, {w.var(N, 2),          w.var(N, 1),        w.var(N, 0),        w.var(N, 9), I})));

    ASSERT_EQ(w.lambda(sNNB,      w.app(gNNBNI, {w.extract(vNNB, 0_s), w.extract(vNNB, 1), w.extract(vNNB, 2), w.var(N, 7), I})),
              w.lambda({N, N, B}, w.app(gNNBNI, {w.var(N, 2),          w.var(N, 1),        w.var(B, 0),        w.var(N, 9), I})));
    ASSERT_EQ(w.lambda(sNNN,      w.app(gNNNNI, {w.extract(vNNN, 0_s), w.extract(vNNN, 1), w.extract(vNNN, 2), w.var(N, 7), I})),
              w.lambda({N, N, N}, w.app(gNNNNI, {w.var(N, 2),          w.var(N, 1),        w.var(N, 0),        w.var(N, 9), I})));

    auto l1a = w.nominal_lambda(sNNB, N)     ->set(w.app(gNNBNI, {w.extract(vNNB, 0_s), w.extract(vNNB, 1), w.extract(vNNB, 2), w.var(N, 7), I}));
    auto l1b = w.nominal_lambda({N, N, B}, N)->set(w.app(gNNBNI, {w.var(N, 2),          w.var(N, 1),        w.var(B, 0),        w.var(N, 9), I}));
    ASSERT_EQ(l1a->type(), l1b->type());
    ASSERT_EQ(l1a->body(), l1b->body());
    ASSERT_NE(l1a, l1b);

    auto l2a = w.nominal_lambda(sNNN, N)     ->set(w.app(gNNNNI, {w.extract(vNNN, 0_s), w.extract(vNNN, 1), w.extract(vNNN, 2), w.var(N, 7), I}));
    auto l2b = w.nominal_lambda({N, N, N}, N)->set(w.app(gNNNNI, {w.var(N, 2),          w.var(N, 1),        w.var(N, 0),        w.var(N, 9), I}));
    ASSERT_EQ(l2a->type(), l2b->type());
    ASSERT_EQ(l2a->body(), l2b->body());
    ASSERT_NE(l2a, l2b);
}

static const int test_num_vars = 1000;

TEST(Lambda, AppCurry) {
    World w;
    const Def* cur = w.val_nat_32();
    for (int i = 0; i < test_num_vars; ++i)
        cur = w.lambda(w.type_nat(), cur);

    for (int i = 0; i < test_num_vars; ++i)
        cur = w.app(cur, w.val_nat_64());

    ASSERT_EQ(cur, w.val_nat_32());
}

TEST(Lambda, AppArity) {
    World w;
    auto l = w.lambda(DefArray(test_num_vars, w.type_nat()), w.val_nat_32());
    auto r = w.app(l, DefArray(test_num_vars, w.val_nat_64()));
    ASSERT_EQ(r, w.val_nat_32());
}
