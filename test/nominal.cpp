#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << (x->name() == "" ? #x : x->name()) << " <" << x->gid() << "> = " << x << ": " << x->type() << endl; }while(0)

TEST(Nominal, sigma) {
    World w;
    auto nat = w.type_nat();
    auto nat2 = w.sigma_type(2, {"Nat x Nat"});
    nat2->set(0, nat);
    nat2->set(1, nat);
    ASSERT_TRUE(nat->is_closed());
    ASSERT_TRUE(nat->free_vars().none());

    auto n42 = w.val_nat(42);
    ASSERT_DEATH(w.tuple(nat2, {n42}), ".*");
}

TEST(Nominal, sigma_free_vars) {
    World w;
    auto star = w.star();

    auto sig = w.sigma_type(3);
    auto v0 = w.var(star, 0);
    auto v1 = w.var(star, 1);
    auto v3 = w.var(star, 3);
    sig->set(0, v0);
    sig->set(1, v3);
    sig->set(2, v1);
    ASSERT_TRUE(sig->free_vars().test(0));
    ASSERT_FALSE(sig->free_vars().test(1));
    ASSERT_TRUE(sig->free_vars().test(2));
    ASSERT_TRUE(sig->free_vars().any());
    ASSERT_TRUE(sig->free_vars().any_begin(1));
    ASSERT_TRUE(sig->free_vars().none_begin(3));
}

TEST(Nominal, lambda_free_vars) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto lam = w.pi_lambda(w.pi(nat, nat), {"lam"});
    auto v0 = w.var(nat, 0);
    auto v1 = w.var(w.pi(nat, nat), 1);
    lam->set(w.app(v1, v0));
    ASSERT_TRUE(lam->free_vars().test(0));
    ASSERT_FALSE(lam->free_vars().any_begin(1));
}

TEST(Nominal, reduce_to_unique_nominals) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto sig = w.sigma_type(1, {"sig"});
    auto v0 = w.var(star, 0);
    sig->set(0, v0);

    auto lam = w.lambda(star, w.tuple({sig, sig}));
    auto red = w.app(lam, nat);
    ASSERT_EQ(red->op(0), red->op(1));
}

TEST(Nominal, polymorphic_list) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto list = w.pi_lambda(w.pi(star, star), {"list"});
    auto cons = w.sigma_type(2, {"cons"});
    cons->set(0, w.var(star, 0));
    cons->set(1, w.app(list, w.var(star, 1)));
    ASSERT_TRUE(cons->is_closed());
    ASSERT_TRUE(cons->free_vars().any_end(1));
    print_value_type(cons);
    auto nil = w.sigma_type(0, {"nil"});
    ASSERT_TRUE(nil->is_closed());
    print_value_type(nil);
    auto cons_or_nil = w.variant({cons, nil});
    list->set(cons_or_nil);
    ASSERT_TRUE(list->is_closed());
    print_value_type(list);
    auto apped = w.app(list, nat);
    print_value_type(apped);
    print_value_type(apped->op(0));
    print_value_type(apped->op(1));
    // TODO asserts
}

TEST(Nominal, nat) {
    World w;
    auto star = w.star();

    auto variant = w.variant(2, star, {"Nat"});
    variant->set(0, w.sigma_type(0, {"0"}));
    auto succ = w.sigma_type(1, {"Succ"});
    succ->set(0, variant);
    variant->set(1, succ);
    print_value_type(variant);
    print_value_type(succ);
    // TODO asserts, methods on nat
}
