#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Nominal, Sigma) {
    World w;
    auto nat = w.type_nat();
    auto nat2 = w.sigma_type(2, {"Nat x Nat"})->set(w, 0, nat)->set(w, 1, nat);
    ASSERT_TRUE(nat2->free_vars().none());
    ASSERT_EQ(w.pi(nat2, nat)->domain(), nat2);

    auto n42 = w.lit_nat(42);
    ASSERT_FALSE(nat2->assignable(w, n42));
}

TEST(Nominal, Option) {
    World w;
    auto nat = w.type_nat();

    // could replace None by unit and make it structural
    auto none = w.sigma_type(0, {"None"});
    // auto some = w.lambda(w.pi(w.star(), w.star()), {"Some"});
    // some->set(w.sigma({w.var(0, w.star())}));
    auto option_nominal = w.lambda(w.pi(w.star(), w.star()), {"Option"})->set(w, w.variant({none, w.var(w.star(), 0)}));
    // option_nominal->set(w.variant({none, w.app(some, w.var(0, w.star()))}));
    auto app = w.app(option_nominal, nat);
    EXPECT_TRUE(app->isa<App>());
    EXPECT_EQ(app->op(1), nat);
    auto option = w.lambda(w.star(), w.variant({none, w.var(w.star(), 0)}), {"Option"});
    EXPECT_EQ(w.app(option, nat), w.variant({none, nat}));
}

TEST(Nominal, PolymorphicList) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    // could replace Nil by unit and make it structural
    auto nil = w.sigma_type(0, {"Nil"});
    auto list = w.lambda(w.pi(star, star), {"List"});
    EXPECT_TRUE(list->is_nominal());
    list->dump();
    auto app_var = w.app(list, w.var(star, 1));
    app_var->dump();
    EXPECT_TRUE(app_var->isa<App>());
    auto cons = w.sigma({w.var(star, 0), app_var});
    EXPECT_TRUE(cons->free_vars().any_end(1));
    list->set(w, w.variant({nil, cons}));
    auto apped = w.app(list, nat);
    EXPECT_TRUE(apped->isa<App>());
    EXPECT_EQ(apped->op(1), nat);
}

TEST(Nominal, Nat) {
    World w;
    auto star = w.star();

    auto variant = w.variant(star, 2, {"Nat"});
    variant->set(w, 0, w.sigma_type(0, {"0"}));
    auto succ = w.sigma_type(1, {"Succ"});
    succ->set(w, 0, variant);
    variant->set(w, 1, succ);
    // TODO asserts, methods on nat
}
