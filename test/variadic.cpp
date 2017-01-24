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
}
