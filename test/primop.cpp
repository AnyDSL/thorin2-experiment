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

TEST(Primop, Arithop) {
    World w;
    w.op<wadd>()->type()->dump();
    auto a = w.op<wadd>(w.val(23), w.val(42));
    a->dump();
    a->type()->dump();
    ASSERT_EQ(a->type(), (w.type_i(32)));


    auto m1 = w.tuple({w.tuple({w.val(0), w.val(1), w.val(2)}), w.tuple({w.val(3), w.val(4), w.val(5)})});
    auto m2 = w.tuple({w.tuple({w.val(6), w.val(7), w.val(8)}), w.tuple({w.val(9), w.val(10), w.val(11)})});
    auto p1 = w.pack(2, w.pack(3, w.val(23)));
    auto p2 = w.pack(2, w.pack(3, w.val(42)));
    p1->dump();
    w.op<wadd>(m1, m2)->dump();
    w.op<wadd>(m1, p2)->dump();
    w.op<wadd>(p1, m2)->dump();
    w.op<wadd>(p1, p2)->dump();

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
