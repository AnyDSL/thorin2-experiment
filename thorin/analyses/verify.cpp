#include "thorin/world.h"
#include "thorin/analyses/scope.h"
#include "thorin/util/log.h"

namespace thorin {

static void verify_top_level(World& world) {
    Scope::for_each(world, [&] (const Scope& scope) {
        for (auto def : scope.free()) {
            if (!def->isa_lambda())
                ELOG("top-level continuation '{}' got free def '{}' at location '{}'", scope.entry(), def, def->loc());
        }
    });
}

class Cycles {
public:
    enum Color {
        Gray, Black
    };

    explicit Cycles(World& world)
        : world_(world)
    {
        def2color_.rehash(round_to_power_of_2(world.defs().size()));
    }

    World& world() { return world_; }
    void run();
    void analyze_call(Lambda*);
    void analyze(ParamSet& params, Lambda*, const Def*);

private:
    World& world_;
    DefMap<Color> def2color_;
};

void Cycles::run() {
    for (auto lambda : world().lambdas())
        analyze_call(lambda);
}

void Cycles::analyze_call(Lambda* lambda) {
    if (def2color_.emplace(lambda, Gray).second) {
        ParamSet params;
        for (auto op : lambda->ops())
            analyze(params, lambda, op);

        for (auto param : params) {
            if (def2color_.emplace(param, Gray).second) {
                analyze_call(param->lambda());
                def2color_[param] = Black;
            }
        }

        def2color_[lambda] = Black;
    } else {
        assertf(def2color_[lambda] != Gray, "detected cycle: '{}'", lambda);
    }
}

void Cycles::analyze(ParamSet& params, Lambda* lambda, const Def* def) {
    if (!def->isa<Lambda>()) {
        if (def2color_.emplace(def, Black).second) {
            for (auto op : def->ops())
                analyze(params, lambda, op);
        }
    } else if (auto param = def->isa<Param>()) {
        if (param->lambda() != lambda) {
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
