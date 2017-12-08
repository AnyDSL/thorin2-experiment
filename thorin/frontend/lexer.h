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

    template <typename Pred>
    bool accept_if(Pred pred, bool append = true) {
        if (pred(peek())) {
            if (append) str_.append(peek_bytes_);
            next();
            return true;
        }
        return false;
    }
    bool accept(uint32_t val, bool append = true) {
        return accept_if([val] (uint32_t p) { return p == val; }, append);
    }
    bool accept(const char* p, bool append = true) {
        while (*p != '\0') {
            if (!accept(*p++, append)) return false;
        }
        return true;
    }

    uint32_t next();
    uint32_t peek() const { return peek_; }
    const std::string& str() const { return str_; }
    Location location() const { return {filename_, front_line_, front_col_, back_line_, back_col_}; }

    std::istream& stream_;
    uint32_t peek_ = 0;
    char peek_bytes_[5] = {0, 0, 0, 0, 0};
    const char* filename_;
    std::string str_;
    uint32_t front_line_ = 1,
             front_col_  = 1,
             back_line_  = 1,
             back_col_   = 1,
             peek_line_  = 1,
             peek_col_   = 1;
};

}

#endif
