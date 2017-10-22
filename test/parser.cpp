#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Parser, SimplePi) {
    World w;
    auto def = w.pi(w.star(), w.pi(w.var(w.star(), 0), w.var(w.star(), 1)));
    ASSERT_EQ(parse(w, "Π(T:*). Π(U:T). T"), def);
}

TEST(Parser, SimpleLambda) {
    World w;
    auto def = w.lambda(w.star(), w.lambda(w.var(w.star(), 0), w.var(w.var(w.star(), 1), 0)));
    ASSERT_EQ(parse(w, "λ(T:*). λ(x:T). x"), def);
}
