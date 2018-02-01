#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/frontend/lexer.h"

using namespace thorin;

TEST(Lexer, Tokens) {
    std::string str ="{ } ( ) < > [ ] : , . * \\pi \\lambda cn bool";
    std::istringstream is(str, std::ios::binary);

    Lexer lexer(is, "stdin");
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::L_Brace));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::R_Brace));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::L_Paren));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::R_Paren));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::L_Angle));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::R_Angle));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::L_Bracket));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::R_Bracket));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Colon));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Comma));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Dot));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Star));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Pi));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Lambda));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Cn));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Bool));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Eof));
}

TEST(Lexer, Literals) {
    std::string str = "1s8 1s16 1s32 1s64 1u8 1u16 1u32 1u64 1.0r16 1.0r32 1.0r64 +1s32 -1s32 0xFFs32 -0xFFs32 0o10s32 -0o10s32 0b10s32 -0b10s32";
    std::istringstream is(str, std::ios::binary);

    Lexer lexer(is, "stdin");

    constexpr int n = 19;
    Literal::Tag tags[n] = {
        Literal::Tag::Lit_s8,
        Literal::Tag::Lit_s16,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s64,

        Literal::Tag::Lit_u8,
        Literal::Tag::Lit_u16,
        Literal::Tag::Lit_u32,
        Literal::Tag::Lit_u64,

        Literal::Tag::Lit_r16,
        Literal::Tag::Lit_r32,
        Literal::Tag::Lit_r64,

        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32
    };

    Box boxes[n] = {
        Box(s8(1)),
        Box(s16(1)),
        Box(s32(1)),
        Box(s64(1)),

        Box(u8(1)),
        Box(u16(1)),
        Box(u32(1)),
        Box(u64(1)),

        Box(r16(1.0)),
        Box(r32(1.0)),
        Box(r64(1.0)),

        Box(s32(1)),
        Box(s32(-1)),
        Box(s32(255)),
        Box(s32(-255)),
        Box(s32(8)),
        Box(s32(-8)),
        Box(s32(2)),
        Box(s32(-2))
    };

    for (int i = 0; i < n; i++) {
        auto tok = lexer.lex();
        EXPECT_TRUE(tok.isa(Token::Tag::Literal)) << "index " << i;
        EXPECT_TRUE(tok.literal().tag == tags[i]) << "index " << i;
        EXPECT_TRUE(tok.literal().box == boxes[i]) << "index " << i;
    }
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Eof));
}

TEST(Lexer, Utf8) {
    std::string str = u8"\ufeffΠ λ ℚ 𝔸 𝕄";
    std::istringstream is(str, std::ios::binary);

    Lexer lexer(is, "stdin");
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Pi));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Lambda));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Qualifier_Type));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Arity_Kind));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Multi_Arity_Kind));
    EXPECT_TRUE(lexer.lex().isa(Token::Tag::Eof));
}

TEST(Lexer, Eof) {
    std::istringstream is("", std::ios::binary);

    Lexer lexer(is, "stdin");
    for (int i = 0; i < 100; i++) {
        EXPECT_TRUE(lexer.lex().isa(Token::Tag::Eof));
    }
}
