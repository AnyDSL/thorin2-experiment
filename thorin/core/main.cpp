#include "thorin/core/world.h"
#include "thorin/util/stream.h"

#include <iostream>
#include <sstream>

namespace thorin {

void emit() {
    std::ostringstream os;

    streamf(os, "// This file is auto-generated.\n");
    streamf(os, "// Do not change this file.\n");
    streamf(os, "\n");
    streamf(os, "#include \"thorin/core/world.h\"\n");
    streamf(os, "\n");
    streamf(os, "namespace thorin::core {{\n");
    streamf(os, "\n");
    streamf(os, "const Def* normalize_add_(const Def* callee, const Def* arg, Debug dbg) {{\n");
    streamf(os, "}}\n");
    streamf(os, "\n");
    streamf(os, "}}\n");

    std::cout << os.str();
}

}

int main(int, const char** argv) {
    thorin::emit();
    return EXIT_SUCCESS;
}
