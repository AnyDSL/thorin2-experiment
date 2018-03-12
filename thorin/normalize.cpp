#include "thorin/normalize.h"

#include "thorin/world.h"
#include "thorin/transform/reduce.h"

namespace thorin {

/*
 * helpers
 */

std::array<const Def*, 2> shrink_shape(const Def* def) {
    auto& w = def->world();
    if (def->isa<Arity>())
        return {{def, w.arity(1)}};
    if (auto sigma = def->isa<Sigma>())
        return {{sigma->op(0), shift_free_vars(w.sigma(sigma->ops().skip_front()), -1)}};
    auto variadic = def->as<Variadic>();
    return {{variadic->arity(), w.variadic(variadic->arity()->as<Arity>()->value() - 1, variadic->body())}};
}

std::array<const Def*, 2> split(const Def* def) {
    auto& w = def->world();
    auto a = w.extract(def, 0_u64);
    auto b = w.extract(def, 1_u64);
    return {{a, b}};
}

bool is_foldable(const Def* def) { return def->isa<Lit>() || def->isa<Tuple>() || def->isa<Pack>(); }

const Lit* foldable_to_left(const Def*& a, const Def*& b) {
    if (is_foldable(b))
        std::swap(a, b);

    return a->isa<Lit>();
}

const Def* commute(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    if (a->gid() > b->gid() && !a->isa<Lit>())
        return w.raw_app(callee, {b, a}, dbg);
    return w.raw_app(callee, {a, b}, dbg);
}

const Def* reassociate(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    std::array<const Def*, 2> args{{a, b}};
    std::array<std::array<const Def*, 2>, 2> aa{{{{nullptr, nullptr}}, {{nullptr, nullptr}}}};
    std::array<bool, 2> foldable{{false, false}};

    for (size_t i = 0; i != 2; ++i) {
        if (auto app = args[i]->isa<App>()) {
            aa[i] = split(app->arg());
            foldable[i] = is_foldable(aa[i][0]);
        }
    }

    if (foldable[0] && foldable[1]) {
        auto f = w.app(callee, {aa[0][0], aa[1][0]}, dbg);
        return w.app(callee, {f, w.app(callee, {aa[0][1], aa[1][1]}, dbg)}, dbg);
    }

    for (size_t i = 0; i != 2; ++i) {
        if (foldable[i])
            return w.app(callee, {aa[i][0], w.app(callee, {args[1-i], aa[i][1]}, dbg)}, dbg);
    }

    return commute(callee, a, b, dbg);
}

// TODO merge this with the other overload
const Def* normalize_tuple(const Def* callee, const Def* a, Debug dbg) {
    auto& w = callee->world();
    auto ta = a->isa<Tuple>();
    auto pa = a->isa<Pack>();

    if (ta || pa) {
        auto [head, tail] = shrink_shape(app_arg(callee));
        auto new_callee = w.app(app_callee(callee), tail);

        if (ta) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, ta->op(i), dbg); }));
        assert(pa);
        return w.pack(head, w.app(new_callee, pa->body(), dbg), dbg);
    }

    return nullptr;
}

const Def* normalize_tuple(const Def* callee, const Def* a, const Def* b, Debug dbg) {
    auto& w = callee->world();
    auto ta = a->isa<Tuple>(), tb = b->isa<Tuple>();
    auto pa = a->isa<Pack>(),  pb = b->isa<Pack>();

    if ((ta || pa) && (tb || pb)) {
        auto [head, tail] = shrink_shape(app_arg(callee));
        auto new_callee = w.app(app_callee(callee), tail);

        if (ta && tb) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, {ta->op(i),  tb->op(i)},  dbg); }));
        if (ta && pb) return w.tuple(DefArray(ta->num_ops(), [&](auto i) { return w.app(new_callee, {ta->op(i),  pb->body()}, dbg); }));
        if (pa && tb) return w.tuple(DefArray(tb->num_ops(), [&](auto i) { return w.app(new_callee, {pa->body(), tb->op(i)},  dbg); }));
        assert(pa && pb);
        return w.pack(head, w.app(new_callee, {pa->body(), pb->body()}, dbg), dbg);
    }

    return nullptr;
}

/*
 * normalize
 */

template<BOp op>
const Def* normalize_BOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);

    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto ba = la->box().get_bool(), bb = lb->box().get_bool();
        switch (op) {
            case BOp::band: return world.lit(world.type_bool(), {ba & bb});
            case BOp:: bor: return world.lit(world.type_bool(), {ba | bb});
            case BOp::bxor: return world.lit(world.type_bool(), {ba ^ bb});
        }
    }

    if (auto la = foldable_to_left(a, b)) {
        if (is_zero(la)) {
            switch (op) {
                case BOp::band: return la;
                case BOp::bor:  return b;
                default: THORIN_UNREACHABLE;
            }
        }

        if (is_allset(la)) {
            switch (op) {
                case BOp::band: return b;
                case BOp::bor:  return la;
                default: THORIN_UNREACHABLE;
            }
        }
    }

    return reassociate(callee, a, b, dbg);
}

template<NOp op>
const Def* normalize_NOp(const Def* callee, const Def* arg, Debug dbg) {
    auto& world = static_cast<World&>(callee->world());

    auto [a, b] = split(arg);

    auto la = a->isa<Lit>(), lb = b->isa<Lit>();
    if (la && lb) {
        auto na = get_nat(la), nb = get_nat(lb);
        switch (op) {
            case NOp::nadd: return world.lit(world.type_nat(), {int64_t(na + nb)});
            case NOp::nsub: return world.lit(world.type_nat(), {int64_t(na - nb)});
            case NOp::nmul: return world.lit(world.type_nat(), {int64_t(na * nb)});
            case NOp::ndiv: return nb ? (const Def*)world.lit(world.type_nat(), {int64_t(na / nb)}) : world.bottom(world.type_nat());
            case NOp::nmod: return nb ? (const Def*)world.lit(world.type_nat(), {int64_t(na % nb)}) : world.bottom(world.type_nat());
        }
    }

    return reassociate(callee, a, b, dbg);
}

// instantiate templates
#define CODE(T, o) template const Def* normalize_ ## T<T::o>(const Def*, const Def*, Debug);
    THORIN_B_OP(CODE)
    THORIN_N_OP(CODE)
#undef CODE

}
