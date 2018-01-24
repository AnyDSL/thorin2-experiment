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

template<int w> struct Fold_i_eq { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() == b.get<T>()); } };
template<int w> struct Fold_i_ne { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() != b.get<T>()); } };
template<int w> struct Fold_iugt { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() <  b.get<T>()); } };
template<int w> struct Fold_iuge { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() <= b.get<T>()); } };
template<int w> struct Fold_iult { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >  b.get<T>()); } };
template<int w> struct Fold_iule { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >= b.get<T>()); } };
template<int w> struct Fold_isgt { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() <  b.get<T>()); } };
template<int w> struct Fold_isge { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() <= b.get<T>()); } };
template<int w> struct Fold_islt { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >  b.get<T>()); } };
template<int w> struct Fold_isle { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >= b.get<T>()); } };

template<int w> struct Fold_rt   { static Box run(Box  , Box  ) { return {true}; } };
template<int w> struct Fold_rult { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <  b.get<T>()}; } };
template<int w> struct Fold_rugt { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >  b.get<T>()}; } };
template<int w> struct Fold_rune { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() != b.get<T>()}; } };
template<int w> struct Fold_rueq { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() == b.get<T>()}; } };
template<int w> struct Fold_rule { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <= b.get<T>()}; } };
template<int w> struct Fold_ruge { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >= b.get<T>()}; } };
template<int w> struct Fold_runo { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>())}; } };
template<int w> struct Fold_rord { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>())}; } };
template<int w> struct Fold_rolt { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <  b.get<T>()}; } };
template<int w> struct Fold_rogt { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >  b.get<T>()}; } };
template<int w> struct Fold_rone { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() != b.get<T>()}; } };
template<int w> struct Fold_roeq { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() == b.get<T>()}; } };
template<int w> struct Fold_role { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <= b.get<T>()}; } };
template<int w> struct Fold_roge { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >= b.get<T>()}; } };
template<int w> struct Fold_rf   { static Box run(Box  , Box  ) { return {false}; } };

}

#endif
