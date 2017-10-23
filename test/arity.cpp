#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Arity, Sigma) {
    World w;
    auto a0 = w.arity(0);
    auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto a3 = w.arity(3);

    ASSERT_EQ(a0, w.sigma({a0, a0}));
    ASSERT_EQ(a0, w.sigma({a1, a0}));
    ASSERT_EQ(a0, w.sigma({a0, a1}));
    // ASSERT_EQ(a1, w.sigma({a1, a1}));

    // ASSERT_EQ(a3, w.sigma({a3, a1}));
    // ASSERT_EQ(w.sigma({a2, a3}), w.sigma({a1, a2, a1, a3}));

    ASSERT_EQ(w.star(), w.sigma({a2, w.sigma({a3, a2}), a3})->type());
}

TEST(Arity, DependentSigma) {
    World w;
    // auto a0 = w.arity(0);
    // auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto a3 = w.arity(3);

    auto arity_dep = w.sigma({a2, w.extract(w.tuple({a3, a2}), w.var(a2, 0))});
    arity_dep->dump();
    arity_dep->type()->dump();
    // TODO actual test on arity_dep
}

TEST(Arity, Variadic) {
    World w;
    auto a0 = w.arity(0);
    auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto N = w.type_nat();
    auto unit = w.unit();

    ASSERT_EQ(unit, w.variadic({a0, a1, a2}, N));
    ASSERT_EQ(unit, w.variadic({a1, a0}, N));
    ASSERT_EQ(w.unit_kind(), w.variadic({a0, a2}, w.arity_kind()));
    ASSERT_EQ(w.variadic(a2, w.unit_kind()), w.variadic({a2, a0, a2}, w.arity_kind()));
}

TEST(Arity, Pack) {
    World w;
    auto a0 = w.arity(0);
    auto a1 = w.arity(1);
    auto a2 = w.arity(2);
    auto N = w.type_nat();
    /*auto unit =*/ w.unit();
    auto tuple0t = w.tuple(w.unit_kind(), {});

    ASSERT_EQ(tuple0t, w.pack({a0, a1, a2}, N));
    ASSERT_EQ(tuple0t, w.pack({a1, a0}, N));
    ASSERT_EQ(w.pack(a2, tuple0t), w.pack({a2, a0, a2}, N));
}
