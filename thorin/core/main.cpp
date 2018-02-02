#include "thorin/core/world.h"
#include "thorin/frontend/parser.h"
#include "thorin/rules/compiler.h"

#include <iostream>
#include <sstream>

void emit() {
#if 0
    d->dump();
    l->dump();
    r->dump();
    std::array<Array<const char*>, Num<WOp>> rules_WOp;
    rules_WOp[size_t(WOp::add)] = {
        "[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> x",
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, x) -> mul f w x 1ₐ ({2u64: int w}, x)",
    };
    rules_WOp[size_t(WOp::sub)] = {
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, {0u64: int w}) -> x",
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, x) -> {0u64: int w})",
    };
    rules_WOp[size_t(WOp::mul)] = {
        "[f: nat, w: nat, x: int w]. mul f w 1ₐ ({1u64: int w}, x) -> x",
        "[f: nat, w: nat, x: int w]. mul f w 1ₐ ({0u64: int w}, x) -> {0u64: int w}",
    };
    rules_WOp[size_t(WOp::shl)] = {
        "[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> {0u64: int w}",
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, {0u64: int w}) -> x",
    };

    std::array<Array<const char*>, Num<MOp>> rules_MOp;
    rules_MOp[size_t(MOp::sdiv)] = {};
    rules_MOp[size_t(MOp::udiv)] = {};
    rules_MOp[size_t(MOp::smod)] = {};
    rules_MOp[size_t(MOp::umod)] = {};

    std::array<Array<const char*>, Num<ICmp>> rules_ICmp;
    rules_ICmp[size_t(ICmp::t)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_t f w (x, y) -> true",
    };
    rules_ICmp[size_t(ICmp::f)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_f f w (x, y) -> false",
    };
    rules_ICmp[size_t(ICmp::sge)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_sge  f w (x, y) -> icmp_sle  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::sgt)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_sgt  f w (x, y) -> icmp_slt  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::ugt)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_ugt  f w (x, y) -> icmp_ult  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::uge)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_uge  f w (x, y) -> icmp_ule  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::sugt)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_sugt f w (x, y) -> icmp_sult f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::suge)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_suge f w (x, y) -> icmp_sule f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::eq)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_eq   f w (x, x) -> true",
    };
    rules_ICmp[size_t(ICmp::ne)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_ne   f w (x, x) -> false",
    };
    rules_ICmp[size_t(ICmp::sle)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_sle  f w (x, x) -> true",
    };
    rules_ICmp[size_t(ICmp::ule)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_ule  f w (x, x) -> true",
    };
    rules_ICmp[size_t(ICmp::sule)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_sule f w (x, x) -> true",
    };
    // iand
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ ({0u64: int w}, x) -> {0u64: int w}");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ ({-1us64: int w}, x) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (x, x) -> x");
    // ior
    rule("[f: nat, w: nat, x: int w]. ior f w 1ₐ ({0u64: int w}, x) -> x");
    rule("[f: nat, w: nat, x: int w]. ior f w 1ₐ ({-1us64: int w}, x) -> {0u64: int w}");
    rule("[f: nat, w: nat, x: int w]. ior f w 1ₐ (x, x) -> x");
    // ixor
    rule("[f: nat, w: nat, x: int w]. ixor f w 1ₐ (x, x) -> {0u64: int w}");
    rule("[f: nat, w: nat, x: int w]. ixor f w 1ₐ ({-1s64: int w}, (ixor f w 1ₐ ({-1s64: int w}, x))) -> x"); // double negation
    // absorption
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (x, ior  f w 1ₐ (x, y)) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (x, iand f w 1ₐ (x, y)) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (x, ior  f w 1ₐ (y, x)) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (x, iand f w 1ₐ (y, x)) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (ior  f w 1ₐ (x, y), x) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (iand f w 1ₐ (x, y), x) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (ior  f w 1ₐ (y, x), x) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (iand f w 1ₐ (y, x), x) -> x");
#endif
}

using namespace thorin::core;
using namespace thorin::rules;

int main(int, const char**) {
    World w;
    Compiler compiler;
    compiler.register_rule(w.op<WOp::add>(), parse_rule(w, "[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> x"));
    compiler.register_rule(w.op<WOp::mul>(), parse_rule(w, "[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> x"));
    //compiler.register_rule(w.op<WOp::mul>(), parse_rule(w, "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, x) -> mul f w 1ₐ ({2u64: int w}, x)"));
    compiler.emit();
    return EXIT_SUCCESS;
}
