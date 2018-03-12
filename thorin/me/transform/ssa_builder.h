#ifndef THORIN_ME_TRANSFORM_SSA_BUILDER_H
#define THORIN_ME_TRANSFORM_SSA_BUILDER_H

#if 0

#include "thorin/me/world.h"

namespace thorin::me {

class SSABuilder;
class Slot;
class World;

//------------------------------------------------------------------------------

class Value {
public:
    enum Tag {
        Empty,
        ImmutableValRef,
        MutableValRef,
        PtrRef,
        AggRef,
    };

    Value()
        : tag_(Empty)
        , builder_(nullptr)
        , handle_(-1)
        , type_(nullptr)
        , def_(nullptr)
    {}
    Value(const Value& value)
        : tag_   (value.tag())
        , builder_(value.builder_)
        , handle_ (value.handle_)
        , type_   (value.type_)
        , def_    (value.def_)
        , value_  (value.value_ == nullptr ? nullptr : new Value(*value.value_))
    {}
    Value(Value&& value)
        : Value()
    {
        swap(*this, value);
    }

    Value static create_val(SSABuilder&, const Def* val);
    Value static create_mut(SSABuilder&, size_t handle, const Def* type);
    Value static create_ptr(SSABuilder&, const Def* ptr);
    Value static create_agg(Value value, const Def* offset);

    Tag tag() const { return tag_; }
    SSABuilder* builder() const { return builder_; }
    World& world() const;
    const Def* load(Debug) const;
    void store(const Def* val, Debug) const;
    const Def* def() const { return def_; }
    operator bool() { return tag() != Empty; }
    bool use_lea() const { /*TODO*/ return false; }

    Value& operator= (Value other) { swap(*this, other); return *this; }
    friend void swap(Value& v1, Value& v2) {
        using std::swap;
        swap(v1.tag_,     v2.tag_);
        swap(v1.builder_, v2.builder_);
        swap(v1.handle_,  v2.handle_);
        swap(v1.type_,    v2.type_);
        swap(v1.def_,     v2.def_);
        swap(v1.value_,   v2.value_);
    }

private:
    Tag tag_;
    SSABuilder* builder_;
    size_t handle_;
    const Def* type_;
    const Def* def_;
    std::unique_ptr<Value> value_;
};

//------------------------------------------------------------------------------

class JumpTarget {
public:
    JumpTarget(Debug dbg = {})
        : debug_(dbg)
    {}
#ifndef NDEBUG
#else
    ~JumpTarget();
#endif

    const Debug& debug() const { return debug_; }
    Symbol name() const { return debug().name(); }
    //World& world() const { assert(cn_); return cn_->world(); }
    //void seal() { assert(cn_); cn_->seal(); }

private:
    void jump_from(Cn* bb);
    Cn* branch_to(World& world, Debug);
    Cn* untangle();
    Cn* enter();
    Cn* enter_unsealed(World& world);

    Debug debug_;
    Cn* cn_ = nullptr;
    bool first_ = false;

    //friend void Cn::jump(JumpTarget&, Debug);
    friend class SSABuilder;
};

//------------------------------------------------------------------------------

class SSABuilder {
public:
    class BasicBlock {
    public:
        BasicBlock(const BasicBlock&) = delete;
        BasicBlock(BasicBlock&&) = delete;
        BasicBlock& operator=(const BasicBlock&) = delete;

        BasicBlock(Cn* cn)
            : cn_(cn)
        {}

        ///@{ getters
        World& world() { return static_cast<World&>(cn_->world()); }
        Cn* cn() const { return cn_; }
        BasicBlock* parent() { return parent_; }
        std::vector<BasicBlock*> preds() { return preds_; }
        std::vector<BasicBlock*> succs() { return succs_; }
        ///@}

        ///@ value numbering - get/set value
        const Def* set_value(size_t handle, const Def* def) { return get(handle) = def; }
        const Def* set_mem(const Def* def) { return set_value(0, def); }
        const Def* get_value(size_t handle, const Def* type, Debug dbg = {});
        const Def* get_mem() { return get_value(0, world().type_mem(), {"mem"}); }
        ///@}

        ///@{ value numbering - enter/seal
        void seal();
        bool is_sealed() const { return is_sealed_; }
        ///@}

    private:
        class Todo {
        public:
            Todo() {}
            Todo(size_t handle, size_t index, const Def* type, Debug dbg)
                : handle_(handle)
                , index_(index)
                , type_(type)
                , debug_(dbg)
            {}

            size_t handle() const { return handle_; }
            size_t index() const { return index_; }
            const Def* type() const { return type_; }
            Debug debug() const { return debug_; }

        private:
            size_t handle_;
            size_t index_;
            const Def* type_;
            Debug debug_;
        };

        ///@ value numbering - internal helpers
        const Def* fix(size_t handle, size_t index, const Def* type, Debug dbg);
        const Def* try_remove_trivial_param(const Def*);
        Tracker& get(size_t handle) {
            if (handle >= values_.size())
                values_.resize(handle+1);
            return values_[handle];
        }
        ///@}

        void link(BasicBlock* other) {
            this ->succs_.emplace_back(other);
            other->preds_.emplace_back(this);
        }

        /**
         * There exist three cases to distinguish here.
         * - @p parent_ == this: This @p BasicBlock is considered as a basic block, i.e.,
         *                       SSA construction will propagate value through this @p BasicBlock's predecessors.
         * - @p parent_ == nullptr: This @p BasicBlock is considered as top level function, i.e.,
         *                          SSA construction will stop propagate values here.
         *                          Any @p get_value which arrives here without finding a definition will return @p bottom.
         * - otherwise: This @p BasicBlock is considered as function head nested in @p parent_.
         *              Any @p get_value which arrives here without finding a definition will recursively try to find one in @p parent_.
         */
        BasicBlock* parent_;
        Cn* cn_;
        std::deque<Tracker> values_;
        std::vector<Todo> todos_;
        std::vector<BasicBlock*> preds_;
        std::vector<BasicBlock*> succs_;
        bool is_sealed_  = false;
        bool is_visited_ = false;
    };

    SSABuilder(World& world)
        : world_(world)
        , current_(nullptr)
    {}

    ///@{ getters
    World& world() const { return world_; }
    bool is_reachable() const { return current_ != nullptr; }
    void set_unreachable() { current_ = nullptr; }
    BasicBlock* current() const { return current_; }
    ///@}

    //const Def* create_frame(Debug);
    //const Def* alloc(const Def* type, const Def* extra, Debug dbg = {});
    //const Def* load(const Def* ptr, Debug dbg = {});
    //const Def* extract(const Def* agg, const Def* index, Debug dbg = {});
    //const Def* extract(const Def* agg, u32 index, Debug dbg = {});
    //void store(const Def* ptr, const Def* val, Debug dbg = {});
    //Cn* cn(const FnType* fn, CC cc = CC::C, Intrinsic intrinsic = Intrinsic::None, Debug dbg = {});
    //Cn* cn(const FnType* fn, Debug dbg = {}) { return cn(fn, CC::C, Intrinsic::None, dbg); }
    //Cn* cn(Debug dbg = {});

    ///@{ value numbering - get/set value
    ///@}

    ///@{ value numbering - enter/seal
    //Cn* enter(JumpTarget& jt) { return current_ = jt.enter(); }
    //void enter(Cn* cn) { current_ = cn; cn->seal(); }
    //Cn* enter_unsealed(JumpTarget& jt) { return current_ = jt.enter_unsealed(world_); }
    //void seal(Cn* cn) { return get(cn)->seal(); }
    //void seal() { return seal(current()); }
    //bool is_sealed(Cn* cn) const { return get(cn)->is_sealed(); }
    //bool is_sealed() const { return is_sealed(current()); }
    ///@}

    ///@{ value numbering - terminate
    void jump(JumpTarget& jt, Debug dbg = {});
    void branch(const Def* cond, JumpTarget& t, JumpTarget& f, Debug dbg = {});
    void match(const Def* val, JumpTarget& otherwise, Defs patterns, Array<JumpTarget>& targets, Debug dbg = {});
    const Def* call(const Def* to, Defs args, const Def* ret_type, Debug dbg = {});
    ///@}

    //Continuation* parent() const { return parent_; }            ///< See @p parent_ for more information.
    //void set_parent(Continuation* parent) { parent_ = parent; } ///< See @p parent_ for more information.

protected:
    BasicBlock* get(Cn* cn) const {
        auto [i, success] = cn2bb_.emplace(cn, nullptr);
        if (success)
            i->second = std::make_unique<BasicBlock>(cn);
        return i->second.get();
    }

    World& world_;
    BasicBlock* current_ = nullptr;
    mutable CnMap<std::unique_ptr<BasicBlock>> cn2bb_;
};

//------------------------------------------------------------------------------

}
#endif

#endif
