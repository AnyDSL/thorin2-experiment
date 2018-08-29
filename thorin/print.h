#ifndef THORIN_PRINT_H
#define THORIN_PRINT_H

#include "thorin/util/stream.h"

namespace thorin {

class Def;

class Printer : public thorin::IndentPrinter<Printer> {
public:
    explicit Printer(std::ostream& ostream = std::cout, const char* tab = "    ")
        : IndentPrinter<Printer>(ostream, tab)
    {}

    void print(const Def* def);
};

template void Streamable<Printer>::dump() const;

void print(const Def* def);

}

#endif
