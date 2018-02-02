#include "thorin/rules/compiler.h"

#include "thorin/util/stream.h"

namespace thorin::rules {

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
