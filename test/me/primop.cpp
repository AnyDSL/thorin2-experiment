#include "gtest/gtest.h"

#include "thorin/me/world.h"
#include "thorin/print.h"

namespace thorin::me {

TEST(Primop, Types) {
    World w;

    EXPECT_EQ(w.type_i(16), w.app(w.type_i(), w.lit_nat_16()));
    EXPECT_EQ(w.type_i(64), w.app(w.type_i(), w.lit_nat_64()));

    EXPECT_EQ(w.type_f(64), w.app(w.type_f(), w.lit_nat_64()));
    EXPECT_EQ(w.type_f(16), w.app(w.type_f(), w.lit_nat_16()));
}

template<class T, class O, O o>
void test_fold(World& w, const Def* type) {
    auto t = w.app(type, w.lit_nat(sizeof(T)*8));
    auto a = w.op<o>(w.lit(t, T(23)), w.lit(t, T(42)));
    EXPECT_EQ(a->type(), t);
    EXPECT_EQ(a, w.lit(t, T(65)));

    auto m1 = w.tuple({w.tuple({w.lit(t, T(0)), w.lit(t, T(1)), w.lit(t, T(2))}),
                       w.tuple({w.lit(t, T(3)), w.lit(t, T(4)), w.lit(t, T(5))})});
    auto m2 = w.tuple({w.tuple({w.lit(t, T(6)), w.lit(t, T(7)), w.lit(t, T(8))}),
                       w.tuple({w.lit(t, T(9)), w.lit(t,T(10)), w.lit(t, T(11))})});
    auto p1 = w.pack(2, w.pack(3, w.lit(t, T(23))));
    auto p2 = w.pack(2, w.pack(3, w.lit(t, T(42))));
    EXPECT_EQ(w.op<o>(m1, m2), w.tuple({w.tuple({w.lit(t, T(6)),  w.lit(t,  T(8)), w.lit(t, T(10))}),
                                        w.tuple({w.lit(t, T(12)), w.lit(t, T(14)), w.lit(t, T(16))})}));
    EXPECT_EQ(w.op<o>(p1, m2), w.tuple({w.tuple({w.lit(t, T(29)), w.lit(t, T(30)), w.lit(t, T(31))}),
                                        w.tuple({w.lit(t, T(32)), w.lit(t, T(33)), w.lit(t, T(34))})}));
    EXPECT_EQ(w.op<o>(m1, p2), w.tuple({w.tuple({w.lit(t, T(42)), w.lit(t, T(43)), w.lit(t, T(44))}),
                                        w.tuple({w.lit(t, T(45)), w.lit(t, T(46)), w.lit(t, T(47))})}));
    EXPECT_EQ(w.op<o>(p1, p2), w.pack({2, 3}, w.lit(t, T(65))));
}

TEST(Primop, ConstFolding) {
    World w;

#define CODE(T) test_fold<T, WOp, WOp::add>(w, w.type_i());
    THORIN_U_TYPES(CODE)
#undef CODE
#define CODE(T) test_fold<T, FOp, FOp::fadd>(w, w.type_f());
    THORIN_F_TYPES(CODE)
#undef CODE

    EXPECT_EQ(w.op<IOp::ashr>(w.lit_i(u8(-1)), w.lit_i(1_u8)), w.lit_i(u8(-1)));
    EXPECT_EQ(w.op<IOp::lshr>(w.lit_i(u8(-1)), w.lit_i(1_u8)), w.lit_i(127_u8));

    EXPECT_EQ(w.op<WOp::add>(WFlags::nuw, w.lit_i(0xff_u8), w.lit_i(1_u8)), w.bottom(w.type_i(8)));
}

template<class T>
void test_icmp(World& w) {
    auto t = w.type_i(sizeof(T)*8);
    auto l23 = w.lit(t, T(-23));
    auto l42 = w.lit(t, T(42));
    auto lt = w.lit_true();
    auto lf = w.lit_false();

    EXPECT_EQ(w.op<ICmp::e  >(l23, l23), lt);
    EXPECT_EQ(w.op<ICmp::ne >(l23, l23), lf);
    EXPECT_EQ(w.op<ICmp::sg >(l23, l23), lf);
    EXPECT_EQ(w.op<ICmp::sge>(l23, l23), lt);
    EXPECT_EQ(w.op<ICmp::sl >(l23, l23), lf);
    EXPECT_EQ(w.op<ICmp::sle>(l23, l23), lt);
    EXPECT_EQ(w.op<ICmp::ug >(l23, l23), lf);
    EXPECT_EQ(w.op<ICmp::uge>(l23, l23), lt);
    EXPECT_EQ(w.op<ICmp::ul> (l23, l23), lf);
    EXPECT_EQ(w.op<ICmp::ule>(l23, l23), lt);

    EXPECT_EQ(w.op<ICmp::e  >(l23, l42), lf);
    EXPECT_EQ(w.op<ICmp::ne >(l23, l42), lt);
    EXPECT_EQ(w.op<ICmp::sg >(l23, l42), lf);
    EXPECT_EQ(w.op<ICmp::sge>(l23, l42), lf);
    EXPECT_EQ(w.op<ICmp::sl >(l23, l42), lt);
    EXPECT_EQ(w.op<ICmp::sle>(l23, l42), lt);
    EXPECT_EQ(w.op<ICmp::ug >(l23, l42), lt);
    EXPECT_EQ(w.op<ICmp::uge>(l23, l42), lt);
    EXPECT_EQ(w.op<ICmp::ul >(l23, l42), lf);
    EXPECT_EQ(w.op<ICmp::ule>(l23, l42), lf);

    EXPECT_EQ(w.op<ICmp::e  >(l42, l23), lf);
    EXPECT_EQ(w.op<ICmp::ne >(l42, l23), lt);
    EXPECT_EQ(w.op<ICmp::sg >(l42, l23), lt);
    EXPECT_EQ(w.op<ICmp::sge>(l42, l23), lt);
    EXPECT_EQ(w.op<ICmp::sl >(l42, l23), lf);
    EXPECT_EQ(w.op<ICmp::sle>(l42, l23), lf);
    EXPECT_EQ(w.op<ICmp::ug >(l42, l23), lf);
    EXPECT_EQ(w.op<ICmp::uge>(l42, l23), lf);
    EXPECT_EQ(w.op<ICmp::ul >(l42, l23), lt);
    EXPECT_EQ(w.op<ICmp::ule>(l42, l23), lt);
}

template<class T>
void test_fcmp(World& w) {
    auto t = w.type_f(sizeof(T)*8);
    auto l23 = w.lit(t, T(-23));
    auto l42 = w.lit(t, T(42));
    auto lt = w.lit_true();
    auto lf = w.lit_false();

    EXPECT_EQ(w.op<FCmp:: e>(l23, l23), lt);
    EXPECT_EQ(w.op<FCmp::ne>(l23, l23), lf);
    EXPECT_EQ(w.op<FCmp::ge>(l23, l23), lt);
    EXPECT_EQ(w.op<FCmp:: g>(l23, l23), lf);
    EXPECT_EQ(w.op<FCmp::le>(l23, l23), lt);
    EXPECT_EQ(w.op<FCmp:: l>(l23, l23), lf);

    EXPECT_EQ(w.op<FCmp:: o>(l23, l42), lt);
    EXPECT_EQ(w.op<FCmp::ne>(l23, l42), lt);
    EXPECT_EQ(w.op<FCmp:: g>(l23, l42), lf);
    EXPECT_EQ(w.op<FCmp::ge>(l23, l42), lf);
    EXPECT_EQ(w.op<FCmp:: l>(l23, l42), lt);
    EXPECT_EQ(w.op<FCmp::le>(l23, l42), lt);

    EXPECT_EQ(w.op<FCmp:: e>(l42, l23), lf);
    EXPECT_EQ(w.op<FCmp::ne>(l42, l23), lt);
    EXPECT_EQ(w.op<FCmp:: g>(l42, l23), lt);
    EXPECT_EQ(w.op<FCmp::ge>(l42, l23), lt);
    EXPECT_EQ(w.op<FCmp:: l>(l42, l23), lf);
    EXPECT_EQ(w.op<FCmp::le>(l42, l23), lf);
}

TEST(Primop, Cmp) {
    World w;
#define CODE(T) test_icmp<T>(w);
    THORIN_U_TYPES(CODE)
#undef CODE
#define CODE(T) test_fcmp<T>(w);
    THORIN_F_TYPES(CODE)
#undef CODE
}

TEST(Primop, Cast) {
    World w;
    auto x = w.op<Cast::fcast>(16, w.lit_f(23.f));
    auto y = w.op<Cast::f2s>(8, w.lit_f(-1.f));
    x->dump(); x->type()->dump();
    y->dump(); y->type()->dump();
}

TEST(Primop, Normalize) {
    World w;
    auto a = w.axiom(w.type_i(8), {"a"});
    auto b = w.axiom(w.type_i(8), {"b"});
    auto l0 = w.lit_i(0_u8), l2 = w.lit_i(2_u8), l3 = w.lit_i(3_u8), l5 = w.lit_i(5_u8);

    auto add = [&] (auto a, auto b) { return w.op<WOp::add>(a, b); };
    auto sub = [&] (auto a, auto b) { return w.op<WOp::sub>(a, b); };
    auto mul = [&] (auto a, auto b) { return w.op<WOp::mul>(a, b); };

    EXPECT_EQ(add(a, l0), a);
    EXPECT_EQ(add(l0, a), a);
    EXPECT_EQ(add(a, a), mul(a, l2));
    EXPECT_EQ(add(a, b), add(b, a));
    EXPECT_EQ(sub(a, a), l0);

    EXPECT_EQ(add(add(b, l3), a), add(l3, add(a, b)));
    EXPECT_EQ(add(b, add(a, l3)), add(l3, add(a, b)));
    EXPECT_EQ(add(add(b, l2), add(a, l3)), add(b, add(a, l5)));
    EXPECT_EQ(add(add(b, l2), add(a, l3)), add(b, add(a, l5)));

    auto x = w.axiom(w.type_f(16), {"x"});
    EXPECT_FALSE(w.op<FOp::fmul>(x, w.lit_f(0._f16))->isa<Lit>());
    EXPECT_FALSE(w.op<FOp::fmul>(FFlags::nnan, x, w.lit_f(0._f16))->isa<Lit>());
    EXPECT_EQ(w.op<FOp::fmul>(FFlags::fast, w.lit_f( 0._f16), x), w.lit_f( 0._f16));
    EXPECT_EQ(w.op<FOp::fmul>(FFlags::fast, w.lit_f(-0._f16), x), w.lit_f(-0._f16));

    //auto m = w.axiom(w.type_mem(), {"m"});
    // TODO why are those things not equal?
    //EXPECT_EQ(w.op<MOp::sdiv>(m, l3, l0), w.tuple({m, w.bottom(w.type_i(8))}));
    //EXPECT_EQ(w.op<MOp::sdiv>(m, a, l0), w.bottom(w.type_i(8)));
}

TEST(Primop, Ptr) {
    World w;
    const Def* m = w.axiom(w.type_mem(w.lit_nat_0()), {"m"});
    auto e = w.op_enter(m);
    auto f = w.extract(e, 1);
    m = w.extract(e, 0_u64);
    auto p1 = w.op_slot(w.type_f(32), f);
    auto p2 = w.op_slot(w.type_f(32), f);
    EXPECT_NE(p1, p2);
    EXPECT_EQ(p1->type(), w.type_ptr(w.type_f(32)));
    EXPECT_EQ(p2->type(), w.type_ptr(w.type_f(32)));
    auto s1 = w.op_store(m, p1, w.lit_f(23.f));
    //auto s2 = w.op_store(m, p1, w.lit_f(23.f));
    //EXPECT_NE(s1, s2);
    auto l1 = w.op_load(s1, p1);
    //auto l2 = w.op_load(s1, p1);
    //EXPECT_NE(l1, l2);
    //EXPECT_EQ(w.extract(l1, 0_u64)->type(), w.type_mem());
    EXPECT_EQ(w.extract(l1, 1_u64)->type(), w.type_f(32));
}

TEST(Primop, Print2) {
    World w;

    auto a = w.axiom(w.type_i(8), {"a"});
    auto b = w.axiom(w.type_i(8), {"b"});
    auto c = w.axiom(w.type_i(8), {"c"});
    auto d = w.axiom(w.type_i(8), {"d"});
    auto _1 = w.op<WOp::mul>( b,  c);
    auto _2 = w.op<WOp::shl>( a, _1);
    auto _3 = w.op<WOp::sub>(_1,  d);
    auto _4 = w.op<WOp::add>(_2, _3);

    print(_4);
}

}
