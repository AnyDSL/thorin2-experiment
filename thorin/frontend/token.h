#ifndef THORIN_TOKEN_H
#define THORIN_TOKEN_H

#include <string>

#include "thorin/tables.h"
#include "thorin/util/location.h"

namespace thorin {

struct Literal {
    enum class Tag {
#define CODE(T) Lit_##T,
        THORIN_TYPES(CODE)
#undef CODE
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
        L_Brace,
        R_Brace,
        L_Paren,
        R_Paren,
        L_Bracket,
        R_Bracket,
        L_Angle,
        R_Angle,
        Colon,
        Comma,
        Identifier,
        Literal,
        Pi,
        Sigma,
        Lambda,
        Star,
        Eof
    };

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

    Literal literal() const { return literal_; }
    std::string identifier() const { return identifier_; }
    Location location() const { return location_; }

private:
    Tag      tag_;
    Location location_;

    Literal     literal_;
    std::string identifier_;
};

}

#endif // TOKEN_H
