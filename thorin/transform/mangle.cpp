#include "thorin/transform/mangle.h"

#include "thorin/world.h"

namespace thorin {

Mangler::Mangler(const Scope& scope, Defs args, DefSet lift)
    : world_(scope.world())
    , scope_(scope)
    , args_(args)
    , lift_(lift)
    , old_entry_(scope.entry())
    , old2new_(scope.defs().capacity())
{
    assert(!old_entry_->empty());
    //assert(arg->type() == old_entry_->type()->domain());
    assert(lift.empty() && "not yet implemented");
#ifndef NDEBUG
    for (auto def : lift)
        assert(scope.free().contains(def));
#endif
}

Lambda* Mangler::mangle() {
    DefVector param_types;
    for (size_t i = 0, e = args_.size(); i != e; ++i) {
        if (args_[i] == nullptr)
            param_types.emplace_back(old_entry_->param(i)->type());
    }

    auto new_pi = world().pi(old_entry_->qualifier(), world().sigma(param_types), old_entry_->codomain());
    new_entry_ = world().lambda(new_pi, old_entry_->debug_history());

    old2new_[old_entry_] = old_entry_;

    // map params to args
    for (size_t i = 0, j = 0, e = args_.size(); i != e; ++i) {
        auto old_param = old_entry_->param(i);
        if (auto def = args_[i])
            old2new_[old_param] = def;
        else {
            auto new_param = new_entry_->param(j++);
            old2new_[old_param] = new_param;
            new_param->debug().set(old_param->name());
        }
    }

    auto new_filter = mangle(old_entry_->filter());
    auto new_body   = mangle(old_entry_->body());
    return new_entry_->set(new_filter, new_body);
}

const Def* Mangler::mangle(const Def* old_def) {
    if (auto new_def = find(old2new_, old_def)) return new_def;
    if (!within(old_def)) return old_def;

    auto new_type = mangle(old_def->type());

    Def* new_nominal = nullptr;
    if (old_def->is_nominal()) {
        new_nominal = old_def->stub(new_type);
        checked_emplace(old2new_, old_def, new_nominal);
    }

    DefArray new_ops(old_def->num_ops());
    for (size_t i = 0, e = old_def->num_ops(); i != e; ++i)
        new_ops[i] = mangle(old_def->op(i));

    if (new_nominal != nullptr) {
        for (size_t i = 0, e = new_ops.size(); i != e; ++i)
            new_nominal->set(i, new_ops[i]);
        return new_nominal;
    }

    auto new_def = old_def->rebuild(world(), new_type, new_ops);
    return checked_emplace(old2new_, old_def, new_def);
}

//------------------------------------------------------------------------------

Lambda* mangle(const Scope& scope, Defs args, DefSet lift) {
    return Mangler(scope, args, lift).mangle();
}

//Lambda* drop(const Call& call) {
    //Scope scope(call.callee()->as_lambda());
    //return drop(scope, call.args());
//}

//------------------------------------------------------------------------------

}
