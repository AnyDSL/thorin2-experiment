#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Base, PolyId) {
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

TEST(Sigma, Normalization) {
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
              w.pi({N, N, B}, w.app(fNNBNI, {w.var(N, 2),          w.var(N, 1),        w.var(B, 0),        w.var(N, 4), I})));
    ASSERT_EQ(w.pi(sNNN,      w.app(fNNNNI, {w.extract(vNNN, 0_s), w.extract(vNNN, 1), w.extract(vNNN, 2), w.var(N, 7), I})),
              w.pi({N, N, N}, w.app(fNNNNI, {w.var(N, 2),          w.var(N, 1),        w.var(N, 0),        w.var(N, 4), I})));

    ASSERT_EQ(w.lambda(sNNB,      w.app(gNNBNI, {w.extract(vNNB, 0_s), w.extract(vNNB, 1), w.extract(vNNB, 2), w.var(N, 7), I})),
              w.lambda({N, N, B}, w.app(gNNBNI, {w.var(N, 2),          w.var(N, 1),        w.var(B, 0),        w.var(N, 4), I})));
    ASSERT_EQ(w.lambda(sNNN,      w.app(gNNNNI, {w.extract(vNNN, 0_s), w.extract(vNNN, 1), w.extract(vNNN, 2), w.var(N, 7), I})),
              w.lambda({N, N, N}, w.app(gNNNNI, {w.var(N, 2),          w.var(N, 1),        w.var(N, 0),        w.var(N, 4), I})));
}

TEST(Sigma, Unit) {
    World w;

    auto unit = w.unit();
    auto tuple0 = w.tuple0();
    ASSERT_EQ(tuple0->type(), unit);

    auto lam = w.lambda(unit, tuple0);
    ASSERT_EQ(lam->domain(), unit);
    ASSERT_TRUE(lam->domains().size() == 0);
    ASSERT_EQ(lam->type()->body(), unit);
    auto pi = w.pi(Defs({}), unit);
    ASSERT_EQ(pi, lam->type());
    auto apped = w.app(lam, tuple0);
    ASSERT_EQ(tuple0, apped);
}

TEST(Sigma, Assign) {
    World w;
    auto sig = w.sigma({w.star(), w.var(w.star(), 0)})->as<Sigma>();
    ASSERT_TRUE(sig->assignable({w.type_nat(), w.val_nat(42)}));
    ASSERT_FALSE(sig->assignable({w.type_nat(), w.val_bool_bot()}));
}

TEST(Sigma, ExtractAndSingleton) {
    World w;
    auto n23 = w.val_nat(23);
    auto n42 = w.val_nat(42);

    auto fst = w.lambda({w.type_nat(), w.type_nat()}, w.var(w.type_nat(), 1));
    auto snd = w.lambda({w.type_nat(), w.type_nat()}, w.var(w.type_nat(), 0));
    ASSERT_EQ(w.app(fst, {n23, n42}), w.val_nat(23));
    ASSERT_EQ(w.app(snd, {n23, n42}), w.val_nat(42));

    auto poly = w.axiom(w.pi(w.star(), w.star()), {"Poly"});
    auto sigma = w.sigma({w.star(), w.app(poly, w.var(w.star(), 0))}, {"sig"});
    auto sigma_val = w.axiom(sigma,{"val"});
    auto fst_sigma = w.extract(sigma_val, 0_s);
    ASSERT_EQ(fst_sigma->type(), w.star());
    auto snd_sigma = w.extract(sigma_val, 1);
    snd_sigma->type()->dump();
    auto single_sigma = w.singleton(sigma_val);
    std::cout << single_sigma << ": " << single_sigma->type() << std::endl;
    auto single_pi = w.singleton(w.axiom(w.pi(w.star(), w.star()), {"pival"}));
    std::cout << single_pi << ": " << single_pi->type() << std::endl;
}

static const int test_num_vars = 1000;

TEST(App, Curry) {
    World w;
    const Def* cur = w.val_nat_32();
    for (int i = 0; i < test_num_vars; ++i)
        cur = w.lambda(w.type_nat(), cur);

    for (int i = 0; i < test_num_vars; ++i)
        cur = w.app(cur, w.val_nat_64());

    ASSERT_EQ(cur, w.val_nat_32());
}

TEST(App, Arity) {
    World w;
    auto l = w.lambda(DefArray(test_num_vars, w.type_nat()), w.val_nat_32());
    auto r = w.app(l, DefArray(test_num_vars, w.val_nat_64()));
    ASSERT_EQ(r, w.val_nat_32());
}

int main(int argc, char** argv)  {
    ::testing::InitGoogleTest(&argc, argv);
    Log::set(Log::Error, &std::cout);
    return RUN_ALL_TESTS();
}
