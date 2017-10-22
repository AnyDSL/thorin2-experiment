#include "gtest/gtest.h"

#include "thorin/matchers/literals.h"

namespace thorin {

TEST(MatchLiterals, Int) {
    World w;

#if 0
    ASSERT_TRUE(Literal<uw32>(w.val(iflags::uw,     2048)).is(2048));
    ASSERT_TRUE(IntLiteral<sw16>(w.val(iflags::sw,     0)).is_zero());
    ASSERT_TRUE(IntLiteral<uo32>(w.val(iflags::uo,     0)).is_zero());
    ASSERT_TRUE(IntLiteral<sw32>(w.val(iflags::sw, -2048)).is_negative());
#endif
}

TEST(MatchLiterals, Real) {
    World w;

    // ASSERT_TRUE(RealLiteral(w.val_f32(23.0)).matches());
}

// TEST(Primop, Vals) {
//     World w;
//     ASSERT_EQ(w.val_sw16(23), w.val_sw16(23));
//     ASSERT_EQ(w.val_uo32(23), w.val_uo32(23));
//     ASSERT_EQ(w.val_sw16(23)->box().get_u16(), sw16(23));
//     ASSERT_EQ(w.val_uo32(23)->box().get_u32(), uo32(23));

//     ASSERT_EQ(w.val_f16(23._h), w.val_f16(23._h));
//     ASSERT_EQ(w.val_f32(23.f), w.val_f32(23.f));
//     ASSERT_EQ(w.val_f64(23.0), w.val_f64(23.0));
//     //ASSERT_EQ(w.val_f16(23._h)->box().get_r16(), r16(23));
//     ASSERT_EQ(w.val_f32(23.f)->box().get_r32(), r32(23));
//     ASSERT_EQ(w.val_f64(23.0)->box().get_r64(), r64(23));
// }

}
