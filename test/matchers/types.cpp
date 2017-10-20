#include "gtest/gtest.h"

#include "thorin/matchers/types.h"

namespace thorin {

TEST(MatchTypes, Array) {
    World w;

    ASSERT_TRUE(ArrayType(w.type<iflags::uo, iwidth::w16>()).matches());
    ASSERT_TRUE(ArrayType(w.type<iflags::sw, iwidth::w32>()));

    ASSERT_TRUE(ArrayType(w.type<rflags::f, rwidth::w16>()).matches());
    ASSERT_TRUE(ArrayType(w.type<rflags::p, rwidth::w32>()));

    auto tf64 = w.type<rflags::f, rwidth::w64>();
    ASSERT_TRUE(ArrayType(w.variadic(4, tf64)).matches());
    ASSERT_TRUE(ArrayType(w.variadic(4, tf64)).is_variadic());
    ASSERT_TRUE(ArrayType(w.variadic(4, tf64)).is_const());
    ASSERT_EQ(w.arity(4), ArrayType(w.variadic(4, tf64)).arity());
    ASSERT_EQ(tf64, ArrayType(w.variadic(4, tf64)).elem_type());
    ASSERT_EQ(w.sigma({w.arity(4), w.arity(3)}), ArrayType(w.variadic(4, w.variadic(3, tf64))).arity());
    ASSERT_EQ((w.type<rflags::p, rwidth::w32>()), ArrayType(w.variadic(4, w.variadic(3, w.type<rflags::p, rwidth::w32>()))).elem_type());
    // TODO test with free variables
    ASSERT_TRUE(ArrayType(w.type<rflags::p, rwidth::w32>()));
}

// TEST(Primop, Types) {
//     World w;

//     ASSERT_EQ(w.type_uo16(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
//     ASSERT_EQ(w.type_uo16(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uo)), w.val_nat_16()}));
//     ASSERT_EQ(w.type_uw64(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));
//     ASSERT_EQ(w.type_uw64(), w.app(w.type_i(), {w.unlimited(), w.val_nat(int64_t(iflags::uw)), w.val_nat_64()}));

//     ASSERT_EQ(w.type_uo16(), w.type_i(Qualifier::u, iflags::uo, 16));
//     ASSERT_EQ(w.type_uo16(), w.type_i(Qualifier::u, iflags::uo, 16));
//     ASSERT_EQ(w.type_uw64(), w.type_i(Qualifier::u, iflags::uw, 64));
//     ASSERT_EQ(w.type_uw64(), w.type_i(Qualifier::u, iflags::uw, 64));

//     ASSERT_EQ(w.type_f64(), w.app(w.type_r(), {w.unlimited(), w.val_nat(int64_t(rflags::f)), w.val_nat_64()}));
//     ASSERT_EQ(w.type_p16(), w.app(w.type_r(), {w.unlimited(), w.val_nat(int64_t(rflags::p)), w.val_nat_16()}));

//     ASSERT_EQ(w.type_f64(), w.type_r(Qualifier::u, rflags::f, 64));
//     ASSERT_EQ(w.type_p16(), w.type_r(Qualifier::u, rflags::p, 16));
// }

// TEST(Primop, Ptr) {
//     World w;
//     const Def* m = w.axiom(w.type_mem(), {"m"});
//     auto e = w.op_enter(m);
//     auto f = w.extract(e, 1);
//     m = w.extract(e, 0_s);
//     auto p1 = w.op_slot(w.type_p32(), f);
//     auto p2 = w.op_slot(w.type_p32(), f);
//     ASSERT_NE(p1, p2);
//     ASSERT_EQ(p1->type(), w.type_ptr(w.type_p32()));
//     ASSERT_EQ(p2->type(), w.type_ptr(w.type_p32()));
//     auto s1 = w.op_store(m, p1, w.val_p32(23.f));
//     auto s2 = w.op_store(m, p1, w.val_p32(23.f));
//     ASSERT_NE(s1, s2);
//     auto l1 = w.op_load(s1, p1);
//     auto l2 = w.op_load(s1, p1);
//     ASSERT_NE(l1, l2);
//     ASSERT_EQ(w.extract(l1->type(), 0_s), w.type_mem());
//     ASSERT_EQ(w.extract(l1->type(), 1_s), w.type_p32());
// }

}
