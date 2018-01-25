#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO change/introduce tests when/if we introduce subkinding on qualifiers

TEST(Subtypes, Kinds) {
    World w;
    for (auto q1 : QualifierTags) {
        for (auto q2 : QualifierTags) {
            if (q1 == q2) {
                EXPECT_TRUE(w.arity_kind(q1)->subtype_of(w, w.star(q1)));
                EXPECT_TRUE(w.multi_arity_kind(q1)->subtype_of(w, w.star(q1)));
                EXPECT_TRUE(w.star(q1)->subtype_of(w, w.star(q1)));
            } else {
                EXPECT_FALSE(w.arity_kind(q2)->subtype_of(w, w.star(q1)));
                EXPECT_FALSE(w.multi_arity_kind(q2)->subtype_of(w, w.star(q1)));
            }
            EXPECT_FALSE(w.star(q1)->subtype_of(w, w.arity_kind(q2)));
            EXPECT_FALSE(w.star(q1)->subtype_of(w, w.multi_arity_kind(q2)));
        }
    }
}

TEST(Subtypes, Pi) {
    World w;
    for (auto q : QualifierTags) {
        auto pi_a = w.pi(w.star(), w.arity_kind(q));
        auto pi_m = w.pi(w.star(), w.multi_arity_kind(q));
        auto pi_s = w.pi(w.star(), w.star(q));
        EXPECT_TRUE(pi_a->subtype_of(w, pi_s));
        EXPECT_FALSE(pi_s->subtype_of(w, pi_a));
        EXPECT_TRUE(pi_m->subtype_of(w, pi_s));
        EXPECT_FALSE(pi_s->subtype_of(w, pi_m));
        EXPECT_TRUE(pi_s->subtype_of(w, pi_s));
    }
}
