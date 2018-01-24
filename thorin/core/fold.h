#ifndef THORIN_UTIL_FOLD_H
#define THORIN_UTIL_FOLD_H

#include "thorin/core/tables.h"
#include "thorin/util/types.h"

namespace thorin::core {

// This code assumes two-complement arithmetic for unsigned operations.
// This is *implementation-defined* but *NOT* *undefined behavior*.

struct BottomException {};

template<int w, bool nsw, bool nuw>
struct Fold_wadd {
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
struct Fold_wsub {
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
struct Fold_wmul {
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
struct Fold_wshl {
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

template<int w> struct Fold_ashr { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >> b.get<T>()); } };
template<int w> struct Fold_lshr { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >> b.get<T>()); } };
template<int w> struct Fold_iand { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() &  b.get<T>()); } };
template<int w> struct Fold_ior  { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() |  b.get<T>()); } };
template<int w> struct Fold_ixor { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() ^  b.get<T>()); } };

template<int w> struct Fold_radd { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() +  b.get<T>()); } };
template<int w> struct Fold_rsub { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() -  b.get<T>()); } };
template<int w> struct Fold_rmul { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() *  b.get<T>()); } };
template<int w> struct Fold_rdiv { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() /  b.get<T>()); } };
template<int w> struct Fold_rrem { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(rem(a.get<T>(), b.get<T>())); } };

template<ICmp> struct FoldICmp {};
template<> struct FoldICmp<i_eq> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() == b.get<T>()); } }; };
template<> struct FoldICmp<i_ne> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() != b.get<T>()); } }; };
template<> struct FoldICmp<iugt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() <  b.get<T>()); } }; };
template<> struct FoldICmp<iuge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() <= b.get<T>()); } }; };
template<> struct FoldICmp<iult> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >  b.get<T>()); } }; };
template<> struct FoldICmp<iule> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >= b.get<T>()); } }; };
template<> struct FoldICmp<isgt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() <  b.get<T>()); } }; };
template<> struct FoldICmp<isge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() <= b.get<T>()); } }; };
template<> struct FoldICmp<islt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >  b.get<T>()); } }; };
template<> struct FoldICmp<isle> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >= b.get<T>()); } }; };


template<RCmp> struct FoldRCmp {};
template<> struct FoldRCmp<rt  > { template<int w> struct Fold { static Box run(Box  , Box  ) { return {true}; } }; };
template<> struct FoldRCmp<rult> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <  b.get<T>()}; } }; };
template<> struct FoldRCmp<rugt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >  b.get<T>()}; } }; };
template<> struct FoldRCmp<rune> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() != b.get<T>()}; } }; };
template<> struct FoldRCmp<rueq> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() == b.get<T>()}; } }; };
template<> struct FoldRCmp<rule> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <= b.get<T>()}; } }; };
template<> struct FoldRCmp<ruge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >= b.get<T>()}; } }; };
template<> struct FoldRCmp<runo> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>())}; } }; };
template<> struct FoldRCmp<rord> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>())}; } }; };
template<> struct FoldRCmp<rolt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <  b.get<T>()}; } }; };
template<> struct FoldRCmp<rogt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >  b.get<T>()}; } }; };
template<> struct FoldRCmp<rone> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() != b.get<T>()}; } }; };
template<> struct FoldRCmp<roeq> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() == b.get<T>()}; } }; };
template<> struct FoldRCmp<role> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <= b.get<T>()}; } }; };
template<> struct FoldRCmp<roge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >= b.get<T>()}; } }; };
template<> struct FoldRCmp<rf  > { template<int w> struct Fold { static Box run(Box  , Box  ) { return {false}; } }; };

}

#endif
