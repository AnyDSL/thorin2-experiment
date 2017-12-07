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
    Literal parse_literal();

    bool accept(uint32_t val) {
        if (peek() == val) {
            next();
            return true;
        }
        return false;
    }
    bool accept(const char* p) {
        while (*p != '\0') {
            if (!accept(*p++)) return false;
        }
        return true;
    }
    bool accept(std::string& str, uint32_t val) {
        if (peek() == val) {
            str += next();
            return true;
        }
        return false;
    }
    template <typename Pred>
    bool accept_if(Pred pred) {
        if (pred(peek())) {
            next();
            return true;
        }
        return false;
    }
    template <typename Pred>
    bool accept_if(std::string& str, Pred pred) {
        if (pred(peek())) {
            str += next();
            return true;
        }
        return false;
    }

    uint32_t next();
    uint32_t peek() const { return peek_; }
    Location location() const { return {filename_, front_line_, front_col_, back_line_, back_col_}; }
    Location curr() const { return location().back(); }

    std::istream& stream_;
    uint32_t peek_ = 0;
    const char* filename_;
    uint32_t front_line_ = 1, front_col_ = 1, back_line_ = 1, back_col_ = 1, peek_line_ = 1, peek_col_ = 1;
};

}

#endif
