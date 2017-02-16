#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/primop.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    auto B = w.type_bool();
    auto N = w.type_nat();
    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    //p2_4->dump();
    //w.index(2, 1234567890)->dump();
    auto v = w.variadic(w.arity(5), N);
    ASSERT_TRUE(w.dim(v) == w.arity(5));
    ASSERT_TRUE(is_array(v));

    auto t = w.tuple({w.val_nat_2(), w.val_nat_4()});
    ASSERT_TRUE(t->type()->isa<Variadic>());

    // ΠT:*,a:N.Π(ptr[T,a], i:dim(T)).ptr[T.i,a]

    auto s2 = w.sigma({B, N});
    auto n2 = w.sigma_type(2)->set(0, B)->set(1, N);
    auto ps2 = w.axiom(w.type_ptr(s2, w.val_nat_2()), {"ptr_s2"});
    auto pn2 = w.axiom(w.type_ptr(n2, w.val_nat_4()), {"ptr_n2"});

    auto lea1 = LEA(w.op_lea(ps2, 1));
    auto lea2 = LEA(w.op_lea(pn2, 1));
    ASSERT_EQ(lea1.type(), w.type_ptr(N, w.val_nat_2()));
    ASSERT_EQ(lea2.type(), w.type_ptr(N, w.val_nat_4()));
    ASSERT_EQ(lea1.ptr_pointee(), s2);
    ASSERT_EQ(lea2.ptr_pointee(), n2);
    ASSERT_EQ(w.op_insert(w.tuple({w.val_bool_top(), w.val_nat_2()}), 1, w.val_nat_8())->type(), s2);

    auto list = w.axiom(w.pi(w.star(), w.star()), {"list"});
    list->type()->dump();
    auto lb = w.axiom(w.app(list, B), {"lb"});
    auto ln = w.axiom(w.app(list, N ), {"ln"});

    // ΠT:*.Π(Vi:dim(T).list[T.i],list[T])
    auto zip = w.axiom(w.pi(w.star(),
                            w.pi(w.variadic(w.dim(w.var(w.star(), 0)), w.app(list, w.extract(w.var(w.star(), 1), w.var(w.dim(w.var(w.star(), 1)), 0)))),
                                 w.app(list, w.var(w.star(), 1)))), {"zip"});
    ASSERT_EQ(w.app(w.app(zip, s2), {lb, ln})->type(), w.app(list, s2));
    ASSERT_EQ(w.app(w.app(zip, B), lb)->type(), w.app(list, B));

    // ΠT:*.Π(list[T],Vi:dim(T).list[T.i])
    auto rip = w.axiom(w.pi(w.star(),
                            w.pi(w.app(list, w.var(w.star(), 0)),
                                 w.variadic(w.dim(w.var(w.star(), 1)), w.app(list, w.extract(w.var(w.star(), 2), w.var(w.dim(w.var(w.star(), 2)), 0)))))), {"rip"});
    auto l = w.axiom(w.app(list, s2), {"l"});
    ASSERT_EQ(w.app(w.app(rip, s2), l)->type(), w.sigma({lb->type(), ln->type()}));

    ASSERT_EQ(w.pi(w.variadic(w.arity(3), N), B), w.pi({N, N, N}, B));
}
