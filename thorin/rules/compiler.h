#ifndef THORIN_RULES_COMPILER_H
#define THORIN_RULES_COMPILER_H

#include "thorin/world.h"
#include "thorin/rules/rule.h"

namespace thorin::rules {


class Compiler {
public:
    typedef GIDMap<const Def*, Rules> Axiom2Rules;

    void register_rule(const Axiom* axiom, Rule rule) { axiom2rules_[axiom].emplace_back(rule); }

    void emit();

private:
    void emit_prologue();
    void emit_epilogue();

    std::ostringstream out_;
    Axiom2Rules axiom2rules_;
};

}

#endif
