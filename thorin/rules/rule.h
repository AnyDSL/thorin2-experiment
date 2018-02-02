#ifndef THORIN_RULES_RULE_H
#define THORIN_RULES_RULE_H

namespace thorin::rules {

class Rule {
public:
    Rule(const Def* domain, const Def* lhs, const Def* rhs)
        : domain_(domain)
        , lhs_(lhs)
        , rhs_(rhs)
    {}

    const Def* domain() const { return domain_; }
    const Def* lhs() const { return lhs_; }
    const Def* rhs() const { return rhs_; }

private:
    const Def* domain_;
    const Def* lhs_;
    const Def* rhs_;
};

typedef std::vector<Rule> Rules;

}

#endif
