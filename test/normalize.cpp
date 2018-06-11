#include "test/util.h"

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

TEST(NOp, ConstFolding) {
    World w;
    EXPECT_EQ(w.op<NOp::nadd>(w.lit_nat( 2), w.lit_nat( 3)), w.lit_nat( 5));
    EXPECT_EQ(w.op<NOp::nadd>(w.lit_nat(-2), w.lit_nat( 3)), w.lit_nat( 1));
    EXPECT_EQ(w.op<NOp::nadd>(w.lit_nat( 2), w.lit_nat(-3)), w.lit_nat(-1));
    EXPECT_EQ(w.op<NOp::nadd>(w.lit_nat(-2), w.lit_nat(-3)), w.lit_nat(-5));

    EXPECT_EQ(w.op<NOp::nsub>(w.lit_nat( 2), w.lit_nat( 3)), w.lit_nat(-1));
    EXPECT_EQ(w.op<NOp::nsub>(w.lit_nat(-2), w.lit_nat( 3)), w.lit_nat(-5));
    EXPECT_EQ(w.op<NOp::nsub>(w.lit_nat( 2), w.lit_nat(-3)), w.lit_nat( 5));
    EXPECT_EQ(w.op<NOp::nsub>(w.lit_nat(-2), w.lit_nat(-3)), w.lit_nat( 1));

    EXPECT_EQ(w.op<NOp::nmul>(w.lit_nat( 2), w.lit_nat( 3)), w.lit_nat( 6));
    EXPECT_EQ(w.op<NOp::nmul>(w.lit_nat(-2), w.lit_nat( 3)), w.lit_nat(-6));
    EXPECT_EQ(w.op<NOp::nmul>(w.lit_nat( 2), w.lit_nat(-3)), w.lit_nat(-6));
    EXPECT_EQ(w.op<NOp::nmul>(w.lit_nat(-2), w.lit_nat(-3)), w.lit_nat( 6));

    EXPECT_EQ(w.op<NOp::ndiv>(w.lit_nat( 7), w.lit_nat( 2)), w.lit_nat( 3));
    EXPECT_EQ(w.op<NOp::ndiv>(w.lit_nat(-7), w.lit_nat( 2)), w.lit_nat(-3));
    EXPECT_EQ(w.op<NOp::ndiv>(w.lit_nat( 7), w.lit_nat(-2)), w.lit_nat(-3));
    EXPECT_EQ(w.op<NOp::ndiv>(w.lit_nat(-7), w.lit_nat(-2)), w.lit_nat( 3));

    EXPECT_EQ(w.op<NOp::nmod>(w.lit_nat( 7), w.lit_nat( 2)), w.lit_nat( 1));
    EXPECT_EQ(w.op<NOp::nmod>(w.lit_nat(-7), w.lit_nat( 2)), w.lit_nat(-1));
    EXPECT_EQ(w.op<NOp::nmod>(w.lit_nat( 7), w.lit_nat(-2)), w.lit_nat( 1));
    EXPECT_EQ(w.op<NOp::nmod>(w.lit_nat(-7), w.lit_nat(-2)), w.lit_nat(-1));
}

// TODO test normalizations and tuple folding

}
