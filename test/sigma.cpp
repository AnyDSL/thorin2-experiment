#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

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

TEST(Tuple, Error) {
    World w;

    auto tuple = w.tuple({w.error(w.star())});
    ASSERT_TRUE(tuple->has_error());
}

TEST(Sigma, LUB) {
    World w;
    auto s = w.sigma({w.arity(2), w.arity(3)});
    ASSERT_EQ(s->type(), w.arity_kind());
}
