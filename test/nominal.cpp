#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/fe/parser.h"

using namespace thorin;

TEST(Nominal, Sigma) {
    World w;
    auto nat = w.type_nat();
    auto nat2 = w.sigma_type(2, {"Nat x Nat"})->set(0, nat)->set(1, nat);
    EXPECT_TRUE(nat2->free_vars().none());
    EXPECT_EQ(w.pi(nat2, nat)->domain(), nat2);

    auto n42 = w.lit_nat(42);
    EXPECT_FALSE(nat2->assignable(n42));
}

TEST(Nominal, Option) {
    World w;
    auto nat = w.type_nat();

    // could replace None by unit and make it structural
    auto none = w.sigma_type(0, {"None"});
    // auto some = w.lambda(w.pi(w.star(), w.star()), {"Some"});
    // some->set(w.sigma({w.var(0, w.star())}));
    auto option_nominal = w.lambda(w.pi(w.star(), w.star()), {"Option"})->set(w.variant({none, w.var(w.star(), 0)}));
    // option_nominal->set(w.variant({none, w.app(some, w.var(0, w.star()))}));
    auto app = w.app(option_nominal, nat);
    EXPECT_TRUE(app->isa<App>());
    EXPECT_EQ(app->op(1), nat);
    auto option = w.lambda(w.star(), w.variant({w.unit(), w.var(w.star(), 0)}), {"Option"});
    EXPECT_EQ(w.app(option, nat), w.variant({w.unit(), nat}));
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
    list->set(w.variant({nil, cons}));
    auto apped = w.app(list, nat);
    EXPECT_TRUE(apped->isa<App>());
    EXPECT_EQ(apped->op(1), nat);
}

TEST(Nominal, Nat) {
    World w;
    auto star = w.star();

    auto variant = w.variant(star, 2, {"Nat"});
    variant->set(0, w.sigma_type(0, {"0"}));
    auto succ = w.sigma_type(1, {"Succ"});
    succ->set(0, variant);
    variant->set(1, succ);
    // TODO asserts, methods on nat
}

TEST(Nominal, SigmaFreeVars) {
    World w;
    auto star = w.star();

    auto sig = w.sigma_type(3);
    auto v0 = w.var(star, 0);
    auto v1 = w.var(star, 1);
    auto v3 = w.var(star, 3);
    sig->set(0, v0)->set(1, v3)->set(2, v1);
    EXPECT_TRUE(sig->free_vars().test(0));
    EXPECT_FALSE(sig->free_vars().test(1));
    EXPECT_TRUE(sig->free_vars().test(2));
    EXPECT_TRUE(sig->free_vars().any());
    EXPECT_TRUE(sig->free_vars().any_begin(1));
    EXPECT_TRUE(sig->free_vars().none_begin(3));
}

TEST(Nominal, ReduceWithNominals) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto sig = w.sigma_type(1, {"sig"});
    auto v0 = w.var(star, 0);
    sig->set(0, v0);
    EXPECT_TRUE(sig->free_vars().test(0));

    auto lam = w.lambda(star, w.tuple({sig, sig}));
    auto red = w.app(lam, nat);
    EXPECT_FALSE(red->isa<App>());
    EXPECT_EQ(w.extract(red, 0_s), w.extract(red, 1_s));

    auto lam2 = w.lambda(star, w.sigma({sig, sig}));
    auto red2 = w.app(lam2, nat);
    EXPECT_FALSE(red2->isa<App>());
    EXPECT_NE(red2->op(0), red2->op(1));
}

TEST(Nominal, PolymorphicListVariantNominal) {
    World w;
    auto nat = w.type_nat();
    auto star = w.star();

    auto cons_or_nil = w.variant(star, 2, {"cons_or_nil"});
    auto list = w.lambda(star, cons_or_nil);
    auto nil = w.unit();
    auto cons = w.sigma({w.var(star, 0), w.app(list, w.var(star, 1))});
    EXPECT_TRUE(cons->free_vars().any_end(1));
    cons_or_nil->set(0, nil);
    cons_or_nil->set(1, cons);
    auto apped = w.app(list, nat);
    EXPECT_EQ(apped->op(1)->op(0), nat);
}

TEST(Nominal, Module) {
    World w;

    auto S = w.star();
    auto B = w.type_bool();
    auto N = w.type_nat();

    // M := λU:*. L := λT:*. [[T, U], {nil := [] | cons := [L T]}
    // M := λU:*. L := λT:*. [[T, 1], {nil := [] | cons := [L T]}

    auto L = w.lambda(w.pi(S, S), {"L"});
    auto T = L->param({"T"});
    auto l = w.sigma({w.sigma({T, w.var(S, 1)}),
                      w.variant({w.sigma_type(0_s, {"nil"}), w.sigma_type(1, {"cons"})->set(0, w.app(L, T))})});
    L->set(l);

    l->dump();
    L->dump();
    w.app(L, N)->dump();
    auto M = w.lambda(S, L);
    auto LNN = w.app(w.app(M, N), N);
    auto LBN = w.app(w.app(M, N), B);
    auto LNB = w.app(w.app(M, B), N);
    auto LBB = w.app(w.app(M, B), B);

    auto id = fe::parse(w, "λT:*. λx:T. x");

    w.extract(w.axiom(LNN, {"lnn"}), 0_u64)->type()->dump();
    w.extract(w.axiom(LNB, {"lnb"}), 0_u64)->type()->dump();
    w.extract(w.axiom(LBN, {"lbn"}), 0_u64)->type()->dump();
    w.extract(w.axiom(LBB, {"lbb"}), 0_u64)->type()->dump();
    w.app(w.app(id, w.sigma({N, N})), w.extract(w.axiom(LNN, {"lnn'"}), 0_u64));
    w.app(w.app(id, w.sigma({N, B})), w.extract(w.axiom(LNB, {"lnb'"}), 0_u64));
    w.app(w.app(id, w.sigma({B, N})), w.extract(w.axiom(LBN, {"lbn'"}), 0_u64));
    w.app(w.app(id, w.sigma({B, B})), w.extract(w.axiom(LBB, {"lbb'"}), 0_u64));

    EXPECT_EQ(w.extract(w.axiom(LNN), 0_u64)->type(), w.sigma({N, N}));
    EXPECT_EQ(w.extract(w.axiom(LNB), 0_u64)->type(), w.sigma({N, B}));
    EXPECT_EQ(w.extract(w.axiom(LBN), 0_u64)->type(), w.sigma({B, N}));
    EXPECT_EQ(w.extract(w.axiom(LBB), 0_u64)->type(), w.sigma({B, B}));
}
