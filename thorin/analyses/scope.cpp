#include "thorin/analyses/scope.h"

#include <algorithm>
#include <fstream>

#include "thorin/world.h"
#include "thorin/analyses/cfg.h"
#include "thorin/analyses/domtree.h"
#include "thorin/analyses/looptree.h"
#include "thorin/analyses/schedule.h"

namespace thorin {

Scope::Scope(Lambda* entry)
    : entry_(entry)
    , exit_(entry->world().cn_end())
{
    run();
}

Scope::~Scope() {}

Scope& Scope::update() {
    defs_.clear();
    cfa_ = nullptr;
    free_ = nullptr;
    return *this;
}

void Scope::run() {
    std::queue<const Def*> queue;

    auto enqueue = [&] (const Def* def) {
        if (defs_.insert(def).second)
            queue.push(def);
    };

    enqueue(entry_);
    enqueue(entry_->param());

    while (!queue.empty()) {
        auto def = pop(queue);
        if (def != entry_) {
            for (auto use : def->uses())
                enqueue(use);
        }
    }

    enqueue(world().cn_end());
}

const DefSet& Scope::free() const {
    if (!free_) {
        free_ = std::make_unique<DefSet>();

        for (auto def : defs_) {
            for (auto op : def->ops()) {
                if (!contains(op))
                    free_->emplace(op);
            }
        }
    }

    return *free_;
}

const CFA& Scope::cfa() const { return lazy_init(this, cfa_); }
const F_CFG& Scope::f_cfg() const { return cfa().f_cfg(); }
const B_CFG& Scope::b_cfg() const { return cfa().b_cfg(); }

template<bool elide_empty>
void Scope::for_each(const World& world, std::function<void(Scope&)> f) {
    LambdaSet done;
    std::queue<Lambda*> queue;

    auto enqueue = [&] (Lambda* lambda) {
        const auto& p = done.insert(lambda);
        if (p.second)
            queue.push(lambda);
    };

    for (auto lambda : world.external_lambdas()) {
        assert(!lambda->empty() && "external must not be empty");
        enqueue(lambda);
    }

    while (!queue.empty()) {
        auto lambda = pop(queue);
        if (elide_empty && lambda->empty()) continue;
        Scope scope(lambda);
        f(scope);

        for (auto def : scope.free()) {
            // TODO we need to look through the free defs as well, as lambdas might be stored in e.g. globals
            if (auto lambda = def->isa_lambda())
                enqueue(lambda);
        }
    }
}

template void Scope::for_each<true> (const World&, std::function<void(Scope&)>);
template void Scope::for_each<false>(const World&, std::function<void(Scope&)>);

Printer& Scope::stream(Printer& p) const { return schedule(*this).stream(p); }
void Scope::write_thorin(const char* filename) const { return schedule(*this).write_thorin(filename); }
void Scope::thorin() const { schedule(*this).thorin(); }

}
