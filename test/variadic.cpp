#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    w.space()->dump();
    w.space(Qualifier::Affine)->dump();
    w.arity(3)->dump();
    w.arity(7, Qualifier::Affine)->dump();
    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    p2_4->dump();
    w.index(2, 1234567890)->dump();
    w.variadic_sigma(w.type_nat())->dump();
}
