#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variants, negative_test) {
    World w;
    auto variant = w.variant({w.nat(), w.boolean()});
    auto any_nat23 = w.any(variant, w.nat(23));
    auto handle_nat = w.lambda(w.nat(), w.var(w.nat(), 0));
    auto handle_bool = w.lambda(w.boolean(), w.nat(0));
    ASSERT_DEATH(w.match(any_nat23, {handle_bool, handle_nat}), ".*");
}
