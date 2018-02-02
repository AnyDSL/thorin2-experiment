#include "thorin/core/world.h"

#include "thorin/core/normalize.h"
#include "thorin/frontend/parser.h"

namespace thorin::core {

//------------------------------------------------------------------------------
/*
 * helpers
 */

std::array<const Def*, 2> shape_and_body(World& world, const Def* def) {
    if (auto variadic = def->isa<Variadic>())
        return {{variadic->arity(), variadic->body()}};
    return {{world.arity(1), def}};
}

std::array<const Def*, 2> infer_width_and_shape(World& world, const Def* def) {
    if (auto variadic = def->type()->isa<Variadic>()) {
        if (!variadic->body()->isa<Variadic>())
            return {{variadic->body()->as<App>()->arg(), variadic->arity()}};
        std::vector<const Def*> arities;
        const Def* cur = variadic;
        for (; cur->isa<Variadic>(); cur = cur->as<Variadic>()->body())
            arities.emplace_back(cur->as<Variadic>()->arity());
        return {{cur->as<App>()->arg(), world.sigma(arities)}};
    }
    return {{def->type()->as<App>()->arg(), world.arity(1)}};
}

//------------------------------------------------------------------------------

World::World(Debug dbg)
    : thorin::World(dbg)
{
    type_i_     = axiom("int", "Πnat. *");
    type_r_     = axiom("real", "Πnat. *");
    type_ptr_   = axiom("ptr", "Π[*, nat]. *");
    type_mem_   = axiom(star(QualifierTag::Linear), {"M"});
    type_frame_ = axiom(star(), {"F"});

    auto type_WOp  = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s;  int w] ");
    auto type_MOp  = parse(*this, "         Πw: nat. Πs: 𝕄. Π[M, [s;  int w], [s;  int w]]. [M, [s;  int w]]");
    auto type_IOp  = parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s;  int w] ");
    auto type_ROp  = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; real w], [s; real w]].     [s; real w] ");
    auto type_ICmp = parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s; bool]");
    auto type_RCmp = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; real w], [s; real w]].     [s; bool]");

    std::array<Array<const char*>, Num<WOp>> rules_WOp;
    rules_WOp[size_t(WOp::add)] = {
        "[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> x",
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, x) -> mul f w x 1ₐ ({2u64: int w}, x)",
    };
    rules_WOp[size_t(WOp::sub)] = {
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, {0u64: int w}) -> x",
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, x) -> {0u64: int w})",
    };
    rules_WOp[size_t(WOp::mul)] = {
        "[f: nat, w: nat, x: int w]. mul f w 1ₐ ({1u64: int w}, x) -> x",
        "[f: nat, w: nat, x: int w]. mul f w 1ₐ ({0u64: int w}, x) -> {0u64: int w}",
    };
    rules_WOp[size_t(WOp::shl)] = {
        "[f: nat, w: nat, x: int w]. add f w 1ₐ ({0u64: int w}, x) -> {0u64: int w}",
        "[f: nat, w: nat, x: int w]. add f w 1ₐ (x, {0u64: int w}) -> x",
    };

    std::array<Array<const char*>, Num<MOp>> rules_MOp;
    rules_MOp[size_t(MOp::sdiv)] = {};
    rules_MOp[size_t(MOp::udiv)] = {};
    rules_MOp[size_t(MOp::smod)] = {};
    rules_MOp[size_t(MOp::umod)] = {};

    std::array<Array<const char*>, Num<ICmp>> rules_ICmp;
    rules_ICmp[size_t(ICmp::t)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_t f w (x, y) -> true",
    };
    rules_ICmp[size_t(ICmp::f)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_f f w (x, y) -> false",
    };
    rules_ICmp[size_t(ICmp::sge)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_sge  f w (x, y) -> icmp_sle  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::sgt)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_sgt  f w (x, y) -> icmp_slt  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::ugt)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_ugt  f w (x, y) -> icmp_ult  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::uge)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_uge  f w (x, y) -> icmp_ule  f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::sugt)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_sugt f w (x, y) -> icmp_sult f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::suge)] = {
        "[w: nat, s: 𝕄, x: int w, y: int w]. icmp_suge f w (x, y) -> icmp_sule f w (y, x)",
    };
    rules_ICmp[size_t(ICmp::eq)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_eq   f w (x, x) -> true",
    };
    rules_ICmp[size_t(ICmp::ne)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_ne   f w (x, x) -> false",
    };
    rules_ICmp[size_t(ICmp::sle)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_sle  f w (x, x) -> true",
    };
    rules_ICmp[size_t(ICmp::ule)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_ule  f w (x, x) -> true",
    };
    rules_ICmp[size_t(ICmp::sule)] = {
        "[w: nat, s: 𝕄, x: int w]. icmp_sule f w (x, x) -> true",
    };
    // iand
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ ({0u64: int w}, x) -> {0u64: int w}");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ ({-1us64: int w}, x) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (x, x) -> x");
    // ior
    rule("[f: nat, w: nat, x: int w]. ior f w 1ₐ ({0u64: int w}, x) -> x");
    rule("[f: nat, w: nat, x: int w]. ior f w 1ₐ ({-1us64: int w}, x) -> {0u64: int w}");
    rule("[f: nat, w: nat, x: int w]. ior f w 1ₐ (x, x) -> x");
    // ixor
    rule("[f: nat, w: nat, x: int w]. ixor f w 1ₐ (x, x) -> {0u64: int w}");
    rule("[f: nat, w: nat, x: int w]. ixor f w 1ₐ ({-1s64: int w}, (ixor f w 1ₐ ({-1s64: int w}, x))) -> x"); // double negation
    // absorption
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (x, ior  f w 1ₐ (x, y)) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (x, iand f w 1ₐ (x, y)) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (x, ior  f w 1ₐ (y, x)) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (x, iand f w 1ₐ (y, x)) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (ior  f w 1ₐ (x, y), x) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (iand f w 1ₐ (x, y), x) -> x");
    rule("[f: nat, w: nat, x: int w]. iand f w 1ₐ (ior  f w 1ₐ (y, x), x) -> x");
    rule("[f: nat, w: nat, x: int w]. ior  f w 1ₐ (iand f w 1ₐ (y, x), x) -> x");

    // switch over to this thing when rule system works
#define CODE(T, o) {                                                                    \
        auto num = rules_ ## T[size_t(T::o)].size();                                    \
        auto a = axiom(type_ ## T, num, normalize_ ## o, {op2str(T::o)});               \
        for (size_t i = 0; i != num; ++i)                                               \
            a->set(i, rule(rules_ ## T[size_t(T::o)][i]));                              \
        T ## _[size_t(T::o)] = a;                                                       \
    }
#undef CODE
#define CODE(T, o) \
    T ## _[size_t(T::o)] = axiom(type_ ## T, 0, normalize_ ## o, {op2str(T::o)});
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_R_OP (CODE)
#undef CODE

#define CODE(T, o) \
    T ## _[size_t(T::o)] = axiom(type_ ## T, 0, normalize_ ## T<T::o>, {op2str(T::o)});
    THORIN_I_CMP(CODE)
    THORIN_R_CMP(CODE)
#undef CODE

    Cast_[size_t(Cast::scast)] = axiom("scast", "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;  int sw]. [s;  int dw]", 0, normalize_scast);
    Cast_[size_t(Cast::ucast)] = axiom("ucast", "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;  int sw]. [s;  int dw]", 0, normalize_ucast);
    Cast_[size_t(Cast::rcast)] = axiom("rcast", "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s; real sw]. [s; real dw]", 0, normalize_rcast);
    Cast_[size_t(Cast::s2r  )] = axiom("s2r",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;  int sw]. [s; real dw]", 0, normalize_s2r);
    Cast_[size_t(Cast::u2r  )] = axiom("u2r",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;  int sw]. [s; real dw]", 0, normalize_u2r);
    Cast_[size_t(Cast::r2s  )] = axiom("r2s",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s; real sw]. [s;  int dw]", 0, normalize_r2s);
    Cast_[size_t(Cast::r2u  )] = axiom("r2u",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s; real sw]. [s;  int dw]", 0, normalize_r2u);

    op_lea_   = axiom("lea",   "Π[s: 𝕄, Ts: [s; *], as: nat]. Π[ptr([j: s; Ts#j], as), i: s]. ptr(Ts#i, as)");
    op_load_  = axiom("load",  "Π[T: *, a: nat]. Π[M, ptr(T, a)]. [M, T]");
    op_store_ = axiom("store", "Π[T: *, a: nat]. Π[M, ptr(T, a), T]. M");
    op_enter_ = axiom("enter", "ΠM. [M, F]");
    op_slot_  = axiom("slot",  "Π[T: *, a: nat]. Π[F, nat]. ptr(T, a)");

    cn_pe_info_ = axiom("pe_info", "cn[T: *, ptr(int {8s64: nat}, {0s64: nat}), T, cn[]]");
}

const Def* World::op_enter(const Def* mem, Debug dbg) {
    return app(op_enter_, mem, dbg);
}

const Def* types_from_tuple_type(const Def* type) {
    auto& w = type->world();
    if (auto sig = type->isa<Sigma>()) {
        return w.tuple(sig->ops());
    } else if (auto var = type->isa<Variadic>()) {
        return w.pack(var->arity(), var->body());
    }
    return type;
}

const Def* World::op_lea(const Def* ptr, const Def* index, Debug dbg) {
    auto types = types_from_tuple_type(app_arg(ptr->type(), 0));
    return app(app(op_lea_, {types->arity(), types, app_arg(ptr->type(), 1)}, dbg), {ptr, index}, dbg);
}

const Def* World::op_lea(const Def* ptr, size_t i, Debug dbg) {
    auto types = types_from_tuple_type(app_arg(ptr->type(), 0));
    auto idx = index(types->arity()->as<Arity>()->value(), i);
    return app(app(op_lea_, {types->arity(), types, app_arg(ptr->type(), 1)}, dbg), {ptr, idx}, dbg);
}

const Def* World::op_load(const Def* mem, const Def* ptr, Debug dbg) {
    return app(app(op_load_, ptr->type()->as<App>()->arg(), dbg), {mem, ptr}, dbg);
}

const Def* World::op_slot(const Def* type, const Def* frame, Debug dbg) {
    return app(app(op_slot_, {type, lit_nat_0()}, dbg), {frame, lit_nat(Def::gid_counter())}, dbg);
}

const Def* World::op_store(const Def* mem, const Def* ptr, const Def* val, Debug dbg) {
    return app(app(op_store_, ptr->type()->as<App>()->arg(), dbg), {mem, ptr, val}, dbg);
}

}
