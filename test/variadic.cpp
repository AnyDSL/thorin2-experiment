#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/primop.h"

using namespace thorin;

TEST(Variadic, Misc) {
    World w;
    w.arity_kind()->dump();
    w.arity(3)->dump();
    auto p2_4 = w.index(2, 4);
    auto p2_4b = w.index(2, 4);
    ASSERT_EQ(p2_4, p2_4b);
    p2_4->dump();
    w.index(2, 1234567890)->dump();
    auto v = w.variadic(w.arity(5), w.type_nat());
    ASSERT_TRUE(w.dimension(v) == w.arity(5));

    auto t = w.tuple({w.val_nat_2(), w.val_nat_4()});
    ASSERT_TRUE(t->type()->isa<Variadic>());

    // ΠT:*,a:N.Π(ptr[T,a], i:dim(T)).ptr[T.i,a]

    auto s2 = w.sigma({w.type_bool(), w.type_nat()});
    auto n2 = w.sigma_type(2)->set(0, w.type_bool())->set(1, w.type_nat());
    auto ps2 = w.axiom(w.type_ptr(s2, w.val_nat_2()), {"ptr_s2"});
    auto pn2 = w.axiom(w.type_ptr(n2, w.val_nat_4()), {"ptr_n2"});

    auto lea1 = LEA(w.op_lea(ps2, w.index(1, 2)));
    auto lea2 = LEA(w.op_lea(pn2, w.index(1, 2)));
    ASSERT_EQ(lea1.type(), w.type_ptr(w.type_nat(), w.val_nat_2()));
    ASSERT_EQ(lea2.type(), w.type_ptr(w.type_nat(), w.val_nat_4()));
    ASSERT_EQ(lea1.ptr_pointee(), s2);
    ASSERT_EQ(lea2.ptr_pointee(), n2);
    ASSERT_EQ(w.op_insert(w.tuple({w.val_bool_top(), w.val_nat_2()}), w.index(1, 2), w.val_nat_8())->type(), s2);

    auto list = w.axiom(w.pi(w.star(), w.star()), {"list"});
    list->type()->dump();
#if 0
    auto zip = w.axiom(
            w.pi({w.arity_kind(),
                  w.variadic_sigma(w.var(w.arity_kind(), 0), w.star())},
            w.pi( w.variadic_sigma(w.var(w.arity_kind(), 1), w.variadic_tuple(w.var(w.arity_kind(), 1), w.app(list, w.var(w.variadic_sigma(w.var(w.arity_kind(), 1), w.star()), 0)))),
                  w.app(list, w.var(w.variadic_sigma(w.var(w.arity_kind(), 2), w.star()), 1)))),
            {"zip"});
    zip->type()->dump();
    //w.app(zip, {w.arity(2), w.tuple(w.variadic_sigma(w.arity(2), w.star()), {w.type_nat(), w.type_bool()})})->dump();
#endif
}
