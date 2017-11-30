#ifndef THORIN_LEXER_H
#define THORIN_LEXER_H

#include "thorin/util/location.h"
#include "thorin/frontend/token.h"

namespace thorin {

class Lexer {
public:
    Lexer(std::istream&, const char* filename);

    Token lex(); ///< Get next \p Token in stream.
    const char* filename() const { return filename_; }

private:
    void eat();
    void eat_spaces();
    Literal parse_literal();

    bool accept(uint32_t);
    bool accept(const std::string&);

    uint32_t next();
    int peek() const { return stream_.peek(); }
    Location location() const { return {filename_, front_line_, front_col_, back_line_, back_col_}; }
    Location curr() const { return location().back(); }

    std::istream& stream_;
    const char* filename_;
    uint32_t front_line_ = 1, front_col_ = 1, back_line_ = 1, back_col_ = 1, peek_line_ = 1, peek_col_ = 1;
};

}

#endif
