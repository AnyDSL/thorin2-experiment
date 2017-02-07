#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/primop.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    //w.arity_kind()->dump();
    //w.arity(3)->dump();
    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    //p2_4->dump();
    //w.index(2, 1234567890)->dump();
    auto v = w.variadic(w.arity(5), w.type_nat());
    ASSERT_TRUE(w.dimension(v) == w.arity(5));
    ASSERT_TRUE(is_array(v));

    auto t = w.tuple({w.val_nat_2(), w.val_nat_4()});
    ASSERT_TRUE(t->type()->isa<Variadic>());

    // ΠT:*,a:N.Π(ptr[T,a], i:dim(T)).ptr[T.i,a]

    auto s2 = w.sigma({w.type_bool(), w.type_nat()});
    auto n2 = w.sigma_type(2)->set(0, w.type_bool())->set(1, w.type_nat());
    auto ps2 = w.axiom(w.type_ptr(s2, w.val_nat_2()), {"ptr_s2"});
    auto pn2 = w.axiom(w.type_ptr(n2, w.val_nat_4()), {"ptr_n2"});

    auto lea1 = LEA(w.op_lea(ps2, 1));
    auto lea2 = LEA(w.op_lea(pn2, 1));
    ASSERT_EQ(lea1.type(), w.type_ptr(w.type_nat(), w.val_nat_2()));
    ASSERT_EQ(lea2.type(), w.type_ptr(w.type_nat(), w.val_nat_4()));
    ASSERT_EQ(lea1.ptr_pointee(), s2);
    ASSERT_EQ(lea2.ptr_pointee(), n2);
    ASSERT_EQ(w.op_insert(w.tuple({w.val_bool_top(), w.val_nat_2()}), 1, w.val_nat_8())->type(), s2);

    auto list = w.axiom(w.pi(w.star(), w.star()), {"list"});
    list->type()->dump();
    auto lb = w.axiom(w.app(list, w.type_bool()), {"lb"});
    auto ln = w.axiom(w.app(list, w.type_nat() ), {"ln"});

    // ΠT:*.Π(Vi:dim(T).list[T.i],list[T])
    auto zip = w.axiom(w.pi(w.star(),
                            w.pi(w.variadic(w.dimension(w.var(w.star(), 0)), w.app(list, w.extract(w.var(w.star(), 1), w.var(w.dimension(w.var(w.star(), 1)), 0)))),
                                 w.app(list, w.var(w.star(), 1)))), {"zip"});
    ASSERT_EQ(w.app(w.app(zip, s2), {lb, ln})->type(), w.app(list, s2));

    // ΠT:*.Π(list[T],Vi:dim(T).list[T.i])
    auto rip = w.axiom(w.pi(w.star(),
                            w.pi(w.app(list, w.var(w.star(), 0)),
                                 w.variadic(w.dimension(w.var(w.star(), 1)), w.app(list, w.extract(w.var(w.star(), 2), w.var(w.dimension(w.var(w.star(), 2)), 0)))))), {"rip"});
    auto l = w.axiom(w.app(list, s2), {"l"});
    ASSERT_EQ(w.app(w.app(rip, s2), l)->type(), w.sigma({lb->type(), ln->type()}));
}
