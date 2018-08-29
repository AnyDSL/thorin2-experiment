#include <stack>

#include "thorin/def.h"

namespace thorin {

class Printer2 {
public:
    Printer2()
    {}

    void print(const Def* def);

private:
    bool push(const Def* def) {
        if (done_.emplace(def).second) {
            stack_.emplace(def);
            return true;
        }
        return false;
    };

    std::stack<const Def*> stack_;
    DefSet done_;
};

void Printer2::print(const Def* def) {
    stack_.emplace(def);

    while (!stack_.empty()) {
        auto def = stack_.top();

        bool todo = false;
        for (auto op : def->ops()) {
            todo |= push(op);
        }

        if (auto app = def->isa<App>(); !todo || app->has_axiom()) {
            //if (def->isa<Axiom>() || def->isa<Lit>()) {
                ////def->dump();
                //continue;
            //}

            def->dump();
            stack_.pop();
        }
    }
}

void print2(const Def* def) {
    Printer2 printer;
    printer.print(def);
}

}
