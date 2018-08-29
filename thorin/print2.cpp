#include <stack>

#include "thorin/def.h"

namespace thorin {

class Printer2 {
public:
    Printer2() {}

    void print(const Def* def);
};

void Printer2::print(const Def* def) {
    std::stack<const Def*> stack;
    DefSet done;

    auto push = [&](const Def* def) {
        // inline-dump defs with zero ops
        if (def->num_ops() > 0 && done.emplace(def).second) {
            stack.emplace(def);
            return true;
        }
        return false;
    };

    stack.emplace(def);

    while (!stack.empty()) {
        auto def = stack.top();

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
            stack.pop();
        }
    }
}

void print2(const Def* def) {
    Printer2 printer;
    printer.print(def);
}

}
