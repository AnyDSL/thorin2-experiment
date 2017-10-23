#include <cctype>

#include <utf8.h>

#include "thorin/util/log.h"
#include "thorin/frontend/lexer.h"

namespace thorin {

Lexer::Lexer(std::istream& is, const std::string& name)
    : stream_(is)
    , iterator_(is)
    , filename_(name)
    , line_(1), col_(0)
    , cur_(0), eof_(false)
{
    char bytes[3];
    std::fill_n(bytes, 3, std::char_traits<char>::eof());
    stream_.read(bytes, 3);
    if (!utf8::starts_with_bom(bytes, bytes + 3)) {
        stream_.unget();
        stream_.unget();
        stream_.unget();
    }
    eat();
}

Token Lexer::next() {
    eat_spaces();

    auto front_line = line_;
    auto front_col = col_;
    auto make_loc = [&] () {
        return Location(filename_.c_str(), front_line, front_col, line_, col_);
    };

    if (eof()) return Token(make_loc(), Token::Tag::Eof);

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
        if (accept("true"))  return Token(make_loc(), Literal(Literal::Tag::Lit_bool, Box(true)));
        if (accept("false")) return Token(make_loc(), Literal(Literal::Tag::Lit_bool, Box(false)));

        return Token(make_loc(), Token::Tag::Sharp);
    }

    // greek letters
    if (accept(0x0003bb)) return Token(make_loc(), Token::Tag::Lambda);
    if (accept(0x0003a3)) return Token(make_loc(), Token::Tag::Sigma);
    if (accept(0x0003a0)) return Token(make_loc(), Token::Tag::Pi);
    if (accept(0x01D538)) return Token(make_loc(), Token::Tag::Arity_Kind);
    if (accept(0x01D544)) return Token(make_loc(), Token::Tag::Multi_Arity_Kind);
    if (accept(0x00211A)) {
        if (accept(0x2096))
            return Token(make_loc(), Token::Tag::Qualifier_Kind);
        return Token(make_loc(), Token::Tag::Qualifier_Type);
    }

    if (std::isdigit(peek()) || peek() == '+' || peek() == '-') {
        auto lit = parse_literal();
        return Token(make_loc(), lit);
    }

    std::string ident;
    auto is_identifier_char = [&] (uint32_t c) {
        return std::isalpha(c) || c == '_' || (ident != "" && std::isdigit(c));
    };
    while (!eof() && is_identifier_char(peek())) {
        ident += peek();
        eat();
    }
    if (ident != "") return Token(make_loc(), ident);

    std::string invalid;
    utf8::append(peek(), std::back_inserter(invalid));
    ELOG_LOC(make_loc(), "invalid character '{}'", invalid);
}

void Lexer::eat() {
    if (iterator_ == std::istreambuf_iterator<char>()) {
        eof_ = true;
        return;
    }

    if (peek() == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }

    cur_ = utf8::unchecked::next(iterator_);
}

void Lexer::eat_spaces() {
    while (!eof() && std::isspace(peek())) eat();
}

Literal Lexer::parse_literal() {
    std::string lit;
    int base = 10;

    auto parse_digits = [&] () {
        while (std::isdigit(peek()) ||
               (base == 16 && peek() >= 'a' && peek() <= 'f') ||
               (base == 16 && peek() >= 'A' && peek() <= 'F')) {
            lit += peek();
            eat();
        }
    };

    // sign
    bool sign = false;
    if (accept('+')) lit += '+';
    else if (accept('-')) {
        sign = true;
        lit += '-';
    }

    // prefix starting with '0'
    if (accept('0')) {
        if (accept('b')) base = 2;
        else if (accept('x')) base = 16;
        else if (accept('o')) base = 8;
    }

    parse_digits();

    bool exp = false, fract = false;
    if (base == 10) {
        // parse fractional part
        if (accept('.')) {
            fract = true;
            lit += '.';
            parse_digits();
        }

        // parse exponent
        if (accept('e')) {
            exp = true;
            lit += 'e';
            if (accept('+')) lit += '+';
            if (accept('-')) lit += '-';
            parse_digits();
        }
    }

    // suffix
    if (!exp && !fract) {
        if (accept('s')) {
            if (accept("8"))  return Literal(Literal::Tag::Lit_s8,   s8( strtol(lit.c_str(), nullptr, base)));
            if (accept("16")) return Literal(Literal::Tag::Lit_s16, s16( strtol(lit.c_str(), nullptr, base)));
            if (accept("32")) return Literal(Literal::Tag::Lit_s32, s32( strtol(lit.c_str(), nullptr, base)));
            if (accept("64")) return Literal(Literal::Tag::Lit_s64, s64(strtoll(lit.c_str(), nullptr, base)));
        }

        if (!sign && accept('u')) {
            if (accept("8"))  return Literal(Literal::Tag::Lit_u8,   u8( strtoul(lit.c_str(), nullptr, base)));
            if (accept("16")) return Literal(Literal::Tag::Lit_u16, u16( strtoul(lit.c_str(), nullptr, base)));
            if (accept("32")) return Literal(Literal::Tag::Lit_u32, u32( strtoul(lit.c_str(), nullptr, base)));
            if (accept("64")) return Literal(Literal::Tag::Lit_u64, u64(strtoull(lit.c_str(), nullptr, base)));
        }
    }

    if (base == 10 && accept('r')) {
        if (accept("16")) return Literal(Literal::Tag::Lit_r16, r16(strtof(lit.c_str(), nullptr)));
        if (accept("32")) return Literal(Literal::Tag::Lit_r32, r32(strtof(lit.c_str(), nullptr)));
        if (accept("64")) return Literal(Literal::Tag::Lit_r64, r64(strtod(lit.c_str(), nullptr)));
    }

    // untyped literals
    if (base == 10 && !fract && !exp) {
        return Literal(Literal::Tag::Lit_untyped, u64(strtoull(lit.c_str(), nullptr, 10)));
    }

    ELOG_LOC(Location(filename_.c_str(), line_, col_), "invalid literal in {}");
}

bool Lexer::accept(uint32_t c) {
    if (peek() == c) {
        eat();
        return true;
    }
    return false;
}

// TODO this method is basically broken because mathcing "ac" won't work:
// if (accept("ab")) ...
// if (accept("ac")) ..
bool Lexer::accept(const std::string& str) {
    auto it = str.begin();
    while (it != str.end() && accept(*(it++))) ;
    return it == str.end() && (eof() || !(peek() == '_' || std::isalnum(peek())));
}

}
