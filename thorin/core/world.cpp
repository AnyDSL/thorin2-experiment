#include "thorin/core/world.h"

#include "thorin/core/normalize.h"
#include "thorin/frontend/parser.h"

namespace thorin::core {

//------------------------------------------------------------------------------
/*
 * helpers
 */

std::tuple<const Def*, const Def*> shape_and_body(const Def* def) {
    if (auto variadic = def->isa<Variadic>())
        return {variadic->arity(), variadic->body()};
    return {def->world().arity(1), def};
}

const Def* infer_width(const Def* def) {
    auto app  = (def->type()->isa<Variadic>() ? def->as<Variadic>()->body() : def->type())->as<App>();
    return app->arg();
}

std::tuple<const Def*, const Def*> infer_width_and_shape(const Def* def) {
    if (auto variadic = def->type()->isa<Variadic>()) {
        if (!variadic->body()->isa<Variadic>())
            return {variadic->body()->as<App>()->arg(), variadic->arity()};
        std::vector<const Def*> arities;
        const Def* cur = variadic;
        for (; cur->isa<Variadic>(); cur = cur->as<Variadic>()->body())
            arities.emplace_back(cur->as<Variadic>()->arity());
        return {cur->as<App>()->arg(), def->world().sigma(arities)};
    }
    return {def->type()->as<App>()->arg(), def->world().arity(1)};
}

//------------------------------------------------------------------------------

World::World() {
    Env env;
    env["nat"]  = type_nat();
    env["bool"] = type_bool();
    env["int"]  = type_i_ = axiom(parse(*this, "Πnat. *", env), {"int"});
    env["real"] = type_r_ = axiom(parse(*this, "Πnat. *", env), {"real"});
    env["ptr"]  = type_ptr_   = axiom(parse(*this, "Π[*, nat]. *", env), {"ptr"});
    env["M"]    = type_mem_   = axiom(star(QualifierTag::Linear), {"M"});
    env["F"]    = type_frame_ = axiom(star(), {"F"});

    auto type_wop  = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s;  int w] ", env);
    auto type_mop  = parse(*this, "         Πw: nat. Πs: 𝕄. Π[M, [s;  int w], [s;  int w]]. [M, [s;  int w]]", env);
    auto type_iop  = parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s;  int w] ", env);
    auto type_rop  = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; real w], [s; real w]].     [s; real w] ", env);
    auto type_icmp = parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s; bool]", env);
    auto type_rcmp = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; real w], [s; real w]].     [s; bool]", env);

    for (size_t o = 0; o != Num_WOp ; ++o) wop_ [o] = axiom(type_wop,  { op2str( WOp(o))});
    for (size_t o = 0; o != Num_MOp ; ++o) mop_ [o] = axiom(type_mop,  { op2str( MOp(o))});
    for (size_t o = 0; o != Num_IOp ; ++o) iop_ [o] = axiom(type_iop,  { op2str( IOp(o))});
    for (size_t o = 0; o != Num_ROp ; ++o) rop_ [o] = axiom(type_rop,  { op2str( ROp(o))});
    for (size_t o = 0; o != Num_ICmp; ++o) icmp_[o] = axiom(type_icmp, {cmp2str(ICmp(o))});
    for (size_t o = 0; o != Num_RCmp; ++o) rcmp_[o] = axiom(type_rcmp, {cmp2str(RCmp(o))});

    op_scast_ = axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s;  int v]", env));
    op_ucast_ = axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s;  int v]", env));
    op_rcast_ = axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s; real w]. [s; real v]", env));
    op_s2r_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s; real v]", env));
    op_u2r_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s; real v]", env));
    op_r2s_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s; real w]. [s;  int v]", env));
    op_r2u_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s; real w]. [s;  int v]", env));

    op_lea_   = axiom(parse(*this, "Π[s: 𝕄, Ts: [s; *], as: nat]. Π[ptr([j: s; (Ts#j)], as), i: s]. ptr((Ts#i), as)", env), {"lea"});
    op_load_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a)]. [M, T]", env), {"load"});
    op_store_ = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a), T]. M",   env), {"store"});
    op_enter_ = axiom(parse(*this, "ΠM. [M, F]",                               env), {"enter"});
    op_slot_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[F, nat]. ptr(T, a)",    env), {"slot"});

#define CODE(T, o) op<T::o>()->set_normalizer(normalize_ ## T ## o ## _0);
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_R_OP (CODE)
    THORIN_I_CMP(CODE)
    THORIN_R_CMP(CODE)
#undef CODE
}

const Def* World::op_enter(const Def* mem, Debug dbg) {
    return app(op_enter_, mem, dbg);
}

const Def* types_from_tuple_type(World& w, const Def* type) {
    if (auto sig = type->isa<Sigma>()) {
        return w.tuple(sig->ops());
    } else if (auto var = type->isa<Variadic>()) {
        return w.pack(var->arity(), var->body());
    }
    return type;
}

const Def* World::op_lea(const Def* ptr, const Def* index, Debug dbg) {
    auto types = types_from_tuple_type(*this, app_arg(ptr->type(), 0));
    return app(app(op_lea_, {types->arity(), types, app_arg(ptr->type(), 1)}, dbg), {ptr, index}, dbg);
}

const Def* World::op_lea(const Def* ptr, size_t i, Debug dbg) {
    auto types = types_from_tuple_type(*this, app_arg(ptr->type(), 0));
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
