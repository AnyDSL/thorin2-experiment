#include "thorin/print2.h"

#include "thorin/def.h"

namespace thorin {

class Printer2 {
public:
    Printer2()
    {}

    void print(const Def* def);

private:
    void enqueue(const Def* def) {
        if (done_.emplace(def).second)
            queue_.emplace(def);
    };

    std::queue<const Def*> queue_;
    DefSet done_;
};

void Printer2::print(const Def* def) {
    queue_.emplace(def);

    while (!queue_.empty()) {
        auto def = pop(queue_);
        def->dump();

        if (def->isa<Axiom>() || def->isa<Lit>()) {
            //def->dump();
            continue;
        }

        for (auto op : def->ops()) {
            enqueue(op);
        }
    }
}

void print2(const Def* def) {
    Printer2 printer;
    printer.print(def);
}

}
