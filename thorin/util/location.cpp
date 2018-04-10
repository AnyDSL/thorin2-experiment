#include "thorin/util/location.h"
#include "thorin/util/stream.h"

namespace thorin {

Location& Location::operator+=(Location other) {
    this->back_line_ = other.back_line_;
    this->back_col_  = other.back_col_;
    return *this;
}

std::ostream& operator<<(std::ostream& os, Location l) {
#ifdef _MSC_VER
    return os << l.filename() << "(" << l.front_line() << ")";
#else // _MSC_VER
    os << l.filename() << ':';

    if (l.front_col() == uint16_t(-1) || l.back_col() == uint16_t(-1)) {
        if (l.front_line() != l.back_line())
            return streamf(os, "{} - {}", l.front_line(), l.back_line());
        else
            return streamf(os, "{}", l.front_line());
    }

    if (l.front_line() != l.back_line())
        return streamf(os, "{} col {} - {} col {}", l.front_line(), l.front_col(), l.back_line(), l.back_col());

    if (l.front_col() != l.back_col())
        return streamf(os, "{} col {} - {}", l.front_line(), l.front_col(), l.back_col());

    return streamf(os, "{} col {}", l.front_line(), l.front_col());
#endif // _MSC_VER
}

Debug operator+(Debug d1, Debug d2) {
    return {(Location)d1 + (Location)d2, d1.name() + std::string(".") + d2.name()};
}

std::ostream& operator<<(std::ostream& os, Debug dbg) {
    return streamf(os, "{{{}, {}}}", (Location)dbg, dbg.name());
}

}
