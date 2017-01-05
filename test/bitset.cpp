#include "gtest/gtest.h"

#include "thorin/util/bitset.h"

using thorin::BitSet;

TEST(Bitset, IndexOperator) {
    BitSet b;
    b[3] = true;
    b[17] = true;
    b[65] = true;
    b[2342] = true;
    b[3] = true;
    b[17] = true;
    b[65] = true;
    b[2342] = true;
    b[17] = false;
    ASSERT_TRUE(b[3]);
    ASSERT_FALSE(b[17]);
    ASSERT_TRUE(b[65]);
    ASSERT_TRUE(b[2342]);
    ASSERT_FALSE(b[8197]);
    ASSERT_EQ(b.count(), size_t(3));
}

TEST(Bitset, SetClear) {
    BitSet b;
    b.set(3).set(17).set(65).set(2342).set(17).set(3).clear(17).toggle(3).toggle(300);
    ASSERT_FALSE(b.test(3));
    ASSERT_FALSE(b.test(17));
    ASSERT_TRUE(b.test(65));
    ASSERT_TRUE(b.test(300));
    ASSERT_TRUE(b.test(2342));
    ASSERT_FALSE(b.test(8197));
    ASSERT_EQ(b.count(), size_t(3));
}
