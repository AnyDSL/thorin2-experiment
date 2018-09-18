#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variants, negative_test) {
    World w;
    w.enable_expensive_checks();
    auto B = w.type_bool();
    auto N = w.type_nat();
    auto variant = w.variant(w.kind_star(), {N, B});
    auto x = w.axiom(variant, {"x"});
    auto handle_N = w.lambda(N, w.var(N, 0));
    auto handle_B_B = w.lambda(w.sigma({B, B}), w.lit_nat(0));
    EXPECT_THROW(w.match(x, {handle_B_B, handle_N}), TypeError);
}

TEST(Variants, positive_tests) {
    World w;
    auto B = w.type_bool();
    auto N = w.type_nat();
    // Test variant types and matches
    auto variant = w.variant(w.kind_star(), {N, B});
    auto x = w.axiom(variant, {"x"});
    auto assumed_var = w.axiom(variant, {"someval"});
    auto handle_N = w.lambda(N, w.var(N, 0));
    auto handle_B = w.lambda(B, w.axiom(N,{"0"}));
    Array<const Def*> handlers{handle_N, handle_B};
    auto match_N = w.match(x, handlers);
    match_N->dump(); // 23
    auto o_match_N = w.match(x, {handle_B, handle_N});
    EXPECT_EQ(match_N, o_match_N);
    auto match_B = w.match(x, handlers);
    match_B->dump(); // 0
    auto match = w.match(assumed_var, handlers);
    match->dump(); // match someval with ...
    match->type()->dump();
    // TODO don't want to allow this, does not have a real intersection interpretation, should be empty
    w.intersection(w.kind_star(), {w.pi(N, N), w.pi(B, B)})->dump();
}

TEST(Variants, identities) {
    World w;
    auto B = w.type_bool();
    auto N = w.type_nat();
    auto F = w.bot(w.kind_star());
    EXPECT_EQ(w.variant(w.kind_star(), {N}), N);
    EXPECT_EQ(w.variant(w.kind_star(), {N, F}), N);
    EXPECT_EQ(w.variant(w.kind_star(), {N, F, F}), N);
    EXPECT_EQ(w.variant(w.kind_star(), {F}), F);
    EXPECT_EQ(w.variant(w.kind_star(), {F, F}), F);
    EXPECT_EQ(w.variant(w.kind_star(), {N, B, F}), w.variant(w.kind_star(), {B, N}));
}
