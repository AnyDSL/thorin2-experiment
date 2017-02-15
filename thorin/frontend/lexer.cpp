#include <cctype>

#include "thorin/util/log.h"
#include "thorin/frontend/lexer.h"

namespace thorin {

Lexer::Lexer(std::istream& is, const std::string& name)
    : stream_(is)
    , filename_(name)
    , line_(1), col_(1)
{}

Token Lexer::next() {
    eat_spaces();

    auto front_line = line_;
    auto front_col = col_;
    auto make_loc = [&] () {
        return Location(filename_.c_str(), front_line, front_col, line_, col_);
    };

    if (stream_.peek() == std::istream::traits_type::eof())
        return Token(make_loc(), Token::Tag::Eof);

    if (accept('{')) return Token(make_loc(), Token::Tag::L_Brace);
    if (accept('}')) return Token(make_loc(), Token::Tag::R_Brace);
    if (accept('(')) return Token(make_loc(), Token::Tag::L_Paren);
    if (accept(')')) return Token(make_loc(), Token::Tag::R_Paren);
    if (accept('[')) return Token(make_loc(), Token::Tag::L_Bracket);
    if (accept(']')) return Token(make_loc(), Token::Tag::R_Bracket);
    if (accept('<')) return Token(make_loc(), Token::Tag::L_Angle);
    if (accept('>')) return Token(make_loc(), Token::Tag::R_Angle);
    if (accept(':')) return Token(make_loc(), Token::Tag::Colon);
    if (accept(',')) return Token(make_loc(), Token::Tag::Comma);
    if (accept('.')) return Token(make_loc(), Token::Tag::Dot);
    if (accept('*')) return Token(make_loc(), Token::Tag::Star);

    if (accept('#')) {
        if (accept("lambda")) return Token(make_loc(), Token::Tag::Lambda);
        if (accept("sigma"))  return Token(make_loc(), Token::Tag::Sigma);
        if (accept("pi"))     return Token(make_loc(), Token::Tag::Pi);
    }

    // greek letters for lambda, sigma, and pi
    if (accept(0xce)) {
        if (accept(0xbb)) return Token(make_loc(), Token::Tag::Lambda);
        if (accept(0xa3)) return Token(make_loc(), Token::Tag::Sigma);
        if (accept(0xa0)) return Token(make_loc(), Token::Tag::Pi);
    }

    if (accept("true"))  return Token(make_loc(), Literal(Literal::Tag::Lit_bool, Box(true)));
    if (accept("false")) return Token(make_loc(), Literal(Literal::Tag::Lit_bool, Box(false)));

    auto c = stream_.peek();
    if (std::isdigit(c) || c == '+' || c == '-') {
        auto lit = parse_literal();
        return Token(make_loc(), lit);
    }

    std::string ident;
    while (std::isalpha(c)) {
        ident += stream_.peek();
        eat();
    }
    if (ident != "") return Token(make_loc(), ident);

    ELOG("invalid token in {}", make_loc());
}

void Lexer::eat() {
    int c = stream_.get();

    if (c == std::istream::traits_type::eof())
        return;

    if (c == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
}

void Lexer::eat_spaces() {
    while (std::isspace(stream_.peek())) eat();
}

Literal Lexer::parse_literal() {
    std::string lit;
    auto parse_digits = [&] () {
        while (std::isdigit(stream_.peek())) {
            lit += stream_.peek();
            eat();
        }
    };

    // sign
    bool sign = false;
    if (accept('+'))      lit += '+';
    else if (accept('-')) {
        sign = true;
        lit += '-';
    }

    // prefix starting with '0'
    int base = 10;
    if (accept('0')) {
        if (accept('b')) base = 2;
        if (accept('x')) base = 16;
        base = 8;
    }

    parse_digits();

    if (base == 10) {
        // parse fractional part
        if (accept('.')) parse_digits();

        // parse exponent
        bool exp = false;
        if (accept('e')) {
            exp = true;
            lit += 'e';
            if (accept('+')) lit += '+';
            if (accept('-')) lit += '-';
            parse_digits();
        }
    }

    // suffix
    if (!exp) {
        if (accept("s8"))  return Literal(Literal::Tag::Lit_s8,   s8( strtol(lit.c_str(), nullptr, base)));
        if (accept("s16")) return Literal(Literal::Tag::Lit_s16, s16( strtol(lit.c_str(), nullptr, base)));
        if (accept("s32")) return Literal(Literal::Tag::Lit_s32, s32( strtol(lit.c_str(), nullptr, base)));
        if (accept("s64")) return Literal(Literal::Tag::Lit_s64, s64(strtoll(lit.c_str(), nullptr, base)));

        if (!sign) {
            if (accept("u8"))  return Literal(Literal::Tag::Lit_u8,   u8( strtoul(lit.c_str(), nullptr, base)));
            if (accept("u16")) return Literal(Literal::Tag::Lit_u16, u16( strtoul(lit.c_str(), nullptr, base)));
            if (accept("u32")) return Literal(Literal::Tag::Lit_u32, u32( strtoul(lit.c_str(), nullptr, base)));
            if (accept("u64")) return Literal(Literal::Tag::Lit_u64, u64(strtoull(lit.c_str(), nullptr, base)));
        }
    }

    if (base == 10) {
        if (accept("r16")) return Literal(Literal::Tag::Lit_r16, r16(strtof(lit.c_str(), nullptr)));
        if (accept("r32")) return Literal(Literal::Tag::Lit_r32, r32(strtof(lit.c_str(), nullptr)));
        if (accept("r64")) return Literal(Literal::Tag::Lit_r64, r64(strtod(lit.c_str(), nullptr)));
    }

    ELOG("invalid literal in {}", Location(filename_.c_str(), line_, col_));
}

bool Lexer::accept(int c) {
    if (stream_.peek() == c) {
        eat();
        return true;
    }
    return false;
}

bool Lexer::accept(const std::string& str) {
    auto it = str.begin();
    while (it != str.end() && accept(*(it++))) ;
    return it == str.end() && std::isspace(stream_.peek());
}

}
