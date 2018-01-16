#ifndef THORIN_CORE_WORLD_H
#define THORIN_CORE_WORLD_H

#include <memory>
#include <string>

#include "thorin/world.h"
#include "thorin/core/tables.h"

namespace thorin {
namespace core {

class World : public ::thorin::World {
public:
    World();

    //@{ types and type constructors
    const Axiom* type_i() { return type_i_; }
    const Axiom* type_r() { return type_r_; }
    const App* type_i(iflags flags, int64_t width) { return type_i(Qualifier::Unlimited, flags, width); }
    const App* type_r(rflags flags, int64_t width) { return type_r(Qualifier::Unlimited, flags, width); }
    const App* type_i(Qualifier q, iflags flags, int64_t width) {
        auto f = val_nat(int64_t(flags)); auto w = val_nat(width); return type_i(qualifier(q), f, w);
    }
    const App* type_r(Qualifier q, rflags flags, int64_t width) {
        auto f = val_nat(int64_t(flags)); auto w = val_nat(width); return type_r(qualifier(q), f, w);
    }
    const App* type_i(const Def* q, const Def* flags, const Def* width, Debug dbg = {}) {
        return app(type_i(), {q, flags, width}, dbg)->as<App>();
    }
    const App* type_r(const Def* q, const Def* flags, const Def* width, Debug dbg = {}) {
        return app(type_r(), {q, flags, width}, dbg)->as<App>();
    }
    const Axiom* type_mem() { return type_mem_; }
    const Axiom* type_frame() { return type_frame_; }
    const Axiom* type_ptr() { return type_ptr_; }
    const Def* type_ptr(const Def* pointee, Debug dbg = {}) { return type_ptr(pointee, val_nat_0(), dbg); }
    const Def* type_ptr(const Def* pointee, const Def* addr_space, Debug dbg = {}) {
        return app(type_ptr_, {pointee, addr_space}, dbg);
    }
    //@}

    //@{ values
    const Axiom* val(iflags f,  uint8_t val) { return assume(type_i(f,  8), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, uint16_t val) { return assume(type_i(f, 16), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, uint32_t val) { return assume(type_i(f, 32), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, uint64_t val) { return assume(type_i(f, 64), {val}, {std::to_string(val)}); }

    const Axiom* val(iflags f,  int8_t val) { return assume(type_i(f,  8), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, int16_t val) { return assume(type_i(f, 16), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, int32_t val) { return assume(type_i(f, 32), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, int64_t val) { return assume(type_i(f, 64), {val}, {std::to_string(val)}); }

    const Axiom* val(rflags f, half   val) { return assume(type_r(f, 16), {val}, {std::to_string(val)}); }
    const Axiom* val(rflags f, float  val) { return assume(type_r(f, 32), {val}, {std::to_string(val)}); }
    const Axiom* val(rflags f, double val) { return assume(type_r(f, 64), {val}, {std::to_string(val)}); }
    //@}

    //@{ arithmetic operations
    template<iarithop O> const Axiom* op() { return iarithop_[O]; }
    template<rarithop O> const Axiom* op() { return rarithop_[O]; }
    template<iarithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {});
    template<rarithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {});
    //@}

    //@{ relational operations
    const Axiom* op_icmp() { return op_icmp_; }
    const Axiom* op_rcmp() { return op_rcmp_; }
    const Def* op_icmp(irel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_icmp(val_nat(int64_t(rel)), a, b, dbg); }
    const Def* op_rcmp(rrel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_rcmp(val_nat(int64_t(rel)), a, b, dbg); }
    const Def* op_icmp(const Def* rel, const Def* a, const Def* b, Debug dbg = {});
    const Def* op_rcmp(const Def* rel, const Def* a, const Def* b, Debug dbg = {});
    //@}

    //@{ tuple operations
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
    std::array<const Axiom*, Num_IArithOp> iarithop_;
    std::array<const Axiom*, Num_RArithOp> rarithop_;
    const Axiom* op_icmp_;
    const Axiom* op_rcmp_;
};

}
}

#endif
