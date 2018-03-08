#include "thorin/transform/mangle.h"

#include "thorin/world.h"

#if 0

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

    auto lambda_type = world().lambda_type(param_types);
    new_entry_ = world().lambda(lambda_type, old_entry_->debug_history());

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

    //mangle_body(old_entry_, new_entry_);

    auto new_filter = mangle(old_entry_->filter());
    auto new_callee = mangle(old_entry_->callee());
    auto new_arg    = mangle(old_entry_->arg());

    return new_entry_->set(new_filter, new_callee, new_arg);
}

//void Mangler::mangle_body(Lambda* old_lambda, Lambda* new_lambda) {
    //assert(!old_lambda->empty());

    //// fold branch and match
    //// TODO find a way to factor this out in lambda.cpp
    ////if (auto callee = old_lambda->callee()->isa_lambda()) {
        ////switch (callee->intrinsic()) {
            ////case Intrinsic::Branch: {
                ////if (auto lit = mangle(old_lambda->arg(0))->isa<PrimLit>()) {
                    ////auto cont = lit->value().get_bool() ? old_lambda->arg(1) : old_lambda->arg(2);
                    ////return new_lambda->jump(mangle(cont), {}, old_lambda->jump_debug());
                ////}
                ////break;
            ////}
            ////case Intrinsic::Match:
                ////if (old_lambda->num_args() == 2)
                    ////return new_lambda->jump(mangle(old_lambda->arg(1)), {}, old_lambda->jump_debug());

                ////if (auto lit = mangle(old_lambda->arg(0))->isa<PrimLit>()) {
                    ////for (size_t i = 2; i < old_lambda->num_args(); i++) {
                        ////auto new_arg = mangle(old_lambda->arg(i));
                        ////if (world().extract(new_arg, 0_s)->as<PrimLit>() == lit)
                            ////return new_lambda->jump(world().extract(new_arg, 1), {}, old_lambda->jump_debug());
                    ////}
                ////}
                ////break;
            ////default:
                ////break;
        ////}
    ////}

    //Array<const Def*> nops(old_lambda->num_ops());
    //for (size_t i = 0, e = nops.size(); i != e; ++i)
        //nops[i] = mangle(old_lambda->op(i));

    //Defs nargs(nops.skip_front()); // new args of new_lambda
    //auto ntarget = nops.front();   // new target of new_lambda

    //// check whether we can optimize tail recursion
    //if (ntarget == old_entry_) {
        //std::vector<size_t> cut;
        //bool substitute = true;
        //for (size_t i = 0, e = args_.size(); i != e && substitute; ++i) {
            //if (auto def = args_[i]) {
                //substitute &= def == nargs[i];
                //cut.push_back(i);
            //}
        //}

        ////if (substitute) {
            ////const auto& args = concat(nargs.cut(cut), new_entry_->params().get_back(lift_.size()));
            ////return new_lambda->jump(new_entry_, args, old_lambda->jump_debug());
        ////}
    //}

    //new_lambda->jump(ntarget, nargs, old_lambda->jump_debug());
//}

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

#endif
