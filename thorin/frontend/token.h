#ifndef THORIN_TOKEN_H
#define THORIN_TOKEN_H

#include <string>

#include "thorin/tables.h"
#include "thorin/util/location.h"
#include "thorin/util/utility.h"

namespace thorin {

#define THORIN_TOKENS(f) \
    f(L_Brace,          "{") \
    f(R_Brace,          "}") \
    f(L_Paren,          "(") \
    f(R_Paren,          ")") \
    f(L_Bracket,        "[") \
    f(R_Bracket,        "]") \
    f(L_Angle,          "<") \
    f(R_Angle,          ">") \
    f(Colon,            ":") \
    f(Comma,            ",") \
    f(Dot,              ".") \
    f(Semicolon,        ";") \
    f(Star,             "*") \
    f(Sharp,            "#") \
    f(Arrow,            "<-") \
    f(QualifierU,       "ᵁ") \
    f(QualifierR,       "ᴿ") \
    f(QualifierA,       "ᴬ") \
    f(QualifierL,       "ᴸ") \
    f(Backslash,        "\\") \
    f(Pi,               "\\pi") \
    f(Sigma,            "\\sigma") \
    f(Lambda,           "\\lambda") \
    f(Qualifier_Type,   "\\qualifier_type") \
    f(Qualifier_Kind,   "\\qualifier_kind") \
    f(Arity_Kind,       "\\arity_kind") \
    f(Multi_Arity_Kind, "\\multi_arity_kind") \
    f(Identifier,       "identifier") \
    f(Literal,          "literal") \
    f(Eof,              "eof")

struct Literal {
    enum class Tag {
#define CODE(T) Lit_##T,
        THORIN_TYPES(CODE)
#undef CODE
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
        THORIN_TOKENS(CODE)
#undef CODE
    };

    Token() {}
    Token(Location loc, Literal lit)
        : tag_(Tag::Literal)
        , location_(loc)
        , literal_(lit)
    {}

    Token(Location loc, const std::string& ident)
        : tag_(Tag::Identifier)
        , location_(loc)
        , identifier_(ident)
    {}

    Token(Location loc, Tag tag)
        : tag_(tag)
        , location_(loc)
    {}

    Tag tag() const { return tag_; }
    Literal literal() const { return literal_; }
    std::string identifier() const { return identifier_; }
    Location location() const { return location_; }

    bool isa(Tag tag) const { return tag_ == tag; }

    static std::string tag_to_string(Tag tag) {
        switch (tag) {
#define CODE(T, S) case Tag::T: return S;
            THORIN_TOKENS(CODE)
#undef CODE
            default: THORIN_UNREACHABLE;
        }
    }

private:
    Tag      tag_;
    Location location_;

    Literal     literal_;
    std::string identifier_;
};

std::ostream& operator<<(std::ostream& os, const Token& t);

}

#endif // TOKEN_H
