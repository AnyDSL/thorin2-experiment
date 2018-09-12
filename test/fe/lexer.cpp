#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/fe/lexer.h"

using namespace thorin;
using namespace thorin::fe;

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

TEST(Lexer, LocId) {
    std::istringstream is(" test  abc    def if  \nwhile   foo   ");
    Lexer lexer(is, "stdin");
    auto t1 = lexer.lex();
    auto t2 = lexer.lex();
    auto t3 = lexer.lex();
    auto t4 = lexer.lex();
    auto t5 = lexer.lex();
    auto t6 = lexer.lex();
    auto t7 = lexer.lex();
    std::ostringstream oss;
    thorin::streamf(oss, "{} {} {} {} {} {} {}", t1, t2, t3, t4, t5, t6, t7);
    EXPECT_EQ(oss.str(), "test abc def if while foo <eof>");
    EXPECT_EQ(t1.loc(), Loc("stdin", 1,  2, 1,  5));
    EXPECT_EQ(t2.loc(), Loc("stdin", 1,  8, 1, 10));
    EXPECT_EQ(t3.loc(), Loc("stdin", 1, 15, 1, 17));
    EXPECT_EQ(t4.loc(), Loc("stdin", 1, 19, 1, 20));
    EXPECT_EQ(t5.loc(), Loc("stdin", 2,  1, 2,  5));
    EXPECT_EQ(t6.loc(), Loc("stdin", 2,  9, 2, 11));
    EXPECT_EQ(t7.loc(), Loc("stdin", 2, 14, 2, 14));
}

TEST(Lexer, Literals) {
    std::string str = "1s8 1s16 1s32 1s64 1u8 1u16 1u32 1u64 1.0f16 1.0f32 1.0f64 +1s32 -1s32 0xFFs32 -0xFFs32 0o10s32 -0o10s32 0b10s32 -0b10s32 0ₐ 0₁";
    std::istringstream is(str, std::ios::binary);

    Lexer lexer(is, "stdin");

    constexpr int n = 22;
    Literal::Tag tags[n] = {
        Literal::Tag::Lit_s8,
        Literal::Tag::Lit_s16,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s64,

        Literal::Tag::Lit_u8,
        Literal::Tag::Lit_u16,
        Literal::Tag::Lit_u32,
        Literal::Tag::Lit_u64,

        Literal::Tag::Lit_f16,
        Literal::Tag::Lit_f32,
        Literal::Tag::Lit_f64,

        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_s32,
        Literal::Tag::Lit_arity,
        Literal::Tag::Lit_index, Literal::Tag::Lit_index_arity
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

        Box(f16(1.0)),
        Box(f32(1.0)),
        Box(f64(1.0)),

        Box(s32(1)),
        Box(s32(-1)),
        Box(s32(255)),
        Box(s32(-255)),
        Box(s32(8)),
        Box(s32(-8)),
        Box(s32(2)),
        Box(s32(-2)),
        Box(u64(0)),
        Box(u64(0)), Box(u64(1)),
    };

    for (int i = 0; i < n; i++) {
        auto tok = lexer.lex();
        EXPECT_TRUE(tok.isa(Token::Tag::Literal)) << "at index " << i;
        EXPECT_EQ(tok.literal().tag, tags[i]) << "at index " << i;
        EXPECT_EQ(tok.literal().box, boxes[i]) << "at index " << i;
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
