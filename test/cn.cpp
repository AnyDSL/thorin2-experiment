#include "gtest/gtest.h"

#include "thorin/analyses/scope.h"
#include "thorin/core/world.h"
#include "thorin/frontend/parser.h"

namespace thorin::core {

TEST(Cn, Simple) {
    World w;
    auto C = w.cn_type(w.unit());
    auto k = w.cn(parse(w, "[int {32s64: nat}, cn int {32s64: nat}]"), {"k"});
    auto x = k->param(0, {"x"});
    auto r = k->param(1, {"r"});
    auto t = w.cn(C, {"t"});
    auto f = w.cn(C, {"f"});
    auto n = w.cn(parse(w, "[int {32s64: nat}]"));
    auto i0 = w.lit_i(0_u32);
    auto cmp = w.op<ICmp::ug>(x, i0);
    k->br(cmp, t, f);
    t->jump(n, w.lit_i(23_u32));
    f->jump(n, w.lit_i(42_u32));
    n->jump(r, n->param());

    w.make_external(k);

    Scope scope(k);
    EXPECT_TRUE(scope.contains(k));
    EXPECT_TRUE(scope.contains(t));
    EXPECT_TRUE(scope.contains(f));
    EXPECT_TRUE(scope.contains(n));
    EXPECT_TRUE(scope.contains(x));
    EXPECT_TRUE(scope.contains(r));
    EXPECT_TRUE(scope.contains(cmp));
    EXPECT_FALSE(scope.contains(i0));
}

TEST(Cn, Poly) {
    World w;
    auto k = w.cn(parse(w, "[T: *, T, cn T]"), {"k"});
    k->jump(k->param(2, {"ret"}), k->param(1, {"x"}));

    w.make_external(k);

    Scope scope(k);
    EXPECT_TRUE(scope.contains(k));
}

}
