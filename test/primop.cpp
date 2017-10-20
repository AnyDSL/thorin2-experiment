#include "gtest/gtest.h"

#include "thorin/world.h"

namespace thorin {

TEST(Primop, Types) {
    World w;

    ASSERT_TRUE(w.is_primitive_type_constructor(w.type_i()));
    ASSERT_TRUE(w.is_primitive_type_constructor(w.type_r()));

    ASSERT_TRUE(w.is_primitive_type(w.type_bool()));
    ASSERT_TRUE(w.is_primitive_type(w.type_nat()));
    ASSERT_TRUE(w.is_primitive_type(w.type_uo16()));
    ASSERT_TRUE(w.is_primitive_type(w.type_uw32()));
    ASSERT_TRUE(w.is_primitive_type(w.type_f32()));
    ASSERT_TRUE(w.is_primitive_type(w.type_p64()));

    ASSERT_EQ(w.type_uo16(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_uo16(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_uw64(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));
    ASSERT_EQ(w.type_uw64(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));

    ASSERT_EQ(w.type_uo16(), w.type_i(Qualifier::u, iflags::uo, 16));
    ASSERT_EQ(w.type_uo16(), w.type_i(Qualifier::u, iflags::uo, 16));
    ASSERT_EQ(w.type_uw64(), w.type_i(Qualifier::u, iflags::uw, 64));
    ASSERT_EQ(w.type_uw64(), w.type_i(Qualifier::u, iflags::uw, 64));

    ASSERT_EQ(w.affine(), w.type_i(Qualifier::a, iflags::uw, 64)->qualifier());
    ASSERT_EQ(w.linear(), w.type_r(Qualifier::l, rflags::f, 64)->qualifier());

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

    ASSERT_EQ(w.val_f16(23._h), w.val_f16(23._h));
    ASSERT_EQ(w.val_f32(23.f), w.val_f32(23.f));
    ASSERT_EQ(w.val_f64(23.0), w.val_f64(23.0));
    //ASSERT_EQ(w.val_f16(23._h)->box().get_r16(), r16(23));
    ASSERT_EQ(w.val_f32(23.f)->box().get_r32(), r32(23));
    ASSERT_EQ(w.val_f64(23.0)->box().get_r64(), r64(23));
}

TEST(Primop, Arithop) {
    World w;
    auto a = w.op<IAdd>(w.val_uo32(23), w.val_uo32(42));
    ASSERT_EQ(a->type(), w.type_uo32());
}

TEST(Primop, Cmp) {
    World w;
}

TEST(Primop, Ptr) {
    World w;
    const Def* m = w.axiom(w.type_mem(), {"m"});
    auto e = w.op_enter(m);
    auto f = w.extract(e, 1);
    m = w.extract(e, 0_s);
    auto p1 = w.op_slot(w.type_p32(), f);
    auto p2 = w.op_slot(w.type_p32(), f);
    ASSERT_NE(p1, p2);
    ASSERT_EQ(p1->type(), w.type_ptr(w.type_p32()));
    ASSERT_EQ(p2->type(), w.type_ptr(w.type_p32()));
    auto s1 = w.op_store(m, p1, w.val_p32(23.f));
    auto s2 = w.op_store(m, p1, w.val_p32(23.f));
    ASSERT_NE(s1, s2);
    auto l1 = w.op_load(s1, p1);
    auto l2 = w.op_load(s1, p1);
    ASSERT_NE(l1, l2);
    ASSERT_EQ(w.extract(l1->type(), 0_s), w.type_mem());
    ASSERT_EQ(w.extract(l1->type(), 1_s), w.type_p32());
}

}
