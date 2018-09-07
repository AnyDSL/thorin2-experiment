#ifndef THORIN_PRINT_H
#define THORIN_PRINT_H

#include <functional>

#include "thorin/util/array.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"

namespace thorin {

class Def;
typedef ArrayRef<const Def*> Defs;
class Lambda;
using DefSet = GIDSet<const Def*>;

class DefPrinter : public PrinterBase<DefPrinter> {
public:
    explicit DefPrinter(std::ostream& ostream = std::cout, const char* tab = "    ")
        : PrinterBase<DefPrinter>(ostream, tab)
    {}

    std::string str(const Def*) const;
    DefPrinter& recurse(const Def*);
    DefPrinter& recurse(const Lambda*);
    inline stream_list<Defs, std::function<void(const Def*)>> list(Defs);

private:

    bool push(const Def* def);
    void _push(const Def* def);

    std::stack<const Def*> stack_;
    std::queue<const Lambda*> lambdas_;
    DefSet done_;
};

template void Streamable<DefPrinter>::dump() const;

}

#endif
