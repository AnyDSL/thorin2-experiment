#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO change/introduce tests when/if we introduce subkinding on qualifiers

TEST(Subtypes, Kinds) {
    World w;
    for (auto q1 : Qualifiers) {
        for (auto q2 : Qualifiers) {
            if (q1 == q2) {
                EXPECT_TRUE(w.arity_kind(q1)->subtype_of(w.star(q1)));
                EXPECT_TRUE(w.multi_kind(q1)->subtype_of(w.star(q1)));
                EXPECT_TRUE(w.star(q1)->subtype_of(w.star(q1)));
            } else {
                EXPECT_FALSE(w.arity_kind(q2)->subtype_of(w.star(q1)));
                EXPECT_FALSE(w.multi_kind(q2)->subtype_of(w.star(q1)));
            }
            EXPECT_FALSE(w.star(q1)->subtype_of(w.arity_kind(q2)));
            EXPECT_FALSE(w.star(q1)->subtype_of(w.multi_kind(q2)));
        }
    }
}

TEST(Subtypes, Pi) {
    World w;
    for (auto q : Qualifiers) {
        auto pi_a = w.pi(w.star(), w.arity_kind(q));
        auto pi_m = w.pi(w.star(), w.multi_kind(q));
        auto pi_s = w.pi(w.star(), w.star(q));
        EXPECT_TRUE(pi_a->subtype_of(pi_s));
        EXPECT_FALSE(pi_s->subtype_of(pi_a));
        EXPECT_TRUE(pi_m->subtype_of(pi_s));
        EXPECT_FALSE(pi_s->subtype_of(pi_m));
        EXPECT_TRUE(pi_s->subtype_of(pi_s));
    }
}

TEST(Subtypes, Sigma) {
    World w;
    for (auto q : Qualifiers) {
        auto am = w.sigma({w.arity_kind(q), w.multi_kind(q)});
        auto ms = w.sigma({w.multi_kind(q), w.star(q)});
        auto ss = w.variadic(2, w.star(q));
        EXPECT_TRUE(am->subtype_of(ms));
        EXPECT_FALSE(ms->subtype_of(am));
        EXPECT_TRUE(am->subtype_of(ss));
        EXPECT_FALSE(ss->subtype_of(am));
        EXPECT_TRUE(ms->subtype_of(ss));
        EXPECT_FALSE(ss->subtype_of(ms));
    }
}
