#ifndef THORIN_PRINT_H
#define THORIN_PRINT_H

#include <stack>

#include "thorin/util/hash.h"
#include "thorin/util/stream.h"

namespace thorin {

class Def;
using DefSet = GIDSet<const Def*>;

class DefPrinter : public PrinterBase<DefPrinter> {
public:
    explicit DefPrinter(std::ostream& ostream = std::cout, const char* tab = "    ")
        : PrinterBase<DefPrinter>(ostream, tab)
    {}

    DefPrinter& recurse(const Def* def);

private:
    bool push(const Def* def);

    std::stack<const Def*> stack_;
    DefSet done_;
};

template void Streamable<DefPrinter>::dump() const;

}

#endif
