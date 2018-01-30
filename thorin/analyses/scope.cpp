#include "thorin/analyses/scope.h"

#include <algorithm>
#include <fstream>

#include "thorin/world.h"
#include "thorin/analyses/cfg.h"
#include "thorin/analyses/domtree.h"
#include "thorin/analyses/looptree.h"
#include "thorin/analyses/schedule.h"

namespace thorin {

Scope::Scope(Cn* entry)
    : world_(entry->world())
{
    run(entry);
}

Scope::~Scope() {}

Scope& Scope::update() {
    auto e = entry();
    cns_.clear();
    defs_.clear();
    cfa_ = nullptr;
    run(e);
    return *this;
}

void Scope::run(Cn* entry) {
    std::queue<const Def*> queue;

    auto enqueue = [&] (const Def* def) {
        if (defs_.insert(def).second) {
            queue.push(def);

            if (auto cn = def->isa_cn())
                cns_.push_back(cn);
        }
    };

    enqueue(entry);

    while (!queue.empty()) {
        auto def = pop(queue);
        if (def != entry) {
            for (auto use : def->uses())
                enqueue(use);
        }
    }

    enqueue(world().cn_end());
}

const CFA& Scope::cfa() const { return lazy_init(this, cfa_); }
const F_CFG& Scope::f_cfg() const { return cfa().f_cfg(); }
const B_CFG& Scope::b_cfg() const { return cfa().b_cfg(); }

template<bool elide_empty>
void Scope::for_each(const World& world, std::function<void(Scope&)> f) {
    CnSet done;
    std::queue<Cn*> queue;

    auto enqueue = [&] (Cn* cn) {
        const auto& p = done.insert(cn);
        if (p.second)
            queue.push(cn);
    };

    for (auto cn : world.external_cns()) {
        assert(!cn->empty() && "external must not be empty");
        enqueue(cn);
    }

    while (!queue.empty()) {
        auto cn = pop(queue);
        if (elide_empty && cn->empty())
            continue;
        Scope scope(cn);
        f(scope);

        for (auto n : scope.f_cfg().reverse_post_order()) {
            for (auto succ : n->cn()->succs()) {
                if (!scope.contains(succ))
                    enqueue(succ);
            }
        }
    }
}

template void Scope::for_each<true> (const World&, std::function<void(Scope&)>);
template void Scope::for_each<false>(const World&, std::function<void(Scope&)>);

std::ostream& Scope::stream(std::ostream& os) const { return schedule(*this).stream(os); }
void Scope::write_thorin(const char* filename) const { return schedule(*this).write_thorin(filename); }
void Scope::thorin() const { schedule(*this).thorin(); }

}
