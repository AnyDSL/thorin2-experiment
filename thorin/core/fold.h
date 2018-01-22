#ifndef THORIN_UTIL_FOLD_H
#define THORIN_UTIL_FOLD_H

#include "thorin/core/tables.h"
#include "thorin/util/types.h"

namespace thorin::core {

// This code assumes two-complement arithmetic for unsigned operations.
// This is *implementation-defined* but *NOT* *undefined behavior*.

struct BottomException {};

template<int w, bool nsw, bool nuw>
struct FoldWAdd {
    static Box run(Box a, Box b) {
        auto x = a.template get<typename w2u<w>::type>();
        auto y = b.template get<typename w2u<w>::type>();
        decltype(x) res = x + y;
        if (nuw && res < x) throw BottomException();
        // TODO nsw
        return {res};
    }
};

template<int w, bool nsw, bool nuw>
struct FoldWSub {
    static Box run(Box a, Box b) {
        typedef typename w2u<w>::type UT;
        auto x = a.template get<UT>();
        auto y = b.template get<UT>();
        decltype(x) res = x - y;
        //if (nuw && y && x > std::numeric_limits<UT>::max() / y) throw BottomException();
        // TODO nsw
        return {res};
    }
};

template<int w, bool nsw, bool nuw>
struct FoldWMul {
    static Box run(Box a, Box b) {
        typedef typename w2u<w>::type UT;
        auto x = a.template get<UT>();
        auto y = b.template get<UT>();
        decltype(x) res = x * y;
        if (nuw && y && x > std::numeric_limits<UT>::max() / y) throw BottomException();
        // TODO nsw
        return {res};
    }
};

template<int w, bool nsw, bool nuw>
struct FoldWShl {
    static Box run(Box a, Box b) {
        typedef typename w2u<w>::type UT;
        auto x = a.template get<UT>();
        auto y = b.template get<UT>();
        decltype(x) res = x << y;
        //if (nuw && y && x > std::numeric_limits<UT>::max() / y) throw BottomException();
        // TODO nsw
        return {res};
    }
};

template<int w> struct FoldAShr { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >> b.get<T>()); } };
template<int w> struct FoldLShr { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >> b.get<T>()); } };
template<int w> struct FoldIAnd { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() & b.get<T>()); } };
template<int w> struct FoldIOr  { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() | b.get<T>()); } };
template<int w> struct FoldIXor { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() ^ b.get<T>()); } };

template<int w> struct FoldRAdd { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() + b.get<T>()); } };
template<int w> struct FoldRSub { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() - b.get<T>()); } };
template<int w> struct FoldRMul { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() * b.get<T>()); } };
template<int w> struct FoldRDiv { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() / b.get<T>()); } };
template<int w> struct FoldRRem { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(rem(a.get<T>(), b.get<T>())); } };

template<int w>
struct FoldICmp {
    static Box run(IRel rel, Box a, Box b) {
        typedef typename w2s<w>::type ST;
        typedef typename w2u<w>::type UT;
        switch (rel) {
            case IRel:: eq: return {a.get<UT>() == b.get<UT>()};
            case IRel:: ne: return {a.get<UT>() != b.get<UT>()};
            case IRel::ugt: return {a.get<UT>() <  b.get<UT>()};
            case IRel::uge: return {a.get<UT>() <= b.get<UT>()};
            case IRel::ult: return {a.get<UT>() >  b.get<UT>()};
            case IRel::ule: return {a.get<UT>() >= b.get<UT>()};
            case IRel::sgt: return {a.get<ST>() <  b.get<ST>()};
            case IRel::sge: return {a.get<ST>() <= b.get<ST>()};
            case IRel::slt: return {a.get<ST>() >  b.get<ST>()};
            case IRel::sle: return {a.get<ST>() >= b.get<ST>()};
            default: THORIN_UNREACHABLE;
        }
    }
};

template<int w>
struct FoldRCmp {
    static Box run(RRel rel, Box a, Box b) {
        typedef typename w2r<w>::type T;
        switch (rel) {
            case RRel::  t: return {true};
            case RRel::ult: return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <  b.get<T>()};
            case RRel::ugt: return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >  b.get<T>()};
            case RRel::une: return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() != b.get<T>()};
            case RRel::ueq: return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() == b.get<T>()};
            case RRel::ule: return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <= b.get<T>()};
            case RRel::uge: return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >= b.get<T>()};
            case RRel::uno: return { std::isunordered(a.get<T>(), b.get<T>())};
            case RRel::ord: return {!std::isunordered(a.get<T>(), b.get<T>())};
            case RRel::olt: return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <  b.get<T>()};
            case RRel::ogt: return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >  b.get<T>()};
            case RRel::one: return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() != b.get<T>()};
            case RRel::oeq: return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() == b.get<T>()};
            case RRel::ole: return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <= b.get<T>()};
            case RRel::oge: return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >= b.get<T>()};
            case RRel::  f: return {false};
            default: THORIN_UNREACHABLE;
        }
    }
};

}

#endif
