#ifndef THORIN_BUILDER_H
#define THORIN_BUILDER_H

#include <utility>

#include "thorin/world.h"

namespace thorin {

template <char... chars>
using BName = std::integer_sequence<char, chars...>;

template<class T, T... chars>
constexpr BName<chars...> operator""_name() { return {}; }

template <typename>
struct StrGetter;

template<char... chars>
struct StrGetter<BName<chars...>> {
    const char* GetString() const {
        static constexpr char str[sizeof...(chars) + 1] = { chars..., '\0' };
        return str;
    }
};

#define NAME(n) decltype(#n ## _name)

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
        return Builder::world().var(BDecompose<T...>::emit(), Builder::depth - offset);
    }
};

template<class Name, class... T>
int BVar<Name, T...>::offset = 0;

template<class... T>
struct B_ {
    static int offset;
    typedef BTuple<T...> type;
};

template<class... T>
int B_<T...>::offset = 0;

template<class Name, class... T>
struct BAssume {
    typedef BTuple<T...> type;
    static const Assume* assume;
    static const Def* emit() {
        return assume ? assume : assume = Builder::world().assume(type::emit(), StrGetter<Name>::str());
    }
};

template<class Name, class... T>
const Assume* BAssume<Name, T...>::assume = nullptr;

template<class Name>
struct BDef {
    static const Def* def;
    static const Def* emit() {
        return def;
    }
};

template<class Name>
const Def* BDef<Name>::def = nullptr;

#define BDEF(n, d) typedef BDef<NAME(n)> n; n::def = d

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

template<class BVar, class Body>
struct BPi {
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
