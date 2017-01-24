#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    w.space()->dump();
    w.space(Qualifier::Affine)->dump();
    w.dimension(3)->dump();
    w.dimension(7, Qualifier::Affine)->dump();
    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    p2_4->dump();
    w.index(2, 1234567890)->dump();
    w.variadic_sigma(w.dimension(5), w.type_nat())->dump();
    auto t = w.variadic_tuple(w.dimension(23), w.val_nat_16());
    t->dump();
    t->type()->dump();

    //auto lea = w.axiom(w.pi(

#if 0
    auto list = w.axiom(w.pi(w.star(), w.star()), {"list"});
    auto zip = w.axiom(
            w.pi({w.space(), w.variadic_sigma(w.var(w.space(), 0), w.star())},
                w.pi({w.variadic_sigma(w.var(w.space(), 1), w.variadic_tuple(w.var(w.space(), 2), w.app(list, w.var(w.variadic_sigma(w.var(w.space(), 2), w.star()), 3))))},
                    w.app(list, w.var(w.variadic_sigma(w.var(w.space(), 3), w.star()), 4)))),
                    //w.app(list, w.var(w.variadic_sigma(w.var(w.space(), 2), w.star()), 3)))),
                    //w.unit())),
            {"zip"});
    zip->dump();
#endif
    //w.app(zip, {w.dimension(2), w.sigma({w.type_nat(), w.type_bool()})})->dump();
    //zip->
}
