#ifndef THORIN_BUILDER_H
#define THORIN_BUILDER_H

#include "thorin/world.h"

namespace thorin {

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
struct BTupleDecompose {
    static void decompose(std::vector<const Def*>&) {}
};

template<class Head, class... Tail>
struct BTupleDecompose<Head, Tail...> {
    static void decompose(std::vector<const Def*>& ops) {
        ops.push_back(Head::emit());
        BTupleDecompose<Tail...>::decompose(ops);
    }
};

template<class... Ops>
struct BTuple {
    static const Def* emit() {
        std::vector<const Def*> ops;
        BTupleDecompose<Ops...>::decompose(ops);
        return Builder::world().tuple(ops);
    }
};

template<char name, class... T>
struct BVar {
    typedef BTuple<T...> type;
    static int offset;
    static const Def* emit() {
        return Builder::world().var(type::emit(), Builder::depth - offset);
    }
};

template<char name, class... T>
int BVar<name, T...>::offset = 0;

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
        return Builder::world().app(Callee::emit(), BTuple<Args...>::emit());
    }
};

}

#endif
