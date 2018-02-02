#include "thorin/rules/compiler.h"

#include "thorin/util/stream.h"

namespace thorin::rules {

void Compiler::emit_prologue() {
    streamln(out_, "// This file is auto-generated.");
    streamln(out_, "// Do not change this file.");
    streamln(out_, "");
    streamln(out_, "#include \"thorin/core/world.h\"");
    streamln(out_, "");
    streamln(out_, "namespace thorin::core {{");
    streamln(out_, "");
}

void Compiler::emit_epilogue() {
    streamln(out_, "}}");
}

void Compiler::emit() {
    emit_prologue();

    for (const auto& [axiom, rules] : axiom2rules_) {
        streamln(out_, "const Def* normalize_{}_(const Def* callee, const Def* arg, Debug dbg) {{", axiom->name());
        for ([[maybe_unused]] const auto& rule : rules) {
        }
        streamln(out_, "}}");
        streamln(out_, "");
    }

    emit_epilogue();

    std::cout<< out_.str();
}

}
