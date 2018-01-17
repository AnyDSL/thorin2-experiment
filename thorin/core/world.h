#ifndef THORIN_CORE_WORLD_H
#define THORIN_CORE_WORLD_H

#include <memory>
#include <string>

#include "thorin/world.h"
#include "thorin/core/tables.h"

namespace thorin {
namespace core {

std::tuple<const Def*, const Def*> shape_and_body(const Def* def);

class World : public ::thorin::World {
public:
    World();

    //@{ types and type constructors
    const Axiom* type_i() { return type_i_; }
    const Axiom* type_r() { return type_r_; }
    const App* type_i(int64_t width) { return type_i(QualifierTag::Unlimited, width); }
    const App* type_r(int64_t width) { return type_r(QualifierTag::Unlimited, width); }
    const App* type_i(QualifierTag q, int64_t width) { return type_i(qualifier(q), val_nat(width)); }
    const App* type_r(QualifierTag q, int64_t width) { return type_r(qualifier(q), val_nat(width)); }
    const App* type_i(const Def* q, const Def* width, Debug dbg = {}) { return app(type_i(), {q, width}, dbg)->as<App>(); }
    const App* type_r(const Def* q, const Def* width, Debug dbg = {}) { return app(type_r(), {q, width}, dbg)->as<App>(); }
    const Axiom* type_mem() { return type_mem_; }
    const Axiom* type_frame() { return type_frame_; }
    const Axiom* type_ptr() { return type_ptr_; }
    const Def* type_ptr(const Def* pointee, Debug dbg = {}) { return type_ptr(pointee, val_nat_0(), dbg); }
    const Def* type_ptr(const Def* pointee, const Def* addr_space, Debug dbg = {}) {
        return app(type_ptr_, {pointee, addr_space}, dbg);
    }
    //@}

    //@{ values
    const Axiom* val(  int8_t val) { return assume(type_i( 8), {val}); }
    const Axiom* val( uint8_t val) { return assume(type_i( 8), {val}); }
    const Axiom* val( int16_t val) { return assume(type_i(16), {val}); }
    const Axiom* val(uint16_t val) { return assume(type_i(16), {val}); }
    const Axiom* val( int32_t val) { return assume(type_i(32), {val}); }
    const Axiom* val(uint32_t val) { return assume(type_i(32), {val}); }
    const Axiom* val( int64_t val) { return assume(type_i(64), {val}); }
    const Axiom* val(uint64_t val) { return assume(type_i(64), {val}); }

    const Axiom* val(half   val) { return assume(type_r(16), {val}); }
    const Axiom* val(float  val) { return assume(type_r(32), {val}); }
    const Axiom* val(double val) { return assume(type_r(64), {val}); }
    //@}

    //@{ arithmetic operations
    template<WArithop O> const Axiom* op() { return warithop_[O]; }
    template<MArithop O> const Axiom* op() { return marithop_[O]; }
    template<IArithop O> const Axiom* op() { return iarithop_[O]; }
    template<RArithop O> const Axiom* op() { return rarithop_[O]; }
    template<WArithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<O>(WFlags::none, a, b, dbg); }
    template<WArithop O> const Def* op(WFlags wflags, const Def* a, const Def* b, Debug dbg = {}) { return op<O>(val_nat(int64_t(wflags)), a, b, dbg); }
    template<WArithop O> const Def* op(const Def* wflags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [shape, body] = shape_and_body(a->type());
        return app(app(app(app(op<O>(), wflags), app_arg(body)), shape), {a, b}, dbg);
    }
    template<MArithop O> const Def* op(const Def* m, const Def* a, const Def* b, Debug dbg = {}) {
        auto [shape, body] = shape_and_body(a->type());
        return app(app(app(op<O>(), app_arg(body)), shape), {m, a, b}, dbg);
    }
    template<IArithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {}) {
        auto [shape, body] = shape_and_body(a->type());
        return app(app(app(op<O>(), app_arg(body)), shape), {a, b}, dbg);
    }
    template<RArithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {}) { return op<O>(RFlags::none, a, b, dbg); }
    template<RArithop O> const Def* op(RFlags f, const Def* a, const Def* b, Debug dbg = {}) { return op<O>(val_nat(int64_t(f)), a, b, dbg); }
    template<RArithop O> const Def* op(const Def* rflags, const Def* a, const Def* b, Debug dbg = {}) {
        auto [shape, body] = shape_and_body(a->type());
        return app(app(app(app(op<O>(), rflags), app_arg(body)), shape), {a, b}, dbg);
    }
    //@}

    //@{ relational operations
    const Axiom* op_icmp() { return op_icmp_; }
    const Axiom* op_rcmp() { return op_rcmp_; }
    //const Def* op_icmp(irel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_icmp(val_nat(int64_t(rel)), a, b, dbg); }
    const Def* op_rcmp(RFlags f, RRel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_rcmp(val_nat(int64_t(f)), val_nat(int64_t(rel)), a, b, dbg); }
    const Def* op_rcmp(RRel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_rcmp(RFlags::none, rel, a, b, dbg); }
    //const Def* op_icmp(const Def* rel, const Def* a, const Def* b, Debug dbg = {});
    const Def* op_rcmp(const Def* rflags, const Def* rel, const Def* a, const Def* b, Debug dbg = {}) {
        auto [shape, body] = shape_and_body(a->type());
        return app(app(app(app(app(op_rcmp(), rflags), rel), shape), app_arg(body)), {a, b}, dbg);
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

private:
    const Axiom* type_i_;
    const Axiom* type_r_;
    const Axiom* type_mem_;
    const Axiom* type_frame_;
    const Axiom* type_ptr_;
    const Axiom* op_enter_;
    const Axiom* op_lea_;
    const Axiom* op_load_;
    const Axiom* op_slot_;
    const Axiom* op_store_;
    std::array<const Axiom*, Num_WArithOp> warithop_;
    std::array<const Axiom*, Num_MArithOp> marithop_;
    std::array<const Axiom*, Num_IArithOp> iarithop_;
    std::array<const Axiom*, Num_RArithOp> rarithop_;
    const Axiom* op_icmp_;
    const Axiom* op_rcmp_;
};

}
}

#endif
