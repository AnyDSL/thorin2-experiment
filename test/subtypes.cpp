#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO change/introduce tests when/if we introduce subkinding on qualifiers

TEST(Subtypes, Kinds) {
    World w;
    for (auto q1 : Qualifiers) {
        for (auto q2 : Qualifiers) {
            if (q1 == q2) {
                EXPECT_TRUE(w.kind_arity(q1)->subtype_of(w.kind_star(q1)));
                EXPECT_TRUE(w.kind_multi(q1)->subtype_of(w.kind_star(q1)));
                EXPECT_TRUE(w.kind_star (q1)->subtype_of(w.kind_star(q1)));
            } else {
                EXPECT_FALSE(w.kind_arity(q2)->subtype_of(w.kind_star(q1)));
                EXPECT_FALSE(w.kind_multi(q2)->subtype_of(w.kind_star(q1)));
            }
            EXPECT_FALSE(w.kind_star(q1)->subtype_of(w.kind_arity(q2)));
            EXPECT_FALSE(w.kind_star(q1)->subtype_of(w.kind_multi(q2)));
        }
    }
}

TEST(Subtypes, Pi) {
    World w;
    for (auto q : Qualifiers) {
        auto pi_a = w.pi(w.kind_star(), w.kind_arity(q));
        auto pi_m = w.pi(w.kind_star(), w.kind_multi(q));
        auto pi_s = w.pi(w.kind_star(), w.kind_star (q));
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
        auto am = w.sigma({w.kind_arity(q), w.kind_multi(q)});
        auto ms = w.sigma({w.kind_multi(q), w.kind_star (q)});
        auto ss = w.variadic(2, w.kind_star(q));
        EXPECT_TRUE(am->subtype_of(ms));
        EXPECT_FALSE(ms->subtype_of(am));
        EXPECT_TRUE(am->subtype_of(ss));
        EXPECT_FALSE(ss->subtype_of(am));
        EXPECT_TRUE(ms->subtype_of(ss));
        EXPECT_FALSE(ss->subtype_of(ms));
    }
}
