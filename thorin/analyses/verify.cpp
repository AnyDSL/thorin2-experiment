#include "thorin/world.h"
#include "thorin/analyses/scope.h"
#include "thorin/util/log.h"

namespace thorin {

static void verify_top_level(World& world) {
    Scope::for_each(world, [&] (const Scope& scope) {
        for (auto def : scope.free()) {
            assertf(def->isa_cn(), "top-level continuation '{}' got free def '{}' at location '{}'",
                    scope.entry(), def, def->location());
        }
    });
}

class Cycles {
public:
    enum Color {
        Gray, Black
    };

    Cycles(World& world)
        : world_(world)
    {
        def2color_.rehash(round_to_power_of_2(world.defs().size()));
    }

    World& world() { return world_; }
    void run();
    void analyze_call(Cn*);
    void analyze(ParamSet& params, Cn*, const Def*);

private:
    World& world_;
    DefMap<Color> def2color_;
};

void Cycles::run() {
    for (auto cn : world().cns())
        analyze_call(cn);
}

void Cycles::analyze_call(Cn* cn) {
    if (def2color_.emplace(cn, Gray).second) {
        ParamSet params;
        for (auto op : cn->ops())
            analyze(params, cn, op);

        for (auto param : params) {
            if (def2color_.emplace(param, Gray).second) {
                analyze_call(param->cn());
                def2color_[param] = Black;
            }
        }

        def2color_[cn] = Black;
    } else
        assertf(def2color_[cn] != Gray, "detected cycle: '{}'", cn);
}

void Cycles::analyze(ParamSet& params, Cn* cn, const Def* def) {
    if (!def->isa<Cn>()) {
        if (def2color_.emplace(def, Black).second) {
            for (auto op : def->ops())
                analyze(params, cn, op);
        }
    } else if (auto param = def->isa<Param>()) {
        if (param->cn() != cn) {
            auto i = def2color_.find(param);
            if (i != def2color_.end())
                assertf(i->second != Gray, "detected cycle induced by parameter: '{}'", param);
            else
                params.emplace(param);
        }
    }
}

void verify(World& world) {
    verify_top_level(world);
    Cycles cycles(world);
    cycles.run();
}

}
