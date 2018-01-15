#include "thorin/core/world.h"

#include "thorin/frontend/parser.h"

namespace thorin {
namespace core {

World::World() {
    auto Q = qualifier_type();
    auto S = star();
    auto N = type_nat();

    auto sigQNN = sigma({Q, N, N});
    type_i_ = axiom(pi(sigQNN, star(extract(var(sigQNN, 0), 0))), {"int" });
    type_r_ = axiom(pi(sigQNN, star(extract(var(sigQNN, 0), 0))), {"real"});

    Env env;
    env["nat"]  = type_nat();
    env["bool"] = type_bool();
    env["ptr"]  = type_ptr_   = axiom(parse(*this, "Π[*, nat]. *", env), {"ptr"});
    env["M"]    = type_mem_   = axiom(star(Qualifier::Linear), {"M"});
    env["F"]    = type_frame_ = axiom(S, {"F"});
    env["int"]  = type_i();
    env["real"] = type_r();

    auto i_type_arithop = parse(*this, "Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[[s;  int(q, f, w)], [s;  int(q, f, w)]]. [s;  int(q, f, w)]", env);
    auto r_type_arithop = parse(*this, "Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[[s; real(q, f, w)], [s; real(q, f, w)]]. [s; real(q, f, w)]", env);

    for (size_t o = 0; o != Num_IArithOp; ++o) iarithop_[o] = axiom(i_type_arithop, {iarithop2str(iarithop(o))});
    for (size_t o = 0; o != Num_RArithOp; ++o) rarithop_[o] = axiom(r_type_arithop, {rarithop2str(rarithop(o))});

    auto i_type_cmp = parse(*this, "Πrel: nat. Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[[s;  int(q, f, w)], [s;  int(q, f, w)]]. [s; bool]", env);
    auto r_type_cmp = parse(*this, "Πrel: nat. Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[[s; real(q, f, w)], [s; real(q, f, w)]]. [s; bool]", env);
    op_icmp_  = axiom(i_type_cmp, {"icmp"});
    op_rcmp_  = axiom(r_type_cmp, {"rcmp"});
    op_lea_   = axiom(parse(*this, "Π[s: 𝕄, Ts: [s; *], as: nat]. Π[ptr([j: s; (Ts#j)], as), i: s]. ptr((Ts#i), as)", env), {"lea"});
    op_load_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a)]. [M, T]", env), {"load"});
    op_store_ = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a), T]. M",   env), {"store"});
    op_enter_ = axiom(parse(*this, "ΠM. [M, F]",                               env), {"enter"});
    op_slot_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[F, nat]. ptr(T, a)",    env), {"slot"});
}

static std::tuple<const Def*, const Def*> shape_and_body(const Def* def) {
    if (auto variadic = def->isa<Variadic>())
        return {variadic->arity(), variadic->body()};
    return {def->world().arity(1), def};
}

template<iarithop O>
const Def* World::op(const Def* a, const Def* b, Debug dbg) {
    auto [shape, body] = shape_and_body(a->type());
    return app(app(app(op<O>(), shape), app_arg(body)), {a, b}, dbg);
}

template<rarithop O>
const Def* World::op(const Def* a, const Def* b, Debug dbg) {
    auto [shape, body] = shape_and_body(a->type());
    return app(app(app(op<O>(), shape), app_arg(body)), {a, b}, dbg);
}

const Def* World::op_icmp(const Def* rel, const Def* a, const Def* b, Debug dbg) {
    auto [shape, body] = shape_and_body(a->type());
    return app(app(app(app(op_icmp(), rel), shape), app_arg(body)), {a, b}, dbg);
}
const Def* World::op_rcmp(const Def* rel, const Def* a, const Def* b, Debug dbg) {
    auto [shape, body] = shape_and_body(a->type());
    return app(app(app(app(op_rcmp(), rel), shape), app_arg(body)), {a, b}, dbg);
}

#define CODE(O) \
    template const Def* World::op<O>(const Def*, const Def*, Debug);
THORIN_I_ARITHOP(CODE)
THORIN_R_ARITHOP(CODE)
#undef CODE

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
    auto idx = index(types->arity()->as<Axiom>()->box().get_u64(), i);
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
