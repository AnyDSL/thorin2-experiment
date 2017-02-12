#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Primop, Types) {
    World w;

    ASSERT_EQ(w.type_auo16(), w.app(w.type_int(), {w.affine(),   w.val_nat(int64_t(IFlags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_auo16(), w.app(w.type_int(), {w.affine(),   w.val_nat(int64_t(IFlags::uo)), w.val_nat_16()}));
    ASSERT_EQ(w.type_ruw64(), w.app(w.type_int(), {w.relevant(), w.val_nat(int64_t(IFlags::uw)), w.val_nat_64()}));
    ASSERT_EQ(w.type_ruw64(), w.app(w.type_int(), {w.relevant(), w.val_nat(int64_t(IFlags::uw)), w.val_nat_64()}));

    ASSERT_EQ(w.type_auo16(), w.type_int(Qualifier::a, IFlags::uo, 16));
    ASSERT_EQ(w.type_auo16(), w.type_int(Qualifier::a, IFlags::uo, 16));
    ASSERT_EQ(w.type_ruw64(), w.type_int(Qualifier::r, IFlags::uw, 64));
    ASSERT_EQ(w.type_ruw64(), w.type_int(Qualifier::r, IFlags::uw, 64));

    ASSERT_EQ(w.type_rf64(), w.app(w.type_real(), {w.relevant(),  w.val_nat(int64_t(RFlags::f)), w.val_nat_64()}));
    ASSERT_EQ(w.type_up16(), w.app(w.type_real(), {w.unlimited(), w.val_nat(int64_t(RFlags::p)), w.val_nat_16()}));

    ASSERT_EQ(w.type_rf64(), w.type_real(Qualifier::r, RFlags::f, 64));
    ASSERT_EQ(w.type_up16(), w.type_real(Qualifier::u, RFlags::p, 16));
}

TEST(Primop, Vals) {
    World w;
    ASSERT_EQ(w.val_usw16(23), w.val_usw16(23));
    ASSERT_EQ(w.val_usw16(23)->box().get_u16(), sw16(23));
    ASSERT_NE(w.val_auo32(23), w.val_auo32(23));
    ASSERT_EQ(w.val_auo32(23)->box().get_u32(), uo32(23));
}

TEST(Primop, Arithop) {
    World w;

    auto a = w.op_iadd_uuo32(w.val_uuo32(23), w.val_uuo32(42));
    a->dump();
    a->type()->dump();

    //auto s32w = w.type_int(32, IFlags::sw);
    //auto a = w.axiom(s32w, {"a"});
    //auto b = w.axiom(s32w, {"b"});
    //a->dump();
    //b->dump();
    //auto add = w.app(w.app(w.iadd(), {n32, w.nat(3)}), {a, b});
    //auto mul = w.app(w.app(w.imul(), {n32, w.nat(3)}), {a, b});
    //add->dump();
    //add->type()->dump();
    //mul->dump();
    //mul->type()->dump();
    //w.type_mem()->dump();

    //auto load = w.axiom(w.pi(w.star(), w.pi({w.type_mem(), w.type_ptr(w.var(w.star(), 1))}, w.sigma({w.type_mem(), w.var(w.star(), 3)}))), {"load"});
    //load->type()->dump();
    //auto x = w.axiom(w.type_ptr(w.type_nat()), {"x"});
    //auto m = w.axiom(w.type_mem(), {"m"});
    //auto nat_load = w.app(load, w.type_nat());
    //nat_load->dump();
    //nat_load->type()->dump();
    //auto p = w.app(nat_load, {m, x});
    //p->dump();
    //p->type()->dump();
}

TEST(Primop, Cmp) {
    World w;
}

TEST(Primop, Ptr) {
    World w;
    //auto s32w = w.type_int(32, IFlags::sw);
    //auto a = w.axiom(s32w, {"a"});
    //auto b = w.axiom(s32w, {"b"});
    //a->dump();
    //b->dump();
    //auto add = w.app(w.app(w.iadd(), {n32, w.nat(3)}), {a, b});
    //auto mul = w.app(w.app(w.imul(), {n32, w.nat(3)}), {a, b});
    //add->dump();
    //add->type()->dump();
    //mul->dump();
    //mul->type()->dump();
    //w.type_mem()->dump();

    //auto load = w.axiom(w.pi(w.star(), w.pi({w.type_mem(), w.type_ptr(w.var(w.star(), 1))}, w.sigma({w.type_mem(), w.var(w.star(), 3)}))), {"load"});
    //load->type()->dump();
    //auto x = w.axiom(w.type_ptr(w.type_nat()), {"x"});
    //auto m = w.axiom(w.type_mem(), {"m"});
    //auto nat_load = w.app(load, w.type_nat());
    //nat_load->dump();
    //nat_load->type()->dump();
    //auto p = w.app(nat_load, {m, x});
    //p->dump();
    //p->type()->dump();
}
