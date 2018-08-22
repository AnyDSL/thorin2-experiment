#include "thorin/print2.h"

#include "thorin/def.h"

namespace thorin {

class Printer2 {
public:
    Printer2(const Def* def)
        : root_(def)
    {}

    void print() {}

private:
    const Def* root_;
};

void print(const Def* def) {
    Printer2 printer(def);
    printer.print();
}

}
