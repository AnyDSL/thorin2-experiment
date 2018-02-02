#include "thorin/core/world.h"
#include "thorin/frontend/parser.h"

#include <iostream>
#include <sstream>

namespace thorin::core {

void emit() {
    World w;
    auto [d, l, r] = parse_rule(w,
"[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> x");
    d->dump();
    l->dump();
    r->dump();
}

}

int main(int, const char**) {
    thorin::core::emit();
    return EXIT_SUCCESS;
}
