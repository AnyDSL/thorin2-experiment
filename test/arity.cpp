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
    ASSERT_EQ(a1, w.sigma({a1, a1}));

    ASSERT_EQ(a3, w.sigma({a3, a1}));
    ASSERT_EQ(w.sigma({a2, a3}), w.sigma({a1, a2, a1, a3}));

    auto arity_dep = w.sigma({w.arity_kind(), w.variadic(w.var(w.arity_kind(), 0), a2)});
    arity_dep->dump();
    arity_dep->type()->dump();
}

TEST(Arity, Index) {
    World w;
    auto idx0 = w.index(1, 0);
    auto idx1_2 = w.index(2, 1);
    auto idx0_3 = w.index(3, 0);

    ASSERT_EQ(idx0, w.tuple({idx0, idx0}));
    ASSERT_EQ(idx1_2, w.tuple({idx1_2, idx0}));
    ASSERT_EQ(w.tuple({idx1_2, idx0_3}), w.tuple({idx0, idx1_2, idx0, idx0_3}));
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
    auto unit = w.unit();
    auto tuple0t = w.tuple(w.unit_kind(), {});

    ASSERT_EQ(tuple0t, w.pack({a0, a1, a2}, N));
    ASSERT_EQ(tuple0t, w.pack({a1, a0}, N));
    ASSERT_EQ(w.pack(a2, tuple0t), w.pack({a2, a0, a2}, N));
}
