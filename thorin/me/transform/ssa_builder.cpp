#include "thorin/me/transform/ssa_builder.h"

#include "thorin/me/world.h"
#include "thorin/util/log.h"

#if 0

namespace thorin::me {

//------------------------------------------------------------------------------

Value Value::create_val(SSABuilder& builder, const Def* val) {
    Value result;
    result.tag_     = ImmutableValRef;
    result.builder_ = &builder;
    result.def_     = val;
    return result;
}

Value Value::create_mut(SSABuilder& builder, size_t handle, const Def* type) {
    Value result;
    result.tag_     = MutableValRef;
    result.builder_ = &builder;
    result.handle_  = handle;
    result.type_    = type;
    return result;
}

Value Value::create_ptr(SSABuilder& builder, const Def* ptr) {
    Value result;
    result.tag_     = PtrRef;
    result.builder_ = &builder;
    result.def_     = ptr;
    return result;
}

Value Value::create_agg(Value value, const Def* offset) {
    assert(value.tag() != Empty);
    if (value.use_lea())
        return create_ptr(*value.builder_, value.builder_->world().op_lea(value.def_, offset, offset->debug()));
    Value result;
    result.tag_    = AggRef;
    result.builder_ = value.builder_;
    result.value_.reset(new Value(value));
    result.def_     = offset;
    return result;
}

#if 0

bool Value::use_lea() const {
    if (tag() == PtrRef)
        return thorin::use_lea(def()->type()->as<PtrType>()->pointee());
    return false;
}

const Def* Value::load(Debug dbg) const {
    switch (tag()) {
        case ImmutableValRef: return def_;
        case MutableValRef:   return builder_->get_value(handle_, type_, dbg);
        case PtrRef:          return builder_->load(def_, dbg);
        case AggRef:          return builder_->extract(value_->load(dbg), def_, dbg);
        default: THORIN_UNREACHABLE;
    }
}

void Value::store(const Def* val, Debug dbg) const {
    switch (tag()) {
        case MutableValRef: builder_->set_value(handle_, val); return;
        case PtrRef:        builder_->store(def_, val, dbg); return;
        case AggRef:        value_->store(world().insert(value_->load(dbg), def_, val, dbg), dbg); return;
        default: THORIN_UNREACHABLE;
    }
}

#endif

World& Value::world() const { return builder_->world(); }

//------------------------------------------------------------------------------

#ifndef NDEBUG
#else
JumpTarget::~JumpTarget() {
    assert((!cn_ || first_ || cn_->is_sealed()) && "JumpTarget not sealed");
}
#endif

#if 0

Cn* JumpTarget::untangle() {
    if (!first_)
        return cn_;
    assert(cn_);
    auto bb = world().basicblock(debug());
    cn_->jump(bb, {}, debug());
    first_ = false;
    return cn_ = bb;
}

void Cn::jump(JumpTarget& jt, Debug dbg) {
    if (!jt.cn_) {
        jt.cn_ = this;
        jt.first_ = true;
    } else
        this->jump(jt.untangle(), {}, dbg);
}

Cn* JumpTarget::branch_to(World& world, Debug dbg) {
    auto bb = world.basicblock(dbg + (cn_ ? name() + Symbol("_crit") : name()));
    bb->jump(*this, dbg);
    bb->seal();
    return bb;
}

Cn* JumpTarget::enter() {
    if (cn_ && !first_)
        cn_->seal();
    return cn_;
}

Cn* JumpTarget::enter_unsealed(World& world) {
    return cn_ ? untangle() : cn_ = world.basicblock(debug());
}


//------------------------------------------------------------------------------

Cn* SSABuilder::cn(Debug dbg) {
    return cn(world().fn_type(), CC::C, Intrinsic::None, dbg);
}

Cn* SSABuilder::cn(const FnType* fn, CC cc, Intrinsic intrinsic, Debug dbg) {
    auto l = world().cn(fn, cc, intrinsic, dbg);
    if (fn->num_ops() >= 1 && fn->ops().front()->isa<MemType>()) {
        auto param = l->params().front();
        l->set_mem(param);
        if (param->debug().name().empty())
            param->debug().set("mem");
    }

    return l;
}

void SSABuilder::jump(JumpTarget& jt, Debug dbg) {
    if (is_reachable()) {
        cur_bb->jump(jt, dbg);
        set_unreachable();
    }
}

void SSABuilder::branch(const Def* cond, JumpTarget& t, JumpTarget& f, Debug dbg) {
    if (is_reachable()) {
        if (auto lit = cond->isa<PrimLit>()) {
            jump(lit->value().get_bool() ? t : f, dbg);
        } else if (&t == &f) {
            jump(t, dbg);
        } else {
            auto tc = t.branch_to(world_, dbg);
            auto fc = f.branch_to(world_, dbg);
            cur_bb->branch(cond, tc, fc, dbg);
            set_unreachable();
        }
    }
}

void SSABuilder::match(const Def* val, JumpTarget& otherwise, Defs patterns, Array<JumpTarget>& targets, Debug dbg) {
    assert(patterns.size() == targets.size());
    if (is_reachable()) {
        if (patterns.size() == 0) return jump(otherwise, dbg);
        if (auto lit = val->isa<PrimLit>()) {
            for (size_t i = 0; i < patterns.size(); i++) {
                if (patterns[i]->as<PrimLit>() == lit)
                    return jump(targets[i], dbg);
            }
            return jump(otherwise, dbg);
        }
        Array<Cn*> cns(patterns.size());
        for (size_t i = 0; i < patterns.size(); i++)
            cns[i] = targets[i].branch_to(world_, dbg);
        cur_bb->match(val, otherwise.branch_to(world_, dbg), patterns, cns, dbg);
        set_unreachable();
    }
}

const Def* SSABuilder::call(const Def* to, Defs args, const Def* ret_type, Debug dbg) {
    if (is_reachable()) {
        auto p = cur_bb->call(to, args, ret_type, dbg);
        cur_bb = p.first;
        return p.second;
    }
    return nullptr;
}

//void SSABuilder::set_mem(const Def* mem) { if (is_reachable()) cur_bb->set_mem(mem); }


const Def* SSABuilder::create_frame(Debug dbg) {
    auto enter = world().enter(get_mem(), dbg);
    set_mem(world().extract(enter, 0_s, dbg));
    return world().extract(enter, 1, dbg);
}

const Def* SSABuilder::alloc(const Def* type, const Def* extra, Debug dbg) {
    if (!extra)
        extra = world().literal_qu64(0, dbg);
    auto alloc = world().alloc(type, get_mem(), extra, dbg);
    set_mem(world().extract(alloc, 0_s, dbg));
    return world().extract(alloc, 1, dbg);
}

const Def* SSABuilder::load(const Def* ptr, Debug dbg) {
    auto load = world().load(get_mem(), ptr, dbg);
    set_mem(world().extract(load, 0_s, dbg));
    return world().extract(load, 1, dbg);
}

void SSABuilder::store(const Def* ptr, const Def* val, Debug dbg) {
    set_mem(world().store(get_mem(), ptr, val, dbg));
}

const Def* SSABuilder::extract(const Def* agg, u32 index, Debug dbg) {
    return extract(agg, world().literal_qu32(index, dbg), dbg);
}

const Def* SSABuilder::extract(const Def* agg, const Def* index, Debug dbg) {
    if (auto ld = Load::is_out_val(agg)) {
        if (use_lea(ld->out_val_type()))
            return load(world().lea(ld->ptr(), index, dbg), dbg);
    }
    return world().extract(agg, index, dbg);
}

#endif
//------------------------------------------------------------------------------

/*
 * values numbering
 */

const Def* SSABuilder::BasicBlock::get_value(size_t handle, const Def* type, Debug dbg) {
    auto result = get(handle);
    if (result)
        goto return_result;

    if (parent() != this) { // is a function head?
        if (parent()) {
            result = parent()->get_value(handle, type, dbg);
            goto return_result;
        }
    } else {
        if (!is_sealed_) {
            // TODO
            //auto param = append_param(type, dbg);
            const Def* param = nullptr;
            size_t index = -1;
            todos_.emplace_back(handle, index, type, dbg);
            result = set_value(handle, param);
            goto return_result;
        }

        switch (preds().size()) {
            case 0:
                goto return_bottom;
            case 1:
                result = set_value(handle, preds().front()->get_value(handle, type, dbg));
                goto return_result;
            default: {
                if (is_visited_) {
                    const Def* param = nullptr; //TODO: append_param(type, dbg)
                    result = set_value(handle, param); // create param to break cycle
                    goto return_result;
                }

                is_visited_ = true;
                const Def* same = nullptr;
                for (auto pred : preds()) {
                    auto def = pred->get_value(handle, type, dbg);
                    if (same && same != def) {
                        same = (const Def*)-1; // defs from preds() are different
                        break;
                    }
                    same = def;
                }
                assert(same != nullptr);
                is_visited_ = false;

                // fix any params which may have been introduced to break the cycle above
                const Def* def = nullptr;
                if (auto found = get(handle)) {
                    size_t index = -1; // TODO:found->as<Param>()->index();
                    def = fix(handle, index, type, dbg);
                }

                if (same != (const Def*)-1) {
                    result = same;
                    goto return_result;
                }

                if (def) {
                    result = set_value(handle, def);
                    goto return_result;
                }

                // TODO
                const Def* param = nullptr; //append_param(type, dbg);
                size_t index = -1;
                set_value(handle, param);
                fix(handle, index, type, dbg);
                result = param;
                goto return_result;
            }
        }
    }

return_bottom:
    WLOG(&dbg, "'{}' may be undefined", dbg.name());
    return set_value(handle, world().bottom(type));

return_result:
    assert(result->type() == type);
    return result;
}

void SSABuilder::BasicBlock::seal() {
    assert(!is_sealed() && "already sealed");
    is_sealed_ = true;

    for (const auto& todo : todos_)
        fix(todo.handle(), todo.index(), todo.type(), todo.debug());
    todos_.clear();
}

const Def* SSABuilder::BasicBlock::fix(size_t /*handle*/, size_t index, const Def* /*type*/, Debug /*dbg*/) {
    // TODO auto param = this->param(index);
    const Def* param = nullptr;
    size_t param_index = -1;

    assert(is_sealed() && "must be sealed");
    assert(index == param_index);

    //for (auto pred : preds()) {
        //assert(!pred->empty());
        //assert(pred->direct_succs().size() == 1 && "critical edge");
        //auto def = pred->get_value(handle, type, dbg);

        // make potentially room for the new arg
        //TODO if (index >= pred->num_args())
            //pred->resize(index+2);

        //assert(!pred->arg(index) && "already set");
        //pred->set_op(index + 1, def);
    //}

    return try_remove_trivial_param(param);
}

const Def* SSABuilder::BasicBlock::try_remove_trivial_param(const Def* param) {
    //assert(param->continuation() == this);
    assert(is_sealed() && "must be sealed");

    //TODO size_t index = param->index();
    size_t index = -1;

    // find Horspool-like phis
    const Def* same = nullptr;
    for (auto pred : preds()) {
        //TODO auto def = pred->arg(index);
        const Def* def = nullptr;
        if (def == param || same == def)
            continue;
        if (same)
            return param;
        same = def;
    }
    assert(same != nullptr);
    param->replace(same);

    //for (auto peek : param->peek()) {
        //auto continuation = peek.from();
        //continuation->unset_op(index+1);
        //continuation->set_op(index+1, world().bottom(param->type(), param->debug()));
    //}

#if 0
    for (auto use : same->uses()) {
        if (SSABuilder::BasicBlock* continuation = use->isa_continuation()) {
            for (auto succ : continuation->succs()) {
                size_t index = -1;
                for (size_t i = 0, e = succ->num_args(); i != e; ++i) {
                    if (succ->arg(i) == use.def()) {
                        index = i;
                        break;
                    }
                }
                if (index != size_t(-1) && param != succ->param(index))
                    succ->try_remove_trivial_param(succ->param(index));
            }
        }
    }
#endif

    return same;
}

//------------------------------------------------------------------------------

}

#endif
