#include "thorin/transform/mangle.h"

#include "thorin/world.h"
#include "thorin/analyses/scope.h"

namespace thorin {
#if 0

Mangler::Mangler(const Scope& scope, const Def* arg, DefSet lift)
    : world_(arg->world())
    , scope_(scope)
    , arg_(arg)
    , lift_(lift)
    , old_entry_(scope.entry())
    , old2new_(scope.defs().capacity())
{
    assert(!old_entry_->empty());
    assert(arg->type() == old_entry_->type()->domain());
#ifndef NDEBUG
    for (auto def : lift)
        assert(scope.free().contains(def));
#endif
}

const Def* Mangler::mangle_signature(const Def* type, const Def* arg) {
    if (arg->isa<Top>())
        return type;
    assert(!type->is_nominal());

    if (auto sigma = type->isa<Sigma>()) {
        std::vector<const Def*> types;

        if (auto tuple = arg->isa<Tuple>()) {
            for (size_t i = 0; i != tuple->num_ops(); ++i) {
                if (tuple->op(i)->isa<Top>())
                    types.emplace_back(sigma->op(i));
                else
                    types.emplace_back(mangle_signature(sigma->op(i), tuple->op(i));
            }
        }
    }

    if (auto variadic = type->isa<Variadic>()) {
    }

    return type;
}

Cn* Mangler::mangle() {
    // create new_entry - but first collect and specialize all param types
    std::vector<const Type*> param_types;
    for (size_t i = 0, e = old_entry()->num_params(); i != e; ++i) {
        if (args_[i] == nullptr)
            param_types.emplace_back(old_entry()->param(i)->type()); // TODO reduce
    }

    auto fn_type = world().fn_type(param_types);
    new_entry_ = world().cn(fn_type, old_entry()->debug_history());

    // map value params
    old2new_[old_entry()] = old_entry();
    for (size_t i = 0, j = 0, e = old_entry()->num_params(); i != e; ++i) {
        auto old_param = old_entry()->param(i);
        if (auto def = args_[i])
            old2new_[old_param] = def;
        else {
            auto new_param = new_entry()->param(j++);
            old2new_[old_param] = new_param;
            new_param->debug().set(old_param->name());
        }
    }

    for (auto def : lift_)
        old2new_[def] = new_entry()->append_param(def->type()); // TODO reduce

    // mangle pe_profile
    if (!old_entry()->pe_profile().empty()) {
        Array<const Def*> new_pe_profile(new_entry()->num_params());
        size_t j = 0;
        for (size_t i = 0, e = old_entry()->num_params(); i != e; ++i) {
            if (args_[i] == nullptr)
                new_pe_profile[j++] = mangle(old_entry()->pe_profile(i));
        }

        for (size_t e = new_entry()->num_params(); j != e; ++j)
            new_pe_profile[j] = world().literal_bool(false, Debug{});

        new_entry()->set_pe_profile(new_pe_profile);
    }

    mangle_body(old_entry(), new_entry());

    return new_entry();
}

Cn* Mangler::mangle_head(Cn* old_cn) {
    assert(!old2new_.contains(old_cn));
    assert(!old_cn->empty());
    Cn* new_cn = old_cn->stub();
    old2new_[old_cn] = new_cn;

    for (size_t i = 0, e = old_cn->num_params(); i != e; ++i)
        old2new_[old_cn->param(i)] = new_cn->param(i);

    return new_cn;
}

void Mangler::mangle_body(Cn* old_cn, Cn* new_cn) {
    assert(!old_cn->empty());
    new_cns_.emplace_back(new_cn);

    // fold branch and match
    // TODO find a way to factor this out in cn.cpp
    if (auto callee = old_cn->callee()->isa_cn()) {
        switch (callee->intrinsic()) {
            case Intrinsic::Branch: {
                if (auto lit = mangle(old_cn->arg(0))->isa<PrimLit>()) {
                    auto cont = lit->value().get_bool() ? old_cn->arg(1) : old_cn->arg(2);
                    return new_cn->jump(mangle(cont), {}, old_cn->jump_debug());
                }
                break;
            }
            case Intrinsic::Match:
                if (old_cn->num_args() == 2)
                    return new_cn->jump(mangle(old_cn->arg(1)), {}, old_cn->jump_debug());

                if (auto lit = mangle(old_cn->arg(0))->isa<PrimLit>()) {
                    for (size_t i = 2; i < old_cn->num_args(); i++) {
                        auto new_arg = mangle(old_cn->arg(i));
                        if (world().extract(new_arg, 0_s)->as<PrimLit>() == lit)
                            return new_cn->jump(world().extract(new_arg, 1), {}, old_cn->jump_debug());
                    }
                }
                break;
            default:
                break;
        }
    }

    Array<const Def*> nops(old_cn->num_ops());
    for (size_t i = 0, e = nops.size(); i != e; ++i)
        nops[i] = mangle(old_cn->op(i));

    Defs nargs(nops.skip_front()); // new args of new_cn
    auto ntarget = nops.front();   // new target of new_cn

    // check whether we can optimize tail recursion
    if (ntarget == old_entry()) {
        std::vector<size_t> cut;
        bool substitute = true;
        for (size_t i = 0, e = args_.size(); i != e && substitute; ++i) {
            if (auto def = args_[i]) {
                substitute &= def == nargs[i];
                cut.push_back(i);
            }
        }

        if (substitute) {
            const auto& args = concat(nargs.cut(cut), new_entry()->params().get_back(lift_.size()));
            return new_cn->jump(new_entry(), args, old_cn->jump_debug());
        }
    }

    new_cn->jump(ntarget, nargs, old_cn->jump_debug());
}

const Def* Mangler::mangle(const Def* old_def) {
    if (auto new_def = find(old2new_, old_def))
        return new_def;
    else if (!within(old_def))
        return old_def;
    else if (auto old_cn = old_def->isa_cn()) {
        auto new_cn = mangle_head(old_cn);
        mangle_body(old_cn, new_cn);
        return new_cn;
    } else if (auto param = old_def->isa<Param>()) {
        assert(within(param->cn()));
        mangle(param->cn());
        assert(old2new_.contains(param));
        return old2new_[param];
    } else {
        auto old_primop = old_def->as<PrimOp>();
        Array<const Def*> nops(old_primop->num_ops());
        for (size_t i = 0, e = old_primop->num_ops(); i != e; ++i)
            nops[i] = mangle(old_primop->op(i));

        auto type = old_primop->type(); // TODO reduce
        return old2new_[old_primop] = old_primop->rebuild(nops, type);
    }
}

//------------------------------------------------------------------------------

Cn* mangle(const Scope& scope, Defs args, Defs lift) {
    return Mangler(scope, args, lift).mangle();
}

Cn* drop(const Call& call) {
    Scope scope(call.callee()->as_cn());
    return drop(scope, call.args());
}

//------------------------------------------------------------------------------

#endif
}
