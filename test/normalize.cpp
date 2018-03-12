#include "gtest/gtest.h"

#include "thorin/me/world.h"

namespace thorin {

TEST(BOp, ConstFolding) {
    World w;
    EXPECT_EQ(w.op<BOp::band>(w.lit_false(), w.lit_false()), w.lit_false());
    EXPECT_EQ(w.op<BOp::band>(w.lit_false(), w.lit_true()),  w.lit_false());
    EXPECT_EQ(w.op<BOp::band>(w.lit_true(),  w.lit_false()), w.lit_false());
    EXPECT_EQ(w.op<BOp::band>(w.lit_true(),  w.lit_true()),  w.lit_true());

    EXPECT_EQ(w.op<BOp::bor>(w.lit_false(), w.lit_false()), w.lit_false());
    EXPECT_EQ(w.op<BOp::bor>(w.lit_false(), w.lit_true()),  w.lit_true());
    EXPECT_EQ(w.op<BOp::bor>(w.lit_true(),  w.lit_false()), w.lit_true());
    EXPECT_EQ(w.op<BOp::bor>(w.lit_true(),  w.lit_true()),  w.lit_true());

    EXPECT_EQ(w.op<BOp::bxor>(w.lit_false(), w.lit_false()), w.lit_false());
    EXPECT_EQ(w.op<BOp::bxor>(w.lit_false(), w.lit_true()),  w.lit_true());
    EXPECT_EQ(w.op<BOp::bxor>(w.lit_true(),  w.lit_false()), w.lit_true());
    EXPECT_EQ(w.op<BOp::bxor>(w.lit_true(),  w.lit_true()),  w.lit_false());
}

TEST(BOp, Normalize) {
#if 0
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

    auto x = w.axiom(w.type_r(16), {"x"});
    EXPECT_FALSE(w.op<ROp::rmul>(x, w.lit_r(0._r16))->isa<Lit>());
    EXPECT_FALSE(w.op<ROp::rmul>(RFlags::nnan, x, w.lit_r(0._r16))->isa<Lit>());
    EXPECT_EQ(w.op<ROp::rmul>(RFlags::fast, w.lit_r(0._r16), x), w.lit_r(0._r16));
    EXPECT_EQ(w.op<ROp::rmul>(RFlags::fast, w.lit_r(-0._r16), x), w.lit_r(-0._r16));

    //auto m = w.axiom(w.type_mem(), {"m"});
    // TODO why are those things not equal?
    //EXPECT_EQ(w.op<MOp::sdiv>(m, l3, l0), w.tuple({m, w.bottom(w.type_i(8))}));
    //EXPECT_EQ(w.op<MOp::sdiv>(m, a, l0), w.bottom(w.type_i(8)));
#endif
}

}
