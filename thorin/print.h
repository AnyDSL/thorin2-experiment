#ifndef THORIN_PRINT_H
#define THORIN_PRINT_H

#include "thorin/util/stream.h"

namespace thorin {

class Def;

class DefPrinter : public PrinterBase<DefPrinter> {
public:
    explicit DefPrinter(std::ostream& ostream = std::cout, const char* tab = "    ")
        : PrinterBase<DefPrinter>(ostream, tab)
    {}

    void print(const Def* def);
};

template void Streamable<DefPrinter>::dump() const;

}

#endif
