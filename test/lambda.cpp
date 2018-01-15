#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Pi, Kinds) {
    World w;
    EXPECT_EQ(w.pi(w.star(), w.star())->type(), w.universe());
    EXPECT_EQ(w.pi(w.star(), w.arity_kind())->type(), w.universe());
    EXPECT_EQ(w.pi(w.type_nat(), w.type_nat())->type(), w.star());
    EXPECT_EQ(w.pi(w.type_nat(), w.arity(2))->type(), w.star());
    EXPECT_EQ(w.pi(w.type_nat(), w.sigma({w.arity(2), w.arity(3)}))->type(), w.star());
}

TEST(Lambda, PolyId) {
    World w;
    auto n16 = w.val_nat_16();
    auto n23 = w.val_nat(23);
    auto n42 = w.val_nat(42);

    EXPECT_EQ(n16, w.val_nat(16));
    EXPECT_EQ(n23, w.val_nat(23));
    EXPECT_EQ(n42, w.val_nat(42));

    auto bb = w.var(w.type_nat(), 0);
    EXPECT_TRUE(bb->free_vars().test(0));
    EXPECT_TRUE(w.lambda(w.type_nat(), bb)->free_vars().none());
    EXPECT_TRUE(w.pi(w.star(), w.pi(w.var(w.star(), 0), w.var(w.star(), 1)))->free_vars().none());

    // λT:*.λx:T.x
    auto T_1 = w.var(w.star(), 0, {"T"});
    EXPECT_TRUE(T_1->free_vars().test(0));
    EXPECT_FALSE(T_1->free_vars().test(1));
    auto T_2 = w.var(w.star(), 1, {"T"});
    EXPECT_TRUE(T_2->free_vars().test(1));
    EXPECT_TRUE(T_2->free_vars().any_begin(1));
    auto x = w.var(T_2, 0, {"x"});
    auto poly_id = w.lambda(T_2->type(), w.lambda(T_1, x));
    EXPECT_FALSE(poly_id->free_vars().test(0));
    EXPECT_FALSE(poly_id->free_vars().test(1));
    EXPECT_FALSE(poly_id->free_vars().any_end(2));
    EXPECT_FALSE(poly_id->free_vars().any_begin(0));

    // λx:w.type_nat().x
    auto int_id = w.app(poly_id, w.type_nat());
    EXPECT_EQ(int_id, w.lambda(w.type_nat(), w.var(w.type_nat(), 0)));
    EXPECT_EQ(w.app(int_id, n23), n23);
}

#if 0
TEST(Lambda, PolyIdPredicative) {
    World w;
    // λT:*.λx:T.x
    auto T_1 = w.var(w.star(), 0, {"T"});
    auto T_2 = w.var(w.star(), 1, {"T"});
    auto x = w.var(T_2, 0, {"x"});
    auto poly_id = w.lambda(T_2->type(), w.lambda(T_1, x))->as<Lambda>();

    EXPECT_FALSE(poly_id->domain()->assignable(poly_id->type()));
}
#endif

static const int test_num_vars = 1000;

TEST(Lambda, AppCurry) {
    World w;
    const Def* cur = w.val_nat_32();
    for (int i = 0; i < test_num_vars; ++i)
        cur = w.lambda(w.type_nat(), cur);

    for (int i = 0; i < test_num_vars; ++i)
        cur = w.app(cur, w.val_nat_64());

    EXPECT_EQ(cur, w.val_nat_32());
}

TEST(Lambda, AppArity) {
    World w;
    auto l = w.lambda(w.sigma(DefArray{test_num_vars, w.type_nat()}), w.val_nat_32());
    auto r = w.app(l, DefArray(test_num_vars, w.val_nat_64()));
    EXPECT_EQ(r, w.val_nat_32());
}

TEST(Lambda, EtaConversion) {
    World w;
    auto N = w.type_nat();

    auto f = w.axiom(w.pi(N, N), {"f"});
    EXPECT_EQ(w.lambda(N, w.app(f, w.var(N, 0))), f);

    auto g = w.axiom(w.pi(w.sigma({N, N}), N), {"g"});
    auto NxN = w.sigma({N, N});
    auto NxNxN = w.sigma({N, N, N});
    EXPECT_EQ(w.lambda(NxN, w.app(g, w.tuple({w.extract(w.var(NxN, 0), 0), w.extract(w.var(NxN, 0), 1)}))), g);
    EXPECT_NE(w.lambda(NxN, w.app(g, w.tuple({w.extract(w.var(NxN, 0), 1), w.extract(w.var(NxN, 0), 0)}))), g);
    EXPECT_NE(w.lambda(NxNxN, w.app(g, w.tuple({w.extract(w.var(NxNxN, 0), 0), w.extract(w.var(NxNxN, 0), 1)}))), g);

    auto h = w.axiom(w.pi(w.sigma({N, N, N}), N), {"h"});
    EXPECT_EQ(w.lambda(NxNxN, w.app(h, w.tuple({w.extract(w.var(NxNxN, 0), 0), w.extract(w.var(NxNxN, 0), 1), w.extract(w.var(NxNxN, 0), 2)}))), h);
    EXPECT_NE(w.lambda(NxNxN, w.app(h, w.tuple({w.extract(w.var(NxNxN, 0), 2), w.extract(w.var(NxNxN, 0), 1), w.extract(w.var(NxNxN, 0), 0)}))), h);
}
