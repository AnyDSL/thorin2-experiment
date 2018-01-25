#ifndef THORIN_CORE_WORLD_H
#define THORIN_CORE_WORLD_H

#include <memory>
#include <string>

#include "thorin/world.h"
#include "thorin/core/tables.h"

namespace thorin::core {

class World;
std::array<const Def*, 2> shape_and_body(World&, const Def* def);
const Def* infer_shape(World&, const Def* def);
std::array<const Def*, 2> infer_width_and_shape(World&, const Def*);

class World : public ::thorin::World {
public:
    World();

    //@{ types and type constructors
    const Axiom* type_i() { return type_i_; }
    const Axiom* type_r() { return type_r_; }
    const App* type_i(s64 width) { return type_i(lit_nat(width)); }
    const App* type_r(s64 width) { return type_r(lit_nat(width)); }
    const App* type_i(const Def* width, Debug dbg = {}) { return app(type_i(), width, dbg)->as<App>(); }
    const App* type_r(const Def* width, Debug dbg = {}) { return app(type_r(), width, dbg)->as<App>(); }
    const Axiom* type_mem() { return type_mem_; }
    const Axiom* type_frame() { return type_frame_; }
    const Axiom* type_ptr() { return type_ptr_; }
    const Def* type_ptr(const Def* pointee, Debug dbg = {}) { return type_ptr(pointee, lit_nat_0(), dbg); }
    const Def* type_ptr(const Def* pointee, const Def* addr_space, Debug dbg = {}) {
        return app(type_ptr_, {pointee, addr_space}, dbg);
    }
    //@}

    //@{ @p Lit%erals
    const Lit* lit_i( s8 val) { return lit(type_i( 8), {val}); }
    const Lit* lit_i(s16 val) { return lit(type_i(16), {val}); }
    const Lit* lit_i(s32 val) { return lit(type_i(32), {val}); }
    const Lit* lit_i(s64 val) { return lit(type_i(64), {val}); }
    const Lit* lit_i( u8 val) { return lit(type_i( 8), {val}); }
    const Lit* lit_i(u16 val) { return lit(type_i(16), {val}); }
    const Lit* lit_i(u32 val) { return lit(type_i(32), {val}); }
    const Lit* lit_i(u64 val) { return lit(type_i(64), {val}); }
    const Lit* lit_r(r16 val) { return lit(type_r(16), {val}); }
    const Lit* lit_r(r32 val) { return lit(type_r(32), {val}); }
    const Lit* lit_r(r64 val) { return lit(type_r(64), {val}); }
    //@}

    //@{ arithmetic operations for WOp
    template<WOp o> const Axiom* op() { return wop_[size_t(o)]; }
    template<WOp o> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<o>(WFlags::none, a, b, dbg); }
    template<WOp o> const Def* op(WFlags flags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(flags, width, shape, a, b, dbg);
    }
    template<WOp o> const Def* op(WFlags flags, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(op<o>(), lit_nat(s64(flags))), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ arithmetic operations for MOp
    template<MOp o> const Axiom* op() { return mop_[size_t(o)]; }
    template<MOp o> const Def* op(const Def* m, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(width, shape, m, a, b, dbg);
    }
    template<MOp o> const Def* op(const Def* width, const Def* shape, const Def* m, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(op<o>(), width), shape), {m, a, b}, dbg);
    }
    //@}

    //@{ arithmetic operations for IOp
    template<IOp o> const Axiom* op() { return iop_[size_t(o)]; }
    template<IOp o> const Def* op(const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(width, shape, a, b, dbg);
    }
    template<IOp o> const Def* op(const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(op<o>(), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ arithmetic operations for ROp
    template<ROp o> const Axiom* op() { return rop_[size_t(o)]; }
    template<ROp o> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<o>(RFlags::none, a, b, dbg); }
    template<ROp o> const Def* op(RFlags flags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(flags, width, shape, a, b, dbg);
    }
    template<ROp o> const Def* op(RFlags flags, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(op<o>(), lit_nat(s64(flags))), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ icmp
    template<ICmp o> const Axiom* op() { return icmp_[size_t(o)]; }
    template<ICmp o> const Def* op(const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(width, shape, a, b, dbg);
    }
    template<ICmp o> const Def* op(const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(op<o>(), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ rcmp
    template<RCmp o> const Axiom* op() { return rcmp_[size_t(o)]; }
    template<RCmp o> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<o>(RFlags::none, a, b, dbg); }
    template<RCmp o> const Def* op(RFlags flags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(flags, width, shape, a, b, dbg);
    }
    template<RCmp o> const Def* op(RFlags flags, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(op<o>(), lit_nat(s64(flags))), width), shape), {a, b}, dbg);
    }

    //@{ conversions
    const Axiom* op_scast() const { return op_scast_; }
    const Axiom* op_ucast() const { return op_ucast_; }
    const Axiom* op_rcast() const { return op_rcast_; }
    const Axiom* op_r2s() const { return op_r2s_; }
    const Axiom* op_r2u() const { return op_r2u_; }
    const Axiom* op_s2r() const { return op_s2r_; }
    const Axiom* op_u2r() const { return op_u2r_; }
    //@}

    //@{ lea - load effective address
    const Axiom* op_lea() { return op_lea_; }
    const Def* op_lea(const Def* ptr, const Def* index, Debug dbg = {});
    const Def* op_lea(const Def* ptr, size_t index, Debug dbg = {});
    //@}

    //@{ memory operations
    const Def* op_alloc(const Def* type, const Def* mem, Debug dbg = {});
    const Def* op_alloc(const Def* type, const Def* mem, const Def* extra, Debug dbg = {});
    const Def* op_enter(const Def* mem, Debug dbg = {});
    const Def* op_global(const Def* init, Debug dbg = {});
    const Def* op_global_const(const Def* init, Debug dbg = {});
    const Def* op_load(const Def* mem, const Def* ptr, Debug dbg = {});
    const Def* op_slot(const Def* type, const Def* frame, Debug dbg = {});
    const Def* op_store(const Def* mem, const Def* ptr, const Def* val, Debug dbg = {});
    //@}

    //@{ intrinsics (AKA built-in Cont%inuations)
    const Axiom* branch();
    const Axiom* match();
    const Axiom* pe_info();
    const Axiom* end_scope();
    const Axiom* amdgpu();
    const Axiom* cuda();
    const Axiom* hls();
    const Axiom* nvvm();
    const Axiom* opencl();
    const Axiom* parallel();
    const Axiom* spawn();
    const Axiom* syc();
    const Axiom* vectorize();
    //@}

private:
    const Axiom* type_i_;
    const Axiom* type_r_;
    const Axiom* type_mem_;
    const Axiom* type_frame_;
    const Axiom* type_ptr_;
    std::array<const Axiom*, Num<WOp >> wop_;
    std::array<const Axiom*, Num<MOp >> mop_;
    std::array<const Axiom*, Num<IOp >> iop_;
    std::array<const Axiom*, Num<ROp >> rop_;
    std::array<const Axiom*, Num<ICmp>> icmp_;
    std::array<const Axiom*, Num<RCmp>> rcmp_;
    const Axiom* op_scast_;
    const Axiom* op_ucast_;
    const Axiom* op_rcast_;
    const Axiom* op_r2s_;
    const Axiom* op_r2u_;
    const Axiom* op_s2r_;
    const Axiom* op_u2r_;
    const Axiom* op_enter_;
    const Axiom* op_lea_;
    const Axiom* op_load_;
    const Axiom* op_slot_;
    const Axiom* op_store_;
};

}

#endif
