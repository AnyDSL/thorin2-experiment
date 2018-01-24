#include "gtest/gtest.h"

#include "thorin/core/world.h"

namespace thorin::core {

TEST(Primop, Types) {
    World w;

    ASSERT_EQ(w.type_i(16), w.app(w.type_i(), w.lit_nat_16()));
    ASSERT_EQ(w.type_i(64), w.app(w.type_i(), w.lit_nat_64()));

    ASSERT_EQ(w.type_r(64), w.app(w.type_r(), w.lit_nat_64()));
    ASSERT_EQ(w.type_r(16), w.app(w.type_r(), w.lit_nat_16()));
}

template<class T, class O, O o>
void test_fold(World& w, const Def* type) {
    auto t = w.app(type, w.lit_nat(sizeof(T)*8));
    auto a = w.op<o>(w.lit(t, T(23)), w.lit(t, T(42)));
    ASSERT_EQ(a->type(), t);
    ASSERT_EQ(a, w.lit(t, T(65)));

    auto m1 = w.tuple({w.tuple({w.lit(t, T(0)), w.lit(t, T(1)), w.lit(t, T(2))}),
                       w.tuple({w.lit(t, T(3)), w.lit(t, T(4)), w.lit(t, T(5))})});
    auto m2 = w.tuple({w.tuple({w.lit(t, T(6)), w.lit(t, T(7)), w.lit(t, T(8))}),
                       w.tuple({w.lit(t, T(9)), w.lit(t,T(10)), w.lit(t, T(11))})});
    auto p1 = w.pack(2, w.pack(3, w.lit(t, T(23))));
    auto p2 = w.pack(2, w.pack(3, w.lit(t, T(42))));
    ASSERT_EQ(w.op<o>(m1, m2), w.tuple({w.tuple({w.lit(t, T(6)),  w.lit(t,  T(8)), w.lit(t, T(10))}),
                                        w.tuple({w.lit(t, T(12)), w.lit(t, T(14)), w.lit(t, T(16))})}));
    ASSERT_EQ(w.op<o>(p1, m2), w.tuple({w.tuple({w.lit(t, T(29)), w.lit(t, T(30)), w.lit(t, T(31))}),
                                        w.tuple({w.lit(t, T(32)), w.lit(t, T(33)), w.lit(t, T(34))})}));
    ASSERT_EQ(w.op<o>(m1, p2), w.tuple({w.tuple({w.lit(t, T(42)), w.lit(t, T(43)), w.lit(t, T(44))}),
                                        w.tuple({w.lit(t, T(45)), w.lit(t, T(46)), w.lit(t, T(47))})}));
    ASSERT_EQ(w.op<o>(p1, p2), w.pack({2, 3}, w.lit(t, T(65))));
}

TEST(Primop, ConstFolding) {
    World w;

#define CODE(T) test_fold<T, WOp, WOp::add>(w, w.type_i());
    THORIN_U_TYPES(CODE)
#undef CODE
#define CODE(T) test_fold<T, ROp, ROp::radd>(w, w.type_r());
    THORIN_R_TYPES(CODE)
#undef CODE

    ASSERT_EQ(w.op<IOp::ashr>(w.lit_i(u8(-1)), w.lit_i(1_u8)), w.lit_i(u8(-1)));
    ASSERT_EQ(w.op<IOp::lshr>(w.lit_i(u8(-1)), w.lit_i(1_u8)), w.lit_i(127_u8));

    ASSERT_EQ(w.op<WOp::add>(WFlags::nuw, w.lit_i(0xff_u8), w.lit_i(1_u8)), w.error(w.type_i(8)));
}

template<class T>
void test_icmp(World& w) {
    auto t = w.type_i(sizeof(T)*8);
    auto l23 = w.lit(t, T(-23));
    auto l42 = w.lit(t, T(42));
    auto lt = w.lit_true();
    auto lf = w.lit_false();

    ASSERT_EQ(w.op<ICmp::eq >(l23, l23), lt);
    ASSERT_EQ(w.op<ICmp::ne >(l23, l23), lf);
    ASSERT_EQ(w.op<ICmp::sge>(l23, l23), lt);
    ASSERT_EQ(w.op<ICmp::sgt>(l23, l23), lf);
    ASSERT_EQ(w.op<ICmp::sle>(l23, l23), lt);
    ASSERT_EQ(w.op<ICmp::slt>(l23, l23), lf);
    ASSERT_EQ(w.op<ICmp::uge>(l23, l23), lt);
    ASSERT_EQ(w.op<ICmp::ugt>(l23, l23), lf);
    ASSERT_EQ(w.op<ICmp::ule>(l23, l23), lt);
    ASSERT_EQ(w.op<ICmp::ult>(l23, l23), lf);

    ASSERT_EQ(w.op<ICmp::eq >(l23, l42), lf);
    ASSERT_EQ(w.op<ICmp::ne >(l23, l42), lt);
    ASSERT_EQ(w.op<ICmp::sge>(l23, l42), lf);
    ASSERT_EQ(w.op<ICmp::sgt>(l23, l42), lf);
    ASSERT_EQ(w.op<ICmp::sle>(l23, l42), lt);
    ASSERT_EQ(w.op<ICmp::slt>(l23, l42), lt);
    ASSERT_EQ(w.op<ICmp::uge>(l23, l42), lt);
    ASSERT_EQ(w.op<ICmp::ugt>(l23, l42), lt);
    ASSERT_EQ(w.op<ICmp::ule>(l23, l42), lf);
    ASSERT_EQ(w.op<ICmp::ult>(l23, l42), lf);

    ASSERT_EQ(w.op<ICmp::eq >(l42, l23), lf);
    ASSERT_EQ(w.op<ICmp::ne >(l42, l23), lt);
    ASSERT_EQ(w.op<ICmp::sge>(l42, l23), lt);
    ASSERT_EQ(w.op<ICmp::sgt>(l42, l23), lt);
    ASSERT_EQ(w.op<ICmp::sle>(l42, l23), lf);
    ASSERT_EQ(w.op<ICmp::slt>(l42, l23), lf);
    ASSERT_EQ(w.op<ICmp::uge>(l42, l23), lf);
    ASSERT_EQ(w.op<ICmp::ugt>(l42, l23), lf);
    ASSERT_EQ(w.op<ICmp::ule>(l42, l23), lt);
    ASSERT_EQ(w.op<ICmp::ult>(l42, l23), lt);
}

template<class T>
void test_rcmp(World& w) {
    auto t = w.type_r(sizeof(T)*8);
    auto l23 = w.lit(t, T(-23));
    auto l42 = w.lit(t, T(42));
    auto lt = w.lit_true();
    auto lf = w.lit_false();

    ASSERT_EQ(w.op<RCmp::oeq>(l23, l23), lt);
    ASSERT_EQ(w.op<RCmp::one>(l23, l23), lf);
    ASSERT_EQ(w.op<RCmp::oge>(l23, l23), lt);
    ASSERT_EQ(w.op<RCmp::ogt>(l23, l23), lf);
    ASSERT_EQ(w.op<RCmp::ole>(l23, l23), lt);
    ASSERT_EQ(w.op<RCmp::olt>(l23, l23), lf);

    ASSERT_EQ(w.op<RCmp::oeq>(l23, l42), lf);
    ASSERT_EQ(w.op<RCmp::one>(l23, l42), lt);
    ASSERT_EQ(w.op<RCmp::oge>(l23, l42), lf);
    ASSERT_EQ(w.op<RCmp::ogt>(l23, l42), lf);
    ASSERT_EQ(w.op<RCmp::ole>(l23, l42), lt);
    ASSERT_EQ(w.op<RCmp::olt>(l23, l42), lt);

    ASSERT_EQ(w.op<RCmp::oeq>(l42, l23), lf);
    ASSERT_EQ(w.op<RCmp::one>(l42, l23), lt);
    ASSERT_EQ(w.op<RCmp::oge>(l42, l23), lt);
    ASSERT_EQ(w.op<RCmp::ogt>(l42, l23), lt);
    ASSERT_EQ(w.op<RCmp::ole>(l42, l23), lf);
    ASSERT_EQ(w.op<RCmp::olt>(l42, l23), lf);
}

TEST(Primop, Cmp) {
    World w;
#define CODE(T) test_icmp<T>(w);
    THORIN_U_TYPES(CODE)
#undef CODE
#define CODE(T) test_rcmp<T>(w);
    THORIN_R_TYPES(CODE)
#undef CODE
}

TEST(Primop, Normalize) {
    World w;
    auto a = w.axiom(w.type_i(8), {"a"});
    auto b = w.axiom(w.type_i(8), {"b"});
    auto z = w.lit_i(0_u8);
    auto l2 = w.lit_i(2_u8);
    auto l3 = w.lit_i(3_u8);
    auto l5 = w.lit_i(5_u8);
    ASSERT_EQ(w.op<WOp::add>(a, z), a);
    ASSERT_EQ(w.op<WOp::add>(z, a), a);
    ASSERT_EQ(w.op<WOp::add>(a, a), w.op<WOp::mul>(a, l2));
    ASSERT_EQ(w.op<WOp::add>(a, b), w.op<WOp::add>(b, a));
    ASSERT_EQ(w.op<WOp::sub>(a, a), z);

    ASSERT_EQ(w.op<WOp::add>(w.op<WOp::add>(b, l3), a), w.op<WOp::add>(l3, w.op<WOp::add>(a, b)));
    ASSERT_EQ(w.op<WOp::add>(b, w.op<WOp::add>(a, l3)), w.op<WOp::add>(l3, w.op<WOp::add>(a, b)));
    ASSERT_EQ(w.op<WOp::add>(w.op<WOp::add>(b, l2), w.op<WOp::add>(a, l3)), w.op<WOp::add>(b, w.op<WOp::add>(a, l5)));
}

TEST(Primop, Ptr) {
    World w;
    const Def* m = w.axiom(w.type_mem(), {"m"});
    auto e = w.op_enter(m);
    auto f = w.extract(e, 1);
    m = w.extract(e, 0_u64);
    auto p1 = w.op_slot(w.type_r(32), f);
    auto p2 = w.op_slot(w.type_r(32), f);
    ASSERT_NE(p1, p2);
    ASSERT_EQ(p1->type(), w.type_ptr(w.type_r(32)));
    ASSERT_EQ(p2->type(), w.type_ptr(w.type_r(32)));
    auto s1 = w.op_store(m, p1, w.lit_r(23.f));
    //auto s2 = w.op_store(m, p1, w.lit_r(23.f));
    //ASSERT_NE(s1, s2);
    auto l1 = w.op_load(s1, p1);
    //auto l2 = w.op_load(s1, p1);
    //ASSERT_NE(l1, l2);
    ASSERT_EQ(w.extract(l1, 0_u64)->type(), w.type_mem());
    ASSERT_EQ(w.extract(l1, 1_u64)->type(), w.type_r(32));
}

}
