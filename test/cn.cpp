#include "gtest/gtest.h"

#include "thorin/analyses/scope.h"
#include "thorin/me/world.h"
#include "thorin/fe/parser.h"

namespace thorin::me {

TEST(Cn, Simple) {
    World w;
    auto C = w.cn(w.unit());
    C->dump();
    auto k = w.lambda(fe::parse(w, "cn[int 32s64::nat, cn int 32s64::nat]")->as<Pi>(), {"k"});
    k->type()->dump();
    auto x = k->param(0, {"x"});
    auto r = k->param(1, {"r"});
    auto t = w.lambda(C, {"t"});
    auto f = w.lambda(C, {"f"});
    auto n = w.lambda(fe::parse(w, "cn[int 32s64::nat]")->as<Pi>());
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

    k->dump_rec();
}

TEST(Cn, Poly) {
    World w;
    auto k = w.lambda(fe::parse(w, "cn[T: *, T, cn T]")->as<Pi>(), {"k"});
    k->jump(k->param(2, {"ret"}), k->param(1, {"x"}));

    w.make_external(k);

    Scope scope(k);
    EXPECT_TRUE(scope.contains(k));
}

}
