#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Parser, SimplePi) {
    std::string str ="Π(*). Π(<0 : *>). <1 : *>";
    std::istringstream is(str);

    World world;
    Lexer lexer(is, "stdin");
    Parser parser(world, lexer);

    auto def = world.pi(world.star(), world.pi(world.var(world.star(), 0), world.var(world.star(), 1)));
    ASSERT_EQ(parser.parse_def(), def);
}

TEST(Parser, SimpleLambda) {
    std::string str ="λ(*).λ(<0:*>).<0:<1:*>>";
    std::istringstream is(str);

    World world;
    Lexer lexer(is, "stdin");
    Parser parser(world, lexer);

    auto def = world.lambda(world.star(), world.lambda(world.var(world.star(), 0), world.var(world.var(world.star(), 1), 0)));
    ASSERT_EQ(parser.parse_def(), def);
}
