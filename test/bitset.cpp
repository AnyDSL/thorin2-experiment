#include "gtest/gtest.h"

#include "thorin/util/bitset.h"

using thorin::BitSet;

TEST(Bitset, Misc) {
    BitSet b;
    b[3] = true;
    b[17] = true;
    ASSERT_TRUE(b[3]);
    ASSERT_TRUE(b.test(3));
    ASSERT_TRUE(b.test(17));
    ASSERT_TRUE(b[17]);
    ASSERT_EQ(b.count(), size_t(2));
}
