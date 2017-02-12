#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Primop, Types) {
    World w;

    ASSERT_EQ(w.type_auo16(), w.app(w.type_i(), {w.affine(),   w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_auo16(), w.app(w.type_i(), {w.affine(),   w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_ruw64(), w.app(w.type_i(), {w.relevant(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));
    ASSERT_EQ(w.type_ruw64(), w.app(w.type_i(), {w.relevant(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));

    ASSERT_EQ(w.type_auo16(), w.type_i(Qualifier::a, iflags::uo, 16));
    ASSERT_EQ(w.type_auo16(), w.type_i(Qualifier::a, iflags::uo, 16));
    ASSERT_EQ(w.type_ruw64(), w.type_i(Qualifier::r, iflags::uw, 64));
    ASSERT_EQ(w.type_ruw64(), w.type_i(Qualifier::r, iflags::uw, 64));

    ASSERT_EQ(w.type_rf64(), w.app(w.type_r(), {w.relevant(),  w.val_nat(int64_t(rflags::f)), w.val_nat_64()}));
    ASSERT_EQ(w.type_up16(), w.app(w.type_r(), {w.unlimited(), w.val_nat(int64_t(rflags::p)), w.val_nat_16()}));

    ASSERT_EQ(w.type_rf64(), w.type_r(Qualifier::r, rflags::f, 64));
    ASSERT_EQ(w.type_up16(), w.type_r(Qualifier::u, rflags::p, 16));
}

TEST(Primop, Vals) {
    World w;
    ASSERT_EQ(w.val_usw16(23), w.val_usw16(23));
    ASSERT_EQ(w.val_usw16(23)->box().get_u16(), sw16(23));
    ASSERT_NE(w.val_auo32(23), w.val_auo32(23));
    ASSERT_EQ(w.val_auo32(23)->box().get_u32(), uo32(23));
}

TEST(Primop, Arithop) {
    World w;

    auto a = w.op_iadd(w.val_uuo32(23), w.val_uuo32(42));
    ASSERT_EQ(a->type(), w.type_uuo32());
}

TEST(Primop, Cmp) {
    World w;
}

TEST(Primop, Ptr) {
    World w;
}
