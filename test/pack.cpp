#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/primop.h"

using namespace thorin;

TEST(Pack, Multi) {
    World w;
    auto N = w.type_nat();

    auto p = w.pack({3, 8, 5}, w.val_nat_16());
    ASSERT_EQ(p, w.pack(3, w.pack(8, w.pack(5, w.val_nat_16()))));
    ASSERT_EQ(p->type(), w.variadic({3, 8, 5}, N));
    // TODO currently broken
    //ASSERT_EQ(w.extract(w.extract(w.extract(p, 1), 2), 3), w.val_nat_16());
}
