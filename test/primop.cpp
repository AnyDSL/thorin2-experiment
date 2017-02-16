#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Primop, Types) {
    World w;

    ASSERT_EQ(w.type_uo16(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_uo16(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_uw64(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));
    ASSERT_EQ(w.type_uw64(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));

    ASSERT_EQ(w.type_uo16(), w.type_i(Qualifier::u, iflags::uo, 16));
    ASSERT_EQ(w.type_uo16(), w.type_i(Qualifier::u, iflags::uo, 16));
    ASSERT_EQ(w.type_uw64(), w.type_i(Qualifier::u, iflags::uw, 64));
    ASSERT_EQ(w.type_uw64(), w.type_i(Qualifier::u, iflags::uw, 64));

    ASSERT_EQ(w.type_f64(), w.app(w.type_r(), {w.unlimited(), w.val_nat(int64_t(rflags::f)), w.val_nat_64()}));
    ASSERT_EQ(w.type_p16(), w.app(w.type_r(), {w.unlimited(), w.val_nat(int64_t(rflags::p)), w.val_nat_16()}));

    ASSERT_EQ(w.type_f64(), w.type_r(Qualifier::u, rflags::f, 64));
    ASSERT_EQ(w.type_p16(), w.type_r(Qualifier::u, rflags::p, 16));
}

TEST(Primop, Vals) {
    World w;
    ASSERT_EQ(w.val_sw16(23), w.val_sw16(23));
    ASSERT_EQ(w.val_uo32(23), w.val_uo32(23));
    ASSERT_EQ(w.val_sw16(23)->box().get_u16(), sw16(23));
    ASSERT_EQ(w.val_uo32(23)->box().get_u32(), uo32(23));
}

TEST(Primop, Arithop) {
    World w;

    auto a = w.op_iadd(w.val_uo32(23), w.val_uo32(42));
    ASSERT_EQ(a->type(), w.type_uo32());
}

TEST(Primop, Cmp) {
    World w;
}

TEST(Primop, Ptr) {
    World w;
}
