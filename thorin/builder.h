#ifndef THORIN_BUILDER_H
#define THORIN_BUILDER_H

#define BVAR(n, T) typedef BVar<decltype(#n ## _var), T> n

#include <utility>

#include "thorin/world.h"

namespace thorin {

template <char... chars>
using VarName = std::integer_sequence<char, chars...>;

template<class T, T... chars>
constexpr VarName<chars...> operator""_var() { return {}; }

template <typename>
struct StrGetter;

template<char... chars>
struct StrGetter<VarName<chars...>> {
    const char* GetString() const {
        static constexpr char str[sizeof...(chars) + 1] = { chars..., '\0' };
        return str;
    }
};

struct Builder {
    static World& world() { return *world_; }
    static World* world_;
    static int depth;
};

World* Builder::world_ = nullptr;
int Builder::depth = 0;

struct BStar {
    BStar() {}
    static const Def* emit() {
        return Builder::world().star();
    }
};

template<class... Ops>
struct BDecomposeHelper {
    static void decompose(std::vector<const Def*>&) {}
};

template<class Head, class... Tail>
struct BDecomposeHelper<Head, Tail...> {
    static void decompose(std::vector<const Def*>& ops) {
        ops.push_back(Head::emit());
        BDecomposeHelper<Tail...>::decompose(ops);
    }
};

template<class... Ops>
struct BDecompose {
    static Array<const Def*> emit() {
        std::vector<const Def*> result;
        BDecomposeHelper<Ops...>::decompose(result);
        return result;
    }
};

template<class... Ops>
struct BTuple {
    static const Def* emit() {
        return Builder::world().tuple(BDecompose<Ops...>::emit());
    }
};

template<class Name, class... T>
struct BVar {
    typedef BTuple<T...> type;
    static int offset;
    static const Def* emit() {
        return Builder::world().var(type::emit(), Builder::depth - offset);
    }
};

template<class Name, class... T>
int BVar<Name, T...>::offset = 0;

template<class BVar, class Body>
struct BLambda {
    static const Def* emit() {
        auto domain = BVar::type::emit();
        BVar::offset = Builder::depth;
        ++Builder::depth;
        auto def = Builder::world().lambda(domain, Body::emit());
        --Builder::depth;
        return def;
    }
};

template<class Callee, class... Args>
struct BApp {
    static const Def* emit() {
        return Builder::world().app(Callee::emit(), BDecompose<Args...>::emit());
    }
};

}

#endif
