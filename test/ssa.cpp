#include "gtest/gtest.h"

#include "thorin/analyses/scope.h"
#include "thorin/fe/parser.h"
#include "thorin/core/world.h"
#include "thorin/core/transform/ssa_builder.h"

namespace thorin::core {

TEST(SSA, Simple) {
    World w;
    SSABuilder builder(w);
#if 0

    // create the four functions:
    auto main = world.lambda(world.fn_type(
                        {world.type_u32(), world.fn_type({world.type_u32()})}));
    // same as world.lambda() but this time if_then/if_else is "unsealed"
    auto if_then = world.basicblock();
    auto if_else = world.basicblock();
    auto next = world.lambda(world.fn_type({world.type_u32()}));

    // create comparison:
    auto cmp = world.relop_cmp_eq(main->param(0), world.literal_u32(0));

    // seal "basic blocks" as soon as possible,
    // i.e., you promise to not add additional predecessors later on
    main->branch(cmp, if_then, if_else);
    if_then->seal();
    if_else->seal();

    // We use handle '0' to represent variable 'a' in the original program.
    // Now let us create the assignments:
    if_then->set_value(0, world.literal_u32(23));
    if_else->set_value(0, world.literal_u32(42));

    // wire next
    if_then->jump(next, {});
    if_else->jump(next, {});
    next->seal();
    next->jump(main->param(1), {next->get_value(0, world.type_u32()}));
#endif
}

}
