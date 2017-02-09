#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Primop, Arithop) {
    World w;
    auto s32w = w.type_int(32, ITypeFlags::sw);
    auto a = w.axiom(s32w, {"a"});
    auto b = w.axiom(s32w, {"b"});
    a->dump();
    b->dump();
    //auto add = w.app(w.app(w.iadd(), {n32, w.nat(3)}), {a, b});
    //auto mul = w.app(w.app(w.imul(), {n32, w.nat(3)}), {a, b});
    //add->dump();
    //add->type()->dump();
    //mul->dump();
    //mul->type()->dump();
    w.type_mem()->dump();

    auto load = w.axiom(w.pi(w.star(), w.pi({w.type_mem(), w.type_ptr(w.var(w.star(), 1))}, w.sigma({w.type_mem(), w.var(w.star(), 3)}))), {"load"});
    load->type()->dump();
    auto x = w.axiom(w.type_ptr(w.type_nat()), {"x"});
    auto m = w.axiom(w.type_mem(), {"m"});
    auto nat_load = w.app(load, w.type_nat());
    nat_load->dump();
    nat_load->type()->dump();
    auto p = w.app(nat_load, {m, x});
    p->dump();
    p->type()->dump();
}
