#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Pack, Multi) {
    World w;
    auto N = w.type_nat();
    auto n16 = w.val_nat_16();

    auto p = w.pack({3, 8, 5}, n16);
    ASSERT_EQ(w.pack(1, n16), n16);
    ASSERT_EQ(p, w.pack(3, w.pack(8, w.pack(5, n16))));
    ASSERT_EQ(p->type(), w.variadic({3, 8, 5}, N));
    ASSERT_EQ(w.pack(1, n16), n16);
    ASSERT_EQ(w.pack({3, 1, 4}, n16), w.pack({3, 4}, n16));
    ASSERT_EQ(w.extract(w.extract(w.extract(p, 1), 2), 3), n16);
}

TEST(Pack, Nested) {
    World w;
    auto A = w.arities();
    auto N = w.type_nat();

    ASSERT_EQ(w.pack(w.sigma({w.arity(3), w.arity(2)}), w.var(N, 1)),
              w.pack(3, w.pack(2, w.var(N, 2))));
    ASSERT_EQ(w.pack(w.variadic(w.arity(3), w.arity(2)), w.var(N, 1)),
              w.pack(2, w.pack(2, w.pack(2, w.var(N, 3)))));
    ASSERT_EQ(w.pack(w.variadic(3, w.var(A, 1)), N),
              w.pack(w.var(A, 0), w.pack(w.var(A, 1), w.pack(w.var(A, 2), N))));

    auto p = w.pack(w.variadic(3, w.arity(2)), w.var(w.variadic(3, w.arity(2)), 0));
    const Def* outer_tuples[2];
    for (int z = 0; z != 2; ++z) {
        const Def* inner_tuples[2];
        for (int y = 0; y != 2; ++y) {
            const Def* t[2];
            for (int x = 0; x != 2; ++x)
                t[x] = w.tuple({w.index(2, z), w.index(2, y), w.index(2, x)});
            inner_tuples[y] = w.tuple(t);
        }
        outer_tuples[z] = w.tuple(inner_tuples);
    }
    ASSERT_EQ(p, w.tuple(outer_tuples));
}

TEST(Pack, Nominal) {
    World w;
    auto N = w.type_nat();
    auto S = w.sigma_type(2)->set(0, N)->set(1, N);
    auto f = w.axiom(w.pi(w.arity(2), N), {"f"});
    ASSERT_EQ(w.pack_nominal_sigma(S, w.app(f, w.var(w.arity(2), 0))),
              w.tuple(S, {w.app(f, w.index(2, 0)), w.app(f, w.index(2, 1))}));
}
