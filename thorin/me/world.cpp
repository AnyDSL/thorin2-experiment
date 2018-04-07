#include "thorin/me/world.h"

#include "thorin/me/normalize.h"
#include "thorin/fe/parser.h"

namespace thorin::me {

//------------------------------------------------------------------------------
/*
 * helpers
 */

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
    type_f_     = axiom("float", "Πnat. *");
    type_ptr_   = axiom("ptr", "Π[*, nat]. *");
    type_mem_   = axiom(star(QualifierTag::Linear), {"M"});
    type_frame_ = axiom(star(), {"F"});

    auto type_WOp  = fe::parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s;   int w], [s;   int w]].     [s;   int w] ");
    auto type_MOp  = fe::parse(*this, "         Πw: nat. Πs: 𝕄. Π[M, [s;   int w], [s;   int w]]. [M, [s;   int w]]");
    auto type_IOp  = fe::parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;   int w], [s;   int w]].     [s;   int w] ");
    auto type_FOp  = fe::parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; float w], [s; float w]].     [s; float w] ");
    auto type_ICmp = fe::parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;   int w], [s;   int w]].     [s; bool]");
    auto type_FCmp = fe::parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; float w], [s; float w]].     [s; bool]");

#define CODE(T, o) \
    T ## _[size_t(T::o)] = axiom(type_ ## T, normalize_ ## T<T::o>, {op2str(T::o)});
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_F_OP (CODE)
    THORIN_I_CMP(CODE)
    THORIN_F_CMP(CODE)
#undef CODE

    Cast_[size_t(Cast::scast)] = axiom("scast", "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;   int sw]. [s;   int dw]", normalize_Cast<Cast::scast>);
    Cast_[size_t(Cast::ucast)] = axiom("ucast", "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;   int sw]. [s;   int dw]", normalize_Cast<Cast::ucast>);
    Cast_[size_t(Cast::fcast)] = axiom("fcast", "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s; float sw]. [s; float dw]", normalize_Cast<Cast::fcast>);
    Cast_[size_t(Cast::s2f  )] = axiom("s2f",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;   int sw]. [s; float dw]", normalize_Cast<Cast::s2f>);
    Cast_[size_t(Cast::u2f  )] = axiom("u2f",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s;   int sw]. [s; float dw]", normalize_Cast<Cast::u2f>);
    Cast_[size_t(Cast::f2s  )] = axiom("f2s",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s; float sw]. [s;   int dw]", normalize_Cast<Cast::f2s>);
    Cast_[size_t(Cast::f2u  )] = axiom("f2u",   "Π[dw: nat, sw: nat]. Πs: 𝕄. Π[s; float sw]. [s;   int dw]", normalize_Cast<Cast::f2u>);

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
