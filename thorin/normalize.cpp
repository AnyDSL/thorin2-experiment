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

const Def* normalize_tuple(const Def* callee, Defs args, Debug dbg) {
    auto& w = callee->world();

    size_t num = size_t(-1);
    bool foldable = std::all_of(args.begin(), args.end(), [&](const Def* def) {
        if (auto tuple = def->isa<Tuple>()) {
            auto n = tuple->num_ops();
            assert(num == size_t(-1) || n == num);
            num = n;
            return true;
        } else if (def->isa<Pack>()) {
            return true;
        }
        return false;
    });

    if (foldable) {
        auto [head, tail] = shrink_shape(app_arg(callee));
        auto new_callee = w.app(app_callee(callee), tail);
        if (std::all_of(args.begin(), args.end(), [&](const Def* def) { return def->isa<Pack>(); })) {
            auto new_arg = DefArray(args.size(), [&](size_t i) { return args[i]->as<Pack>()->body(); });
            return w.pack(head, w.app(new_callee, new_arg, dbg), dbg);
        }
        auto new_ops = DefArray(num,
                [&](size_t i) { return w.app(new_callee, DefArray(args.size(),
                [&](size_t j) { return args[j]->isa<Pack>() ? args[j]->as<Pack>()->body() : args[j]->as<Tuple>()->op(i); }), dbg); });
        return w.tuple(new_ops, dbg);
    }

    return nullptr;
}

/*
 * normalize
 */

const Def* normalize_arity_succ(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    auto [qualifier, arg_arity] = split(arg);
    if (auto arity = arg_arity->isa<Arity>()) {
        auto arity_val = arity->value();
        return w.arity(qualifier, arity_val + 1, dbg);
    }
    return nullptr;
}

/// normalize any arity eliminator E, dependent as well as recursors to 𝔸, 𝕄, *
const Def* normalize_arity_eliminator(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    const Def* pred = nullptr;
    if (auto arity = arg->isa<Arity>()) {
        auto arity_val = arity->value();
        if (arity_val == 0) {
            // callee = (E q P base f) OR (E q base f) -> get base
            return callee->op(0)->op(1);
        }
        pred = w.arity(arity_val - 1);
    } else if (auto arity_app = arg->isa<App>(); arity_app->callee() == w.arity_succ()) {
        pred = arity_app->arg();
    }
    if (pred != nullptr)
        return w.app(w.app(callee->op(1), pred), w.app(callee, pred), dbg);
    return nullptr;
}

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

    if (auto result = normalize_tuple(callee, {a, b}, dbg)) return result;
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

    if (auto result = normalize_tuple(callee, {arg}, dbg)) return result;
    return reassociate(callee, a, b, dbg);
}

// instantiate templates
#define CODE(T, o) template const Def* normalize_ ## T<T::o>(const Def*, const Def*, Debug);
    THORIN_B_OP(CODE)
    THORIN_N_OP(CODE)
#undef CODE

}
