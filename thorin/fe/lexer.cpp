#include <stdexcept>

#include "thorin/util/log.h"
#include "thorin/fe/lexer.h"

namespace thorin::fe {

// character classes
inline bool sp(uint32_t c)  { return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v'; }
inline bool dec(uint32_t c) { return c >= '0' && c <= '9'; }
inline bool hex(uint32_t c) { return dec(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
inline bool sym(uint32_t c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
inline bool bin(uint32_t c) { return '0' <= c && c <= '1'; }
inline bool oct(uint32_t c) { return '0' <= c && c <= '7'; }
inline bool eE(uint32_t c)  { return c == 'e' || c == 'E'; }
inline bool sgn(uint32_t c) { return c == '+' || c == '-'; }

Lexer::Lexer(std::istream& is, const char* filename)
    : stream_(is)
    , filename_(filename)
{
    if (!stream_) throw std::runtime_error("stream is bad");
    next();
    accept(0xfeff, false); // eat utf-8 BOM if present
    front_line_ = front_col_  = 1;
    back_line_  = back_col_   = 1;
    peek_line_  = peek_col_   = 1;
}

inline bool is_bit_set(uint32_t val, uint32_t n) { return bool((val >> n) & 1_u32); }
inline bool is_bit_clear(uint32_t val, uint32_t n) { return !is_bit_set(val, n); }

// see https://en.wikipedia.org/wiki/UTF-8
uint32_t Lexer::next() {
    uint32_t result = peek_;
    uint32_t b1 = stream_.get();
    std::fill(peek_bytes_, peek_bytes_ + 4, 0);
    peek_bytes_[0] = b1;

    if (b1 == (uint32_t) std::istream::traits_type::eof()) {
        back_line_ = peek_line_;
        back_col_  = peek_col_;
        peek_ = b1;
        return result;
    }

    int n_bytes = 1;
    auto get_next_utf8_byte = [&] () {
        uint32_t b = stream_.get();
        peek_bytes_[n_bytes++] = b;
        if (is_bit_clear(b, 7) || is_bit_set(b, 6))
            error("invalid utf-8 character");
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
            peek_col_ = 0;
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
    error("invalid utf-8 character");
    return 0;
}

Token Lexer::lex() {
    while (true) {
        // skip whitespace
        if (accept_if(sp, false)) {
            while (accept_if(sp, false)) {}
            continue;
        }

        str_ = "";
        front_line_ = peek_line_;
        front_col_ = peek_col_;

        if (stream_.eof())
            return {loc(), TT::Eof};

        if (accept('{')) return {loc(), TT::D_brace_l};
        if (accept('}')) return {loc(), TT::D_brace_r};
        if (accept('(')) return {loc(), TT::D_paren_l};
        if (accept(')')) return {loc(), TT::D_paren_r};
        if (accept('[')) return {loc(), TT::D_bracket_l};
        if (accept(']')) return {loc(), TT::D_bracket_r};
        if (accept('<')) {
            if (accept('-')) return {loc(), TT::Arrow};
            return {loc(), TT::D_angle_l}; // TODO
        }
        if (accept('>')) return {loc(), TT::D_angle_r};
        if (accept(':')) {
            if (accept(':')) return {loc(), TT::ColonColon};
            else if (accept('=')) return {loc(), TT::ColonEqual};
            else return {loc(), TT::Colon};
        }
        if (accept('=')) return {loc(), TT::Equal};
        if (accept(',')) return {loc(), TT::Comma};
        if (accept('.')) return {loc(), TT::Dot};
        if (accept(';')) return {loc(), TT::Semicolon};
        if (accept('*')) return {loc(), TT::Star};

        if (accept('\\')) {
            if (accept("lambda")) return {loc(), TT::Lambda};
            if (accept("pi"))     return {loc(), TT::Pi};
            //if (accept("true"))   return {loc(), Literal(Literal::Tag::Lit_bool, Box(true))};
            //if (accept("false"))  return {loc(), Literal(Literal::Tag::Lit_bool, Box(false))};

            return {loc(), TT::Backslash};
        }

        if (accept('#')) return {loc(), TT::Sharp};

        // greek letters
        if (accept(U'λ')) return {loc(), TT::Lambda};
        if (accept(U'Π')) return {loc(), TT::Pi};
        if (accept(U'𝔸')) return {loc(), TT::Arity_Kind};
        if (accept(U'𝕄')) return {loc(), TT::Multi_Kind};
        if (accept(U'ℚ')) return {loc(), TT::Qualifier_Type};
        if (accept(U'ᵁ')) return {loc(), TT::Q_u};
        if (accept(U'ᴿ')) return {loc(), TT::Q_r};
        if (accept(U'ᴬ')) return {loc(), TT::Q_a};
        if (accept(U'ᴸ')) return {loc(), TT::Q_l};
        if (accept(U'«')) return {loc(), TT::D_quote_l};
        if (accept(U'»')) return {loc(), TT::D_quote_r};
        if (accept(U'‹')) return {loc(), TT::D_angle_l};
        if (accept(U'›')) return {loc(), TT::D_angle_r};

        if (dec(peek()) || sgn(peek())) {
            auto lit = parse_literal();
            return {loc(), lit};
        }

        // arity descriptor for index literals
        u64 index_arity = 0;
        auto is_arity_subscript = [] (uint32_t p) { return p > 0x002079 && p < 0x002090; };
        auto update_index_arity = [&] (uint32_t p) { index_arity = 10 * index_arity + (p - 0x002080); };
        if (auto opt = accept_opt([&] (uint32_t p) { return p != 0x002080 && is_arity_subscript(p); }))
            update_index_arity(*opt);
        while (auto opt = accept_opt(is_arity_subscript))
            update_index_arity(*opt);
        if (index_arity != 0)
            return {loc(), {Literal::Tag::Lit_index_arity, index_arity}};

        // identifier
        if (accept_if(sym)) {
            while (accept_if(sym) || accept_if(dec)) {}

            // TODO make this mechanism better
            if (str() == "cn")   return {loc(), TT::Cn};
            if (str() == "bool") return {loc(), TT::Bool};
            return {loc(), str()};
        }

        error("invalid character '{}'", peek_bytes_);
        next();
    }
}

Literal Lexer::parse_literal() {
    int base = 10;

    auto parse_digits = [&] () {
        switch (base) {
            case  2: while (accept_if(bin)) {} break;
            case  8: while (accept_if(oct)) {} break;
            case 10: while (accept_if(dec)) {} break;
            case 16: while (accept_if(hex)) {} break;
        }
    };

    // sign
    bool sign = false;
    if (accept('+')) {}
    else if (accept('-')) { sign = true; }

    // prefix starting with '0'
    if (accept('0', false)) {
        if      (accept('b', false)) base = 2;
        else if (accept('x', false)) base = 16;
        else if (accept('o', false)) base = 8;
    }

    parse_digits();

    bool exp = false, fract = false;
    if (base == 10) {
        // parse fractional part
        if (accept('.')) {
            fract = true;
            parse_digits();
        }

        // parse exponent
        if (accept_if(eE)) {
            exp = true;
            if (accept_if(sgn)) {}
            parse_digits();
        }
    }

    // suffix
    if (!exp && !fract) {
        if (accept('s', false)) {
            if (accept("8",  false)) return {Literal::Tag::Lit_s8,   s8( strtol(str().c_str(), nullptr, base))};
            if (accept("16", false)) return {Literal::Tag::Lit_s16, s16( strtol(str().c_str(), nullptr, base))};
            if (accept("32", false)) return {Literal::Tag::Lit_s32, s32( strtol(str().c_str(), nullptr, base))};
            if (accept("64", false)) return {Literal::Tag::Lit_s64, s64(strtoll(str().c_str(), nullptr, base))};
        }

        if (!sign) {
            if (accept('u', false)) {
                if (accept("8", false))  return {Literal::Tag::Lit_u8,   u8( strtoul(str().c_str(), nullptr, base))};
                if (accept("16", false)) return {Literal::Tag::Lit_u16, u16( strtoul(str().c_str(), nullptr, base))};
                if (accept("32", false)) return {Literal::Tag::Lit_u32, u32( strtoul(str().c_str(), nullptr, base))};
                if (accept("64", false)) return {Literal::Tag::Lit_u64, u64(strtoull(str().c_str(), nullptr, base))};
            }

            if (accept(0x002090))
                return {Literal::Tag::Lit_arity, u64(strtoull(str().c_str(), nullptr, base))};

            if (peek() > 0x002080 && peek() < 0x002090) {
                return {Literal::Tag::Lit_index, u64(strtoull(str().c_str(), nullptr, base))};
            }
        }
    }

    if (base == 10 && accept('f', false)) {
        if (accept("16", false)) return {Literal::Tag::Lit_f16, f16(strtof(str().c_str(), nullptr))};
        if (accept("32", false)) return {Literal::Tag::Lit_f32, f32(strtof(str().c_str(), nullptr))};
        if (accept("64", false)) return {Literal::Tag::Lit_f64, f64(strtod(str().c_str(), nullptr))};
    }

    // untyped literals
    if (base == 10 && !fract && !exp) {
        return Literal(Literal::Tag::Lit_untyped, u64(strtoull(str().c_str(), nullptr, 10)));
    }

    error("invalid literal");
}

}
