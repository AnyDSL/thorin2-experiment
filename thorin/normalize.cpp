#include "thorin/normalize.h"

#include "thorin/world.h"
#include "thorin/transform/reduce.h"

namespace thorin {

/*
 * helpers
 */

std::array<const Def*, 2> shrink_shape(const Def* def) {
    auto& w = def->world();
    if (get_constant_arity(def))
        return {{def, w.lit_arity(1)}};
    if (auto sigma = def->isa<Sigma>())
        return {{sigma->op(0), shift_free_vars(w.sigma(sigma->ops().skip_front()), -1)}};
    auto variadic = def->as<Variadic>();
    return {{variadic->arity(), w.variadic(*get_constant_arity(variadic->arity()) - 1, variadic->body())}};
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
    if (auto arity = get_constant_arity(arg_arity)) {
        return w.lit_arity(qualifier, *arity + 1, dbg);
    }
    return nullptr;
}

const Def* normalize_index_zero(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    auto arg_arity = arg->op(1);
    if (get_constant_arity(arg_arity))
        return w.index_zero(arg_arity, dbg);
    return nullptr;
}

// Πp: [q: ℚ, a: *Aq]. Πa. ASucc p
const Def* normalize_index_succ(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    if (arg->is_term() && is_kind_arity(arg->type()->type())) {
        if (auto index = arg->isa<Lit>()) {
            auto idx = get_index(index);
            auto arity = get_constant_arity(index->type());
            return w.lit_index(w.lit_arity(index->qualifier(), *arity + 1), idx + 1, dbg);
        }
    }
    return nullptr;
}

const Def* normalize_index_eliminator(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    const Def* pred = nullptr;
    if (arg->is_term() && is_kind_arity(arg->type()->type())) {
        // E q P base step a arg
        auto arity = callee->op(1);
        auto elim = callee->op(0);
        auto base = elim->op(0)->op(1);
        auto step = elim->op(1);
        if (auto index = arg->isa<Lit>()) {
            auto index_val = get_index(index);
            if (index_val == 0) {
                // callee = (E q P base step a) -> apply base to a
                return w.app(base, arity);
            }
            pred = w.lit_index(*get_constant_arity(index->type())-1, index_val - 1);
        } else if (auto app = arg->isa<App>()) {
            if (app->callee() == w.index_zero()) {
                // callee = (E q P base step a) -> apply base to a
                return w.app(base, arity);
            } else if (app->callee() == w.index_succ()) {
                pred = arg->op(1);
            }
        }
        if (pred != nullptr) {
            // E q P base step a (IS b i) := step b i (E q P base step b i)
            auto ret_pred =  w.app(w.app(elim, pred->type()), pred);
            auto step_i = w.app(w.app(step, pred->type()), pred);
            return w.app(step_i, ret_pred, dbg);
        }
    }
    return nullptr;
}

/// normalize any arity eliminator E, dependent as well as recursors to *A, *M, *
const Def* normalize_arity_eliminator(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    const Def* pred = nullptr;
    if (auto arity = get_constant_arity(arg)) {
        if (*arity == 0)
            return callee->op(0)->op(1); // callee = (E q P base step) OR (E q base step) -> get base
        pred = w.lit_arity(*arity - 1);
    } else if (auto arity_app = arg->isa<App>(); arity_app->callee() == w.arity_succ()) {
        pred = arity_app->arg();
    }
    if (pred != nullptr)
        return w.app(w.app(callee->op(1), pred), w.app(callee, pred), dbg);
    return nullptr;
}

const Def* normalize_multi_recursor(const Def* callee, const Def* arg, Debug dbg) {
    auto& w = callee->world();
    if (!is_kind_arity(callee->type()->op(1)))
        return nullptr;
    // Recₘ*M base step qual arg
    auto ret = callee->op(0)->op(0)->op(1);
    auto step = callee->op(0)->op(1);
    if (is_kind_arity(arg->type())) {
        ret = w.app(step, w.tuple({ret, arg}), dbg);
    } else if (auto sigma = arg->isa<Sigma>()) {
        for (auto arity : sigma->ops())
            ret = w.app(step, w.tuple({ret, arity}), dbg);
    } else if (auto variadic = arg->isa<Variadic>()) {
        if (auto rank = get_constant_arity(variadic->arity())) {
            // body of variadic must be homogeneous because of normalization and may not depend on var 0
            auto arity = shift_free_vars(variadic->body(), -1);
            for (size_t i = 0; i != rank; ++i)
                ret = w.app(step, w.tuple({ret, arity}), dbg);
        } else
            return nullptr; // rank not constant
    } else
        return nullptr;
    return ret;
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
            case NOp::ndiv: return nb ? (const Def*)world.lit(world.type_nat(), {int64_t(na / nb)}) : world.bot(world.type_nat());
            case NOp::nmod: return nb ? (const Def*)world.lit(world.type_nat(), {int64_t(na % nb)}) : world.bot(world.type_nat());
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
