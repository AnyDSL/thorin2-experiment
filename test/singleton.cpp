#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << (x->name() == "" ? #x : x->name()) << " <" << x->gid() << "> = " << x << ": " << x->type() << endl; }while(0)

TEST(Singleton, free_vars) {
    World w;
    auto star = w.star();

    auto single = w.singleton(w.var(w.var(star, 1), 0));
    ASSERT_TRUE(single->free_vars().test(0));
    ASSERT_TRUE(single->free_vars().test(1));
}
