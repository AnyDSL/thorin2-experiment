#ifndef THORIN_FE_TOKEN_H
#define THORIN_FE_TOKEN_H

#include <string>

#include "thorin/util/debug.h"
#include "thorin/util/symbol.h"
#include "thorin/util/types.h"
#include "thorin/util/utility.h"

namespace thorin::fe {

#define THORIN_APP_ARG_TOKENS(f) \
    f(D_brace_l,        "{") \
    f(D_paren_l,        "(") \
    f(D_bracket_l,      "[") \
    f(D_angle_l,        "‹") \
    f(D_quote_l,        "«") \
    f(Q_u,              "ᵁ") \
    f(Q_r,              "ᴿ") \
    f(Q_a,              "ᴬ") \
    f(Q_l,              "ᴸ") \
    f(Backslash,        "\\") \
    f(Pi,               "\\pi") \
    f(Lambda,           "\\lambda") \
    f(Bool,             "bool") \
    f(Identifier,       "identifier") \
    f(Literal,          "literal")

#define THORIN_SORT_TOKENS(f) \
    f(Arity_Kind,       "\\arity_kind") \
    f(Multi_Kind,       "\\multi_arity_kind") \
    f(Qualifier_Type,   "\\qualifier_type") \
    f(Star,             "*")

#define THORIN_OP_TOKENS(f) \
    f(D_brace_r,          "}") \
    f(D_paren_r,          ")") \
    f(D_bracket_r,        "]") \
    f(D_angle_r,          "›") \
    f(D_quote_r,          "»") \
    f(Colon,            ":") \
    f(ColonColon,       "::") \
    f(ColonEqual,       ":=") \
    f(Comma,            ",") \
    f(Dot,              ".") \
    f(Equal,            "=") \
    f(Semicolon,        ";") \
    f(Sharp,            "#") \
    f(Arrow,            "<-") \
    f(Cn,               "cn") \
    f(Eof,              "<eof>")

struct Literal {
    enum class Tag {
#define CODE(T) Lit_##T,
        THORIN_TYPES(CODE)
#undef CODE
        Lit_arity,
        Lit_index,
        Lit_index_arity,
        Lit_untyped
    };

    Tag tag;
    Box box;

    Literal() {}
    Literal(Tag tag, Box box)
        : tag(tag), box(box)
    {}
};

class Token {
public:
    enum class Tag {
#define CODE(T, S) T,
        THORIN_APP_ARG_TOKENS(CODE)
        THORIN_SORT_TOKENS(CODE)
        THORIN_OP_TOKENS(CODE)
#undef CODE
    };

    Token() {}
    Token(Loc loc, Literal lit)
        : tag_(Tag::Literal)
        , loc_(loc)
        , literal_(lit)
    {}
    Token(Loc loc, const std::string& identifier)
        : tag_(Tag::Identifier)
        , loc_(loc)
        , symbol_(identifier)
    {}
    Token(Loc loc, Tag tag)
        : tag_(tag)
        , loc_(loc)
    {}

    Tag tag() const { return tag_; }
    Literal literal() const { return literal_; }
    Symbol symbol() const { return symbol_; }
    Loc loc() const { return loc_; }

    bool isa(Tag tag) const { return tag_ == tag; }
    bool is_app_arg() const {
        switch (tag_) {
#define CODE(T, S) case Tag::T:
            THORIN_APP_ARG_TOKENS(CODE)
#undef CODE
                return true;
            default: return false;
        }
    }

    static std::string tag_to_string(Tag tag) {
        switch (tag) {
#define CODE(T, S) case Tag::T: return S;
            THORIN_APP_ARG_TOKENS(CODE)
            THORIN_SORT_TOKENS(CODE)
            THORIN_OP_TOKENS(CODE)
#undef CODE
            default: THORIN_UNREACHABLE;
        }
    }

private:
    Tag tag_;
    Loc loc_;

    Literal literal_;
    Symbol symbol_;
};

typedef Token::Tag TT;

std::ostream& operator<<(std::ostream& os, const Token& t);

}

#endif // TOKEN_H
