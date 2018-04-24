#ifndef THORIN_PRINT_H
#define THORIN_PRINT_H

#include "thorin/util/stream.h"

namespace thorin {

class Printer : public thorin::IndentPrinter<Printer> {
public:
    explicit Printer(std::ostream& ostream, const char* tab = "    ")
        : IndentPrinter<Printer>(ostream, tab)
    {}
};

}

#endif
