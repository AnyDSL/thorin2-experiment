#ifndef THORIN_LEXER_H
#define THORIN_LEXER_H

#include "thorin/util/location.h"
#include "thorin/frontend/token.h"

namespace thorin {

class Lexer {
public:
    Lexer(std::istream&, const std::string&);

    std::string filename() const { return filename_; }

    Token next();

private:
    void eat();
    void eat_spaces();
    Literal parse_literal();

    bool accept(int);
    bool accept(const std::string&);

    std::istream& stream_;
    std::string   filename_;
    uint32_t      line_, col_;
};

}

#endif
