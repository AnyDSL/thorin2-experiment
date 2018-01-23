#ifndef THORIN_CORE_WORLD_H
#define THORIN_CORE_WORLD_H

#include <memory>
#include <string>

#include "thorin/world.h"
#include "thorin/core/tables.h"

namespace thorin::core {

std::tuple<const Def*, const Def*> shape_and_body(const Def* def);
const Def* infer_shape(const Def* def);
const Def* infer_width(const Def* def);
std::tuple<const Def*, const Def*> infer_width_and_shape(const Def*);

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
    template<WOp O> const Axiom* op() { return wop_[O]; }
    template<WOp O> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<O>(WFlags::none, a, b, dbg); }
    template<WOp O> const Def* op(WFlags flags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(a);
        return op<O>(flags, width, shape, a, b, dbg);
    }
    template<WOp O> const Def* op(WFlags flags, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(op<O>(), lit_nat(s64(flags))), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ arithmetic operations for MOp
    template<MOp O> const Axiom* op() { return mop_[O]; }
    template<MOp O> const Def* op(const Def* m, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(a);
        return op<O>(width, shape, m, a, b, dbg);
    }
    template<MOp O> const Def* op(const Def* width, const Def* shape, const Def* m, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(op<O>(), width), shape), {m, a, b}, dbg);
    }
    //@}

    //@{ arithmetic operations for IOp
    template<IOp O> const Axiom* op() { return iop_[O]; }
    template<IOp O> const Def* op(const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(a);
        return op<O>(width, shape, a, b, dbg);
    }
    template<IOp O> const Def* op(const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(op<O>(), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ arithmetic operations for ROp
    template<ROp O> const Axiom* op() { return rop_[O]; }
    template<ROp O> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<O>(RFlags::none, a, b, dbg); }
    template<ROp O> const Def* op(RFlags flags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(a);
        return op<O>(flags, width, shape, a, b, dbg);
    }
    template<ROp O> const Def* op(RFlags flags, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(op<O>(), lit_nat(s64(flags))), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ icmp
    const Axiom* op_icmp() { return op_icmp_; }
    const Def* op_icmp(IRel rel, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(a);
        return op_icmp(rel, width, shape, a, b, dbg);
    }
    const Def* op_icmp(IRel rel, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(op_icmp(), lit_nat(s64(rel))), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ rcmp
    const Axiom* op_rcmp() { return op_rcmp_; }
    const Def* op_rcmp(RRel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_rcmp(RFlags::none, rel, a, b, dbg); }
    const Def* op_rcmp(RFlags flags, RRel rel, const Def* a, const Def* b, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(a);
        return op_rcmp(flags, rel, width, shape, a, b, dbg);
    }
    const Def* op_rcmp(RFlags flags, RRel rel, const Def* width, const Def* shape, const Def* a, const Def* b, Debug dbg = {}) {
        return app(app(app(app(app(op_rcmp(), lit_nat(s64(flags))), lit_nat(s64(rel))), width), shape), {a, b}, dbg);
    }
    //@}

    //@{ conversions
    const Axiom* op_trunc() const { return op_trunc_; }
    const Axiom* op_zext() const { return  op_zext_; }
    const Axiom* op_sext() const { return  op_sext_; }
    const Axiom* op_rtrunc() const { return op_rtrunc_; }
    const Axiom* op_rext() const { return op_rext_; }
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

private:
    const Axiom* type_i_;
    const Axiom* type_r_;
    const Axiom* type_mem_;
    const Axiom* type_frame_;
    const Axiom* type_ptr_;
    std::array<const Axiom*, Num_WArithOp> wop_;
    std::array<const Axiom*, Num_MArithOp> mop_;
    std::array<const Axiom*, Num_IArithOp> iop_;
    std::array<const Axiom*, Num_RArithOp> rop_;
    const Axiom* op_icmp_;
    const Axiom* op_rcmp_;
    const Axiom* op_trunc_;
    const Axiom* op_zext_;
    const Axiom* op_sext_;
    const Axiom* op_rtrunc_;
    const Axiom* op_rext_;
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

inline s64 get_nat(const Def* def) { return def->as<Lit>()->box().get_s64(); }

}

#endif
