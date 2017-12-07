#include <cctype>

#include <stdexcept>

#include "thorin/util/log.h"
#include "thorin/frontend/lexer.h"

namespace thorin {

static inline int sym(int c) { return std::isalpha(c) || c == '_'; }
static inline int bin(int c) { return '0' <= c && c <= '1'; }
static inline int oct(int c) { return '0' <= c && c <= '7'; }
static inline int eE(int c) { return c == 'e' || c == 'E'; }
static inline int sgn(int c){ return c == '+' || c == '-'; }

Lexer::Lexer(std::istream& is, const char* filename)
    : stream_(is)
    , filename_(filename)
{
    if (!stream_)
        throw std::runtime_error("stream is bad");
    next();
}

inline bool is_bit_set(uint32_t val, uint32_t n) { return bool((val >> n) & 1_u32); }
inline bool is_bit_clear(uint32_t val, uint32_t n) { return !is_bit_set(val, n); }

// see https://en.wikipedia.org/wiki/UTF-8
uint32_t Lexer::next() {
    uint32_t result = peek_;
    uint32_t b1 = stream_.get();

    if (b1 == (uint32_t) std::istream::traits_type::eof()) {
        peek_ = b1;
        return result;
    }

    auto get_next_utf8_byte = [&] () {
        uint32_t b = stream_.get();
        if (is_bit_clear(b, 7) || is_bit_set(b, 6))
            ELOG_LOC(location(), "invalid utf-8 character");
        return b & 0b00111111_u32;
    };

    auto update_peek = [&] (uint32_t peek) {
        back_line_ = peek_line_;
        back_col_  = peek_col_;
        ++peek_col_;
        peek_ = peek;
        return result;
    };

    if (is_bit_clear(b1, 7)) {
        // 1-byte: 0xxxxxxx
        back_line_ = peek_line_;
        back_col_  = peek_col_;

        if (b1 == '\n') {
            ++peek_line_;
            peek_col_ = 1;
        } else
            ++peek_col_;
        peek_ = b1;
        return result;
    } else {
        if (is_bit_set(b1, 6)) {
            if (is_bit_clear(b1, 5)) {
                // 2-bytes: 110xxxxx 10xxxxxx
                uint32_t b2 = get_next_utf8_byte();
                return update_peek((b1 & 0b00011111_u32) << 6_u32 | b2);
            } else if (is_bit_clear(b1, 4)) {
                // 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
                uint32_t b2 = get_next_utf8_byte();
                uint32_t b3 = get_next_utf8_byte();
                return update_peek((b1 & 0b00001111_u32) << 12_u32 | b2 << 6_u32 | b3);
            } else if (is_bit_clear(b1, 3)) {
                // 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                uint32_t b2 = get_next_utf8_byte();
                uint32_t b3 = get_next_utf8_byte();
                uint32_t b4 = get_next_utf8_byte();
                return update_peek((b1 & 0b00000111_u32) << 18_u32 | b2 << 12_u32 | b3 << 6_u32 | b4);
            }
        }
    }
    ELOG_LOC(location(), "invalid utf-8 character");
    return 0;
}

Token Lexer::lex() {
    while (true) {
        // skip whitespace
        if (accept(std::isspace)) {
            while (accept(std::isspace)) {}
            continue;
        }

        std::string str; // the token string is concatenated here
        front_line_ = peek_line_;
        front_col_ = peek_col_;

        if (stream_.eof())
            return {location(), Token::Tag::Eof};

        if (accept('{')) return {location(), Token::Tag::L_Brace};
        if (accept('}')) return {location(), Token::Tag::R_Brace};
        if (accept('(')) return {location(), Token::Tag::L_Paren};
        if (accept(')')) return {location(), Token::Tag::R_Paren};
        if (accept('[')) return {location(), Token::Tag::L_Bracket};
        if (accept(']')) return {location(), Token::Tag::R_Bracket};
        if (accept('<')) return {location(), Token::Tag::L_Angle};
        if (accept('>')) return {location(), Token::Tag::R_Angle};
        if (accept(':')) return {location(), Token::Tag::Colon};
        if (accept(',')) return {location(), Token::Tag::Comma};
        if (accept('.')) return {location(), Token::Tag::Dot};
        if (accept(';')) return {location(), Token::Tag::Semicolon};
        if (accept('*')) return {location(), Token::Tag::Star};

        if (accept('#')) {
            if (accept("lambda")) return {location(), Token::Tag::Lambda};
            if (accept("pi"))     return {location(), Token::Tag::Pi};
            if (accept("true"))   return {location(), Literal(Literal::Tag::Lit_bool, Box(true))};
            if (accept("false"))  return {location(), Literal(Literal::Tag::Lit_bool, Box(false))};

            return {location(), Token::Tag::Sharp};
        }

        // greek letters
        if (accept(0x0003bb)) return {location(), Token::Tag::Lambda};
        if (accept(0x0003a0)) return {location(), Token::Tag::Pi};
        if (accept(0x01D538)) return {location(), Token::Tag::Arity_Kind};
        if (accept(0x01D544)) return {location(), Token::Tag::Multi_Arity_Kind};
        if (accept(0x00211A)) {
            if (accept(0x2096))
                return {location(), Token::Tag::Qualifier_Kind};
            return {location(), Token::Tag::Qualifier_Type};
        }

        if (std::isdigit(peek()) || sgn(peek())) {
            auto lit = parse_literal();
            return {location(), lit};
        }

        // identifier
        if (accept(str, sym)) {
            while (accept(str, sym) || accept(str, std::isdigit)) {}
            return {location(), str};
        }

        ELOG_LOC(location(), "invalid character '{}'", (char) next());
    }
}

Literal Lexer::parse_literal() {
    std::string lit;
    int base = 10;

    auto parse_digits = [&] () {
        switch (base) {
            case  2: while (accept(lit, bin)) {} break;
            case  8: while (accept(lit, oct)) {} break;
            case 10: while (accept(lit, std::isdigit)) {} break;
            case 16: while (accept(lit, std::isxdigit)) {} break;
        }
    };

    // sign
    bool sign = false;
    if (accept(lit, '+')) {}
    else if (accept(lit, '-')) { sign = true; }

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
        if (accept(lit, eE)) {
            exp = true;
            if (accept(lit, sgn)) {}
            parse_digits();
        }
    }

    // suffix
    if (!exp && !fract) {
        if (accept('s')) {
            if (accept("8"))  return {Literal::Tag::Lit_s8,   s8( strtol(lit.c_str(), nullptr, base))};
            if (accept("16")) return {Literal::Tag::Lit_s16, s16( strtol(lit.c_str(), nullptr, base))};
            if (accept("32")) return {Literal::Tag::Lit_s32, s32( strtol(lit.c_str(), nullptr, base))};
            if (accept("64")) return {Literal::Tag::Lit_s64, s64(strtoll(lit.c_str(), nullptr, base))};
        }

        if (!sign && accept('u')) {
            if (accept("8"))  return {Literal::Tag::Lit_u8,   u8( strtoul(lit.c_str(), nullptr, base))};
            if (accept("16")) return {Literal::Tag::Lit_u16, u16( strtoul(lit.c_str(), nullptr, base))};
            if (accept("32")) return {Literal::Tag::Lit_u32, u32( strtoul(lit.c_str(), nullptr, base))};
            if (accept("64")) return {Literal::Tag::Lit_u64, u64(strtoull(lit.c_str(), nullptr, base))};
        }
    }

    if (base == 10 && accept('r')) {
        if (accept("16")) return {Literal::Tag::Lit_r16, r16(strtof(lit.c_str(), nullptr))};
        if (accept("32")) return {Literal::Tag::Lit_r32, r32(strtof(lit.c_str(), nullptr))};
        if (accept("64")) return {Literal::Tag::Lit_r64, r64(strtod(lit.c_str(), nullptr))};
    }

    // untyped literals
    if (base == 10 && !fract && !exp) {
        return Literal(Literal::Tag::Lit_untyped, u64(strtoull(lit.c_str(), nullptr, 10)));
    }

    ELOG_LOC(location(), "invalid literal in {}");
}

bool Lexer::accept(uint32_t c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

bool Lexer::accept(const char* p) {
    while (*p != '\0') {
        if (!accept(*p++)) return false;
    }
    return true;
}

}
