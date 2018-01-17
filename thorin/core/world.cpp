#include "thorin/core/world.h"

#include "thorin/core/normalize.h"
#include "thorin/frontend/parser.h"

namespace thorin {
namespace core {

World::World() {
    Env env;
    env["nat"]  = type_nat();
    env["bool"] = type_bool();
    env["int"]  = type_i_ = axiom(parse(*this, "Π[q: ℚ, nat]. *q", env), {"int"});
    env["real"] = type_r_ = axiom(parse(*this, "Π[q: ℚ, nat]. *q", env), {"real"});
    env["ptr"]  = type_ptr_   = axiom(parse(*this, "Π[*, nat]. *", env), {"ptr"});
    env["M"]    = type_mem_   = axiom(star(QualifierTag::Linear), {"M"});
    env["F"]    = type_frame_ = axiom(star(), {"F"});

    auto w_type_arithop = parse(*this, "Πf: nat. Πs: 𝕄. Π[q: ℚ, w: nat]. Π[   [s;  int(q, w)], [s;  int(q, w)]].     [s;  int(q, w)] ", env);
    auto m_type_arithop = parse(*this, "         Πs: 𝕄. Π[q: ℚ, w: nat]. Π[M, [s;  int(q, w)], [s;  int(q, w)]]. [M, [s;  int(q, w)]]", env);
    auto i_type_arithop = parse(*this, "         Πs: 𝕄. Π[q: ℚ, w: nat]. Π[   [s;  int(q, w)], [s;  int(q, w)]].     [s;  int(q, w)] ", env);
    auto r_type_arithop = parse(*this, "Πf: nat. Πs: 𝕄. Π[q: ℚ, w: nat]. Π[   [s; real(q, w)], [s; real(q, w)]].     [s; real(q, w)] ", env);

    for (size_t o = 0; o != Num_WArithOp; ++o) warithop_[o] = axiom(w_type_arithop, {arithop2str(WArithop(o))});
    for (size_t o = 0; o != Num_MArithOp; ++o) marithop_[o] = axiom(m_type_arithop, {arithop2str(MArithop(o))});
    for (size_t o = 0; o != Num_IArithOp; ++o) iarithop_[o] = axiom(i_type_arithop, {arithop2str(IArithop(o))});
    for (size_t o = 0; o != Num_RArithOp; ++o) rarithop_[o] = axiom(r_type_arithop, {arithop2str(RArithop(o))});

    //auto type_icmp = parse(*this, "Πrel: nat. Πs: 𝕄. Π[q: ℚ, w: nat]. Π[[s;  int(q, w)], [s;  int(q, w)]]. [s; bool]", env);
    auto type_rcmp = parse(*this, "Πf: nat. Πrel: nat. Πs: 𝕄. Π[q: ℚ, w: nat]. Π[[s; real(q, w)], [s; real(q, w)]]. [s; bool]", env);
    //op_icmp_  = axiom(type_icmp, {"icmp"});
    op_rcmp_  = axiom(type_rcmp, {"rcmp"});
    op_lea_   = axiom(parse(*this, "Π[s: 𝕄, Ts: [s; *], as: nat]. Π[ptr([j: s; (Ts#j)], as), i: s]. ptr((Ts#i), as)", env), {"lea"});
    op_load_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a)]. [M, T]", env), {"load"});
    op_store_ = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a), T]. M",   env), {"store"});
    op_enter_ = axiom(parse(*this, "ΠM. [M, F]",                               env), {"enter"});
    op_slot_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[F, nat]. ptr(T, a)",    env), {"slot"});

    op<iadd>()->set_normalizer(normalize_iadd_flags);
}

std::tuple<const Def*, const Def*> shape_and_body(const Def* def) {
    if (auto variadic = def->isa<Variadic>())
        return {variadic->arity(), variadic->body()};
    return {def->world().arity(1), def};
}

//const Def* World::op_icmp(const Def* rel, const Def* a, const Def* b, Debug dbg) {
    //auto [shape, body] = shape_and_body(a->type());
    //return app(app(app(app(op_icmp(), rel), shape), app_arg(body)), {a, b}, dbg);
//}

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
    return app(app(op_slot_, {type, val_nat_0()}, dbg), {frame, val_nat(Def::gid_counter())}, dbg);
}

const Def* World::op_store(const Def* mem, const Def* ptr, const Def* val, Debug dbg) {
    return app(app(op_store_, ptr->type()->as<App>()->arg(), dbg), {mem, ptr, val}, dbg);
}

}
}
