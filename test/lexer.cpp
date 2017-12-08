#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/frontend/lexer.h"

using namespace thorin;

TEST(Lexer, Tokens) {
    std::string str ="{ } ( ) < > [ ] : , . * \\pi \\lambda";
    std::istringstream is(str, std::ios::binary);

    Lexer lexer(is, "stdin");
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::L_Brace));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::R_Brace));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::L_Paren));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::R_Paren));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::L_Angle));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::R_Angle));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::L_Bracket));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::R_Bracket));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Colon));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Comma));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Dot));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Star));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Pi));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Lambda));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Eof));
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
        ASSERT_TRUE(tok.isa(Token::Tag::Literal)) << "index " << i;
        ASSERT_TRUE(tok.literal().tag == tags[i]) << "index " << i;
        ASSERT_TRUE(tok.literal().box == boxes[i]) << "index " << i;
    }
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Eof));
}

TEST(Lexer, Utf8) {
    std::string str = u8"\ufeffΠ λ ℚ ℚₖ 𝔸 𝕄";
    std::istringstream is(str, std::ios::binary);

    Lexer lexer(is, "stdin");
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Pi));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Lambda));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Qualifier_Type));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Qualifier_Kind));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Arity_Kind));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Multi_Arity_Kind));
    ASSERT_TRUE(lexer.lex().isa(Token::Tag::Eof));
}

TEST(Lexer, Eof) {
    std::istringstream is("", std::ios::binary);

    Lexer lexer(is, "stdin");
    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(lexer.lex().isa(Token::Tag::Eof));
    }
}
