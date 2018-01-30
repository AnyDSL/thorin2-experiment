#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

namespace thorin {

TEST(Cn, Simpel) {
    World w;
    auto C = w.cn_type(w.unit());
    auto k = w.cn(parse(w, "cn[nat, cn nat]")->as<CnType>());
    //auto x = k->param(0, {"x"});
    auto r = k->param(1, {"r"});
    auto t = w.cn(C, {"t"});
    auto f = w.cn(C, {"f"});
    auto n = w.cn(parse(w, "cn[nat]")->as<CnType>());
    k->br(w.lit_true(), t, f);
    t->jump(n, w.lit_nat_16());
    f->jump(n, w.lit_nat_32());
    n->jump(r, n->param());
}

}
