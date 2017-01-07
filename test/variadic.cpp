#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    w.space()->dump();
    w.space(Qualifier::Affine)->dump();
    w.arity(3)->dump();
    w.arity(7, Qualifier::Affine)->dump();
    auto p2_4 = w.proj(2, 4);
    auto p2_4b = w.proj(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    p2_4->dump();
    w.proj(2, 1234567890)->dump();
    w.variadic_sigma(w.nat())->dump();
}
