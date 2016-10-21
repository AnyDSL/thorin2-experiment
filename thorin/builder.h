#ifndef THORIN_META_H
#define THORIN_META_H

#include "thorin/world.h"

namespace thorin {

struct Builder {
    static World& world() { return *world_; }
    static World* world_;
    static int depth;
};

World* Builder::world_ = nullptr;
int Builder::depth = 0;

struct MStar {
    MStar() {}
    static const Def* emit() {
        return Builder::world().star();
    }
};

template<class... Ops>
struct MTupleDecompose {
    static void decompose(std::vector<const Def*>&) {}
};

template<class Head, class... Tail>
struct MTupleDecompose<Head, Tail...> {
    static void decompose(std::vector<const Def*>& ops) {
        ops.push_back(Head::emit());
        MTupleDecompose<Tail...>::decompose(ops);
    }
};

template<class... Ops>
struct MTuple {
    static const Def* emit() {
        std::vector<const Def*> ops;
        MTupleDecompose<Ops...>::decompose(ops);
        return Builder::world().tuple(ops);
    }
};

template<char name, class... T>
struct MVar {
    typedef MTuple<T...> type;
    static int offset;
    static const Def* emit() {
        return Builder::world().var(type::emit(), Builder::depth - offset);
    }
};

template<char name, class... T>
int MVar<name, T...>::offset = 0;

template<class MVar, class Body>
struct MLambda {
    static const Def* emit() {
        auto domain = MVar::type::emit();
        MVar::offset = Builder::depth;
        ++Builder::depth;
        auto def = Builder::world().lambda(domain, Body::emit());
        --Builder::depth;
        return def;
    }
};

template<class Callee, class... Args>
struct MApp {
    static const Def* emit() {
        return Builder::world().app(Callee::emit(), MTuple<Args...>::emit());
    }
};

}

#endif
