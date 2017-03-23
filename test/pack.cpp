#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/primop.h"

using namespace thorin;

TEST(Pack, Multi) {
#if 0
    World w;
    auto N = w.type_nat();
    auto n16 = w.val_nat_16();

    auto p = w.pack({3, 8, 5}, n16);
    ASSERT_EQ(p, w.pack(3, w.pack(8, w.pack(5, n16))));
    ASSERT_EQ(p->type(), w.variadic({3, 8, 5}, N));
    ASSERT_EQ(w.pack(1, n16), n16);
    ASSERT_EQ(w.pack({3, 1, 4}, n16), w.pack({3, 4}, n16));
    // TODO currently broken
    //ASSERT_EQ(w.extract(w.extract(w.extract(p, 1), 2), 3), n16);
#endif
}
