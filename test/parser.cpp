#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Parser, Simple) {
    std::string str ="Π(*). Π(<0 : *>). <1 : *>";
    std::istringstream is(str);

    World world;
    Lexer lexer(is, "stdin");
    Parser parser(world, lexer);

    auto def = world.pi(world.star(), world.pi(world.var(world.star(), 0), world.var(world.star(), 1)));
    ASSERT_EQ(parser.parse_def(), def);
}
