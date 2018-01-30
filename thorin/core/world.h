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
    template<class I> const Lit* lit_i(I val) {
        static_assert(std::is_integral<I>());
        return lit(type_i(sizeof(I)*8), {val});
    }
    template<class R> const Lit* lit_r(R val) {
        static_assert(std::is_floating_point<R>() || std::is_same<R, r16>());
        return lit(type_r(sizeof(R)*8), {val});
    }
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

    //@{ cast
    template<Cast o> const Axiom* op() const { return cast_[size_t(o)]; }
    template<Cast o> const Def* op(s64 dst_width, const Def* a, Debug dbg = {}) { return op<o>(lit_nat(dst_width), a, dbg); }
    template<Cast o> const Def* op(const Def* dst_width, const Def* a, Debug dbg = {}) {
        auto [width, shape] = infer_width_and_shape(*this, a);
        return op<o>(dst_width, width, shape, a, dbg);
    }
    template<Cast o> const Def* op(const Def* dst_width, const Def* width, const Def* shape, const Def* a, Debug dbg = {}) {
        return app(app(app(op<o>(), {dst_width, width}), shape), a, dbg);
    }
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
    const Axiom* cn_amdgpu();
    const Axiom* cn_cuda();
    const Axiom* cn_hls();
    const Axiom* cn_nvvm();
    const Axiom* cn_opencl();
    const Axiom* cn_parallel();
    const Axiom* cn_spawn();
    const Axiom* cn_syc();
    const Axiom* cn_vectorize();
    //@}

    //@{ array operations
    const Axiom* op_map();
    const Axiom* op_fold();
    const Axiom* op_reduce();
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
    std::array<const Axiom*, Num<Cast>> cast_;
    const Axiom* op_enter_;
    const Axiom* op_lea_;
    const Axiom* op_load_;
    const Axiom* op_slot_;
    const Axiom* op_store_;
    const Axiom* op_map_;
    const Axiom* op_fold_;
    const Axiom* op_reduce_;
    const Axiom* cn_pe_info_;
    const Axiom* cn_amdgpu_;
    const Axiom* cn_cuda_;
    const Axiom* cn_hls_;
    const Axiom* cn_nvvm_;
    const Axiom* cn_opencl_;
    const Axiom* cn_parallel_;
    const Axiom* cn_spawn_;
    const Axiom* cn_syc_;
    const Axiom* cn_vectorize_;
};

}

#endif
