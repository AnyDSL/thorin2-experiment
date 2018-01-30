#include "gtest/gtest.h"

#include "thorin/core/world.h"
#include "thorin/frontend/parser.h"

namespace thorin::core {

TEST(Cn, Simpel) {
    World w;
    auto C = w.cn_type(w.unit());
    auto k = w.cn(parse(w, "cn[int {32s64: nat}, cn int {32s64: nat}]")->as<CnType>());
    auto x = k->param(0, {"x"});
    auto r = k->param(1, {"r"});
    auto t = w.cn(C, {"t"});
    auto f = w.cn(C, {"f"});
    auto n = w.cn(parse(w, "cn[int {32s64: nat}]")->as<CnType>());
    k->br(w.op<ICmp::ugt>(x, w.lit_i(0_u32)), t, f);
    t->jump(n, w.lit_i(23_u32));
    f->jump(n, w.lit_i(42_u32));
    n->jump(r, n->param());

    w.make_external(k);
}

}
