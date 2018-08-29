#include <stack>

#include "thorin/def.h"

namespace thorin {

class Printer2 {
public:
    Printer2() {}

    void print(const Def* def);

private:
    bool push(const Def* def) {
        // inline-dump defs with zero ops
        if (def->num_ops() > 0 && done_.emplace(def).second) {
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
        if (auto app = def->isa<App>()) {
            // inline-dump callee if it's an axiom or a curried axiom
            if (get_axiom(app->callee()) == nullptr)
                todo |= push(app->callee());

            if (auto tuple = app->arg()->isa<Tuple>()) {
                for (auto op : tuple->ops())
                    todo |= push(op);

            } else {
                todo |= push(app->arg());
            }

            goto post_order_visit;
        }

        for (auto op : def->ops())
            todo |= push(op);

post_order_visit:
        if (!todo) {
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
