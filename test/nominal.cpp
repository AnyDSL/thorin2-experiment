#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << (x->name() == "" ? #x : x->name()) << " <" << x->gid() << "> = " << x << ": " << x->type() << endl; }while(0)

TEST(Nominal, Sigma) {
    World w;
    auto nat = w.type_nat();
    auto nat2 = w.sigma_type(2, {"Nat x Nat"})->set(0, nat)->set(1, nat);
    ASSERT_TRUE(nat2->is_closed());
    ASSERT_TRUE(nat2->free_vars().none());
    ASSERT_EQ(w.pi(nat2, nat)->domain(),  nat2);

    auto n42 = w.lit_nat(42);
    ASSERT_FALSE(nat2->assignable(n42));
}

TEST(Nominal, SigmaFreeVars) {
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

TEST(Nominal, ReduceWithNominals) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto sig = w.sigma_type(1, {"sig"});
    auto v0 = w.var(star, 0);
    sig->set(0, v0);
    ASSERT_TRUE(sig->is_closed());
    ASSERT_TRUE(sig->free_vars().test(0));

    auto lam = w.lambda(star, w.tuple({sig, sig}));
    auto red = w.app(lam, nat);
    ASSERT_FALSE(red->isa<App>());
    ASSERT_EQ(w.extract(red, 0_s), w.extract(red, 1_s));

    auto sig2 = w.sigma({sig, sig});
    ASSERT_FALSE(sig2->has_error());
    auto lam2 = w.lambda(star, w.sigma({sig, sig}));
    auto red2 = w.app(lam2, nat);
    ASSERT_FALSE(red2->isa<App>());
    ASSERT_NE(red2->op(0), red2->op(1));
}

TEST(Nominal, PolymorphicListVariantNominal) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto cons_or_nil = w.variant(star, 2, {"cons_or_nil"});
    auto list = w.lambda(star, cons_or_nil);
    auto nil = w.unit();
    auto cons = w.sigma({w.var(star, 0), w.app(list, w.var(star, 1))});
    ASSERT_TRUE(cons->free_vars().any_end(1));
    cons_or_nil->set(0, nil);
    cons_or_nil->set(1, cons);
    print_value_type(cons);
    print_value_type(cons_or_nil);
    print_value_type(list);
    auto apped = w.app(list, nat);
    print_value_type(apped);
    print_value_type(apped->op(0));
    print_value_type(apped->op(1));
    ASSERT_EQ(apped->op(1)->op(0), nat);
}

TEST(Nominal, Nat) {
    World w;
    auto star = w.star();

    auto variant = w.variant(star, 2, {"Nat"});
    variant->set(0, w.sigma_type(0, {"0"}));
    auto succ = w.sigma_type(1, {"Succ"});
    succ->set(0, variant);
    variant->set(1, succ);
    print_value_type(variant);
    print_value_type(succ);
    // TODO asserts, methods on nat
}
