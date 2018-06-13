#include "test/util.h"

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
    EXPECT_TRUE(b[3]);
    EXPECT_FALSE(b[17]);
    EXPECT_TRUE(b[65]);
    EXPECT_TRUE(b[2342]);
    EXPECT_FALSE(b[8197]);
    EXPECT_EQ(b.count(), size_t(3));
    EXPECT_TRUE(b.any());
    EXPECT_FALSE(b.none());
}

TEST(Bitset, SetClear) {
    BitSet b;
    b.set(3).set(17).set(65).set(2342).set(17).set(3).clear(17).toggle(3).toggle(300);
    EXPECT_FALSE(b.test(3));
    EXPECT_FALSE(b.test(17));
    EXPECT_TRUE(b.test(65));
    EXPECT_TRUE(b.test(300));
    EXPECT_TRUE(b.test(2342));
    EXPECT_FALSE(b.test(8197));
    EXPECT_EQ(b.count(), size_t(3));
    EXPECT_TRUE(b.any());
    EXPECT_FALSE(b.none());
    EXPECT_TRUE(b.none_range(66,300));
    EXPECT_FALSE(b.none_range(66,301));
}

auto test_range(int l_offset, int r_offset) {
    BitSet b;
    EXPECT_TRUE(b.none());
    EXPECT_FALSE(b.any());

    b.set(7 + l_offset);
    EXPECT_NE(7 + l_offset == 7 + r_offset, b.any_range(7 + l_offset, 7 + r_offset));
    EXPECT_TRUE(b.any_range(3 + l_offset, 10 + r_offset));
    b.clear(7 + l_offset);

    b.set(2 + l_offset);
    EXPECT_FALSE(b.any_range(3 + l_offset, 10 + r_offset));
    b.set(10 + r_offset);
    EXPECT_FALSE(b.any_range(3 + l_offset, 10 + r_offset));

    b.set(3 + l_offset);
    EXPECT_TRUE(b.any_range(3 + l_offset, 10 + r_offset));
    b.clear(3 + l_offset);

    b.set(9 + r_offset);
    EXPECT_TRUE(b.any_range(3 + l_offset, 10 + r_offset));
}

TEST(Bitset, Range) {
    test_range(  0,   0);
    test_range( 64,  64);
    test_range(128, 128);
    test_range( 0,  128);
    test_range(64,  128);
}

TEST(Bitset, Range2) {
    BitSet b;
    b.set(127);
    EXPECT_FALSE(b.any_range(128, 128));
}

TEST(Bitset, Or) {
    BitSet b, c;
    b.set(3).set(100);
    c.set(7);
    b |= c;
    EXPECT_TRUE(b.test(3));
    EXPECT_TRUE(b.test(7));
    EXPECT_TRUE(b.test(100));
    EXPECT_EQ(b.count(), size_t(3));
}

TEST(Bitset, ShiftWithin64) {
    BitSet b;
    b.set(23).set(100);
    b >>= 20;
    EXPECT_TRUE(b.test(3));
    EXPECT_TRUE(b.test(80));
    EXPECT_EQ(b.count(), size_t(2));
}

TEST(Bitset, ShiftOutside64) {
    BitSet b;
    b.set(23).set(100);
    b >>= 80;
    EXPECT_TRUE(b.test(20));
    EXPECT_EQ(b.count(), size_t(1));
}
