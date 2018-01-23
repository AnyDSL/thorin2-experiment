#include "gtest/gtest.h"

#include "thorin/core/world.h"

namespace thorin::core {

TEST(Primop, Types) {
    World w;

    ASSERT_EQ(w.type_i(16), w.app(w.type_i(), w.val_nat_16()));
    ASSERT_EQ(w.type_i(64), w.app(w.type_i(), w.val_nat_64()));

    ASSERT_EQ(w.type_r(64), w.app(w.type_r(), w.val_nat_64()));
    ASSERT_EQ(w.type_r(16), w.app(w.type_r(), w.val_nat_16()));
}

template<class T, class O, O o>
void test_fold(World& w, const Def* type) {
    auto a = w.op<o>(w.val(T(23)), w.val(T(42)));
    ASSERT_EQ(a->type(), w.app(type, w.val_nat(sizeof(T)*8)));
    ASSERT_EQ(a, w.val(T(65)));

    auto m1 = w.tuple({w.tuple({w.val( T(0)), w.val( T(1)), w.val( T(2))}),
                       w.tuple({w.val( T(3)), w.val( T(4)), w.val( T(5))})});
    auto m2 = w.tuple({w.tuple({w.val( T(6)), w.val( T(7)), w.val( T(8))}),
                       w.tuple({w.val( T(9)), w.val(T(10)), w.val(T(11))})});
    auto p1 = w.pack(2, w.pack(3, w.val(T(23))));
    auto p2 = w.pack(2, w.pack(3, w.val(T(42))));
    ASSERT_EQ(w.op<o>(m1, m2), w.tuple({w.tuple({w.val(T(6)),  w.val( T(8)), w.val(T(10))}),
                                        w.tuple({w.val(T(12)), w.val(T(14)), w.val(T(16))})}));
    ASSERT_EQ(w.op<o>(p1, m2), w.tuple({w.tuple({w.val(T(29)), w.val(T(30)), w.val(T(31))}),
                                        w.tuple({w.val(T(32)), w.val(T(33)), w.val(T(34))})}));
    ASSERT_EQ(w.op<o>(m1, p2), w.tuple({w.tuple({w.val(T(42)), w.val(T(43)), w.val(T(44))}),
                                        w.tuple({w.val(T(45)), w.val(T(46)), w.val(T(47))})}));
    ASSERT_EQ(w.op<o>(p1, p2), w.pack({2, 3}, w.val(T(65))));
}

TEST(Primop, ConstFolding) {
    World w;

#define CODE(T) test_fold<T, WArithop, wadd>(w, w.type_i());
    THORIN_U_TYPES(CODE)
#undef CODE
#define CODE(T) test_fold<T, RArithop, radd>(w, w.type_r());
    THORIN_R_TYPES(CODE)
#undef CODE

    ASSERT_EQ(w.op<ashr>(w.val(uint8_t(-1)), w.val(uint8_t(1))), w.val(uint8_t(-1)));
    ASSERT_EQ(w.op<lshr>(w.val(uint8_t(-1)), w.val(uint8_t(1))), w.val(uint8_t(127)));
}

#if 0
TEST(Primop, Cmp) {
    World w;
    auto x = w.op_icmp(irel::lt, w.val(iflags::so, 23), w.val(iflags::so, 42));
    x->dump();
}
#endif

TEST(Primop, Ptr) {
    World w;
    const Def* m = w.axiom(w.type_mem(), {"m"});
    auto e = w.op_enter(m);
    auto f = w.extract(e, 1);
    m = w.extract(e, 0_s);
    auto p1 = w.op_slot(w.type_r(32), f);
    auto p2 = w.op_slot(w.type_r(32), f);
    ASSERT_NE(p1, p2);
    ASSERT_EQ(p1->type(), w.type_ptr(w.type_r(32)));
    ASSERT_EQ(p2->type(), w.type_ptr(w.type_r(32)));
    auto s1 = w.op_store(m, p1, w.val(23.f));
    auto s2 = w.op_store(m, p1, w.val(23.f));
    ASSERT_NE(s1, s2);
    auto l1 = w.op_load(s1, p1);
    auto l2 = w.op_load(s1, p1);
    ASSERT_NE(l1, l2);
    ASSERT_EQ(w.extract(l1, 0_s)->type(), w.type_mem());
    ASSERT_EQ(w.extract(l1, 1_s)->type(), w.type_r(32));
}

}
