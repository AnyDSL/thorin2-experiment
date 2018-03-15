#include "thorin/fe/token.h"

namespace thorin::fe {

std::ostream& operator<<(std::ostream& os, const Token& t) {
    os << t.location() << ": ";
    if (t.isa(Token::Tag::Identifier))
        return os << t.symbol();
    if (t.isa(Token::Tag::Literal))
        return os << t.literal().box.get_u64();
    return os << Token::tag_to_string(t.tag());
}

}
