#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << (x->name() == "" ? #x : x->name()) << " <" << x->gid() << "> = " << x << ": " << x->type() << endl; }while(0)

TEST(Nominal, Misc) {
    World w;
    auto Nat = w.nat();
    auto star = w.star();

    auto s1 = w.sigma_type(1, {"N"});
    auto v1 = w.var(star, 0);
    s1->set(0, v1);
    ASSERT_TRUE(s1->is_closed());
    ASSERT_TRUE(s1->free_vars().test(0));
    ASSERT_TRUE(s1->free_vars().any_end(1));
    ASSERT_TRUE(s1->free_vars().any_begin(0));
    print_value_type(s1);
    auto l1 = w.lambda(star, w.sigma({s1, s1}));
    print_value_type(l1);
    auto a1 = w.app(l1, Nat);
    print_value_type(a1);
    a1->ops().dump();
    ASSERT_NE(a1->op(0), a1->op(1));

    std::cout << "--- NominalTest Polymorphic List ---" << endl;
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
    std::cout << "app list Nat" << endl;
    auto apped = w.app(list, Nat);
    print_value_type(apped);
    print_value_type(apped->op(0));
    print_value_type(apped->op(1));

    std::cout << "--- NominalTest Variant ---" << endl;

    auto variant = w.variant(2, star, {"Nat"});
    variant->set(0, w.sigma_type(0, {"0"}));
    auto succ = w.sigma_type(1, {"Succ"});
    succ->set(0, variant);
    variant->set(1, succ);
    print_value_type(variant);
    print_value_type(succ);
}
