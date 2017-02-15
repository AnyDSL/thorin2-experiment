#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/frontend/lexer.h"

using namespace thorin;

TEST(Lexer, Tokens) {
    std::string str ="{ } ( ) < > [ ] : , . * #pi #sigma #lambda";
    std::istringstream is(str);

    Lexer lexer(is, "stdin");
    ASSERT_TRUE(lexer.next().isa(Token::Tag::L_Brace));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::R_Brace));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::L_Paren));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::R_Paren));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::L_Angle));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::R_Angle));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::L_Bracket));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::R_Bracket));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Colon));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Comma));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Dot));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Star));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Pi));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Sigma));
    ASSERT_TRUE(lexer.next().isa(Token::Tag::Lambda));
}
