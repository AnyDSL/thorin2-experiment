#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    w.arity_kind()->dump();
    w.arity_kind(Qualifier::Affine)->dump();
    w.arity(3)->dump();
    w.arity(7, Qualifier::Affine)->dump();
    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    p2_4->dump();
    w.index(2, 1234567890)->dump();
    w.variadic(w.arity(5), w.type_nat())->dump();

    auto t = w.tuple({w.val_nat_2(), w.val_nat_4()});
    ASSERT_TRUE(t->type()->as<Variadic>()->is_array());

    //auto lea = w.axiom(w.pi(

    auto list = w.axiom(w.pi(w.star(), w.star()), {"list"});
    list->type()->dump();
#if 0
    auto zip = w.axiom(
            w.pi({w.arity_kind(),
                  w.variadic_sigma(w.var(w.arity_kind(), 0), w.star())},
            w.pi( w.variadic_sigma(w.var(w.arity_kind(), 1), w.variadic_tuple(w.var(w.arity_kind(), 1), w.app(list, w.var(w.variadic_sigma(w.var(w.arity_kind(), 1), w.star()), 0)))),
                  w.app(list, w.var(w.variadic_sigma(w.var(w.arity_kind(), 2), w.star()), 1)))),
            {"zip"});
    zip->type()->dump();
    //w.app(zip, {w.arity(2), w.tuple(w.variadic_sigma(w.arity(2), w.star()), {w.type_nat(), w.type_bool()})})->dump();
#endif
}
