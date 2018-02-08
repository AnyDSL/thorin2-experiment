#ifndef THORIN_UTIL_FOLD_H
#define THORIN_UTIL_FOLD_H

#include "thorin/core/tables.h"
#include "thorin/util/types.h"

namespace thorin::core {

// This code assumes two-complement arithmetic for unsigned operations.
// This is *implementation-defined* but *NOT* *undefined behavior*.

struct BottomException {};

template<int w, bool nsw, bool nuw>
struct Fold_add {
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
struct Fold_sub {
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
struct Fold_mul {
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
struct Fold_shl {
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

template<int w> struct Fold_sdiv { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() /  b.get<T>()); } };

template<IOp> struct FoldIOp {};
template<> struct FoldIOp<IOp::ashr> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return T(a.get<T>() >> b.get<T>()); } }; };
template<> struct FoldIOp<IOp::lshr> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() >> b.get<T>()); } }; };
template<> struct FoldIOp<IOp::iand> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() &  b.get<T>()); } }; };
template<> struct FoldIOp<IOp::ior > { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() |  b.get<T>()); } }; };
template<> struct FoldIOp<IOp::ixor> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return T(a.get<T>() ^  b.get<T>()); } }; };

template<ROp> struct FoldROp {};
template<> struct FoldROp<ROp::radd> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() +  b.get<T>()); } }; };
template<> struct FoldROp<ROp::rsub> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() -  b.get<T>()); } }; };
template<> struct FoldROp<ROp::rmul> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() *  b.get<T>()); } }; };
template<> struct FoldROp<ROp::rdiv> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(a.get<T>() /  b.get<T>()); } }; };
template<> struct FoldROp<ROp::rmod> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T(rem(a.get<T>(), b.get<T>())); } }; };

template<ICmp> struct FoldICmp {};
template<> struct FoldICmp<ICmp::t  > { template<int w> struct Fold { static Box run(Box  , Box  ) { return {true}; } }; };
template<> struct FoldICmp<ICmp::eq > { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return {a.get<T>() == b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::ne > { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return {a.get<T>() != b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::sgt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return {a.get<T>() >  b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::sge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return {a.get<T>() >= b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::slt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return {a.get<T>() <  b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::sle> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; return {a.get<T>() <= b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::ugt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return {a.get<T>() >  b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::uge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return {a.get<T>() >= b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::ult> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return {a.get<T>() <  b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::ule> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; return {a.get<T>() <= b.get<T>()}; } }; };
template<> struct FoldICmp<ICmp::sugt>{ template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type S; typedef typename w2u<w>::type U; return {a.get<S>() >  b.get<S>() && a.get<U>() >  b.get<U>()}; } }; };
template<> struct FoldICmp<ICmp::suge>{ template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type S; typedef typename w2u<w>::type U; return {a.get<S>() >= b.get<S>() && a.get<U>() >= b.get<U>()}; } }; };
template<> struct FoldICmp<ICmp::sult>{ template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type S; typedef typename w2u<w>::type U; return {a.get<S>() <  b.get<S>() && a.get<U>() <  b.get<U>()}; } }; };
template<> struct FoldICmp<ICmp::sule>{ template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type S; typedef typename w2u<w>::type U; return {a.get<S>() <= b.get<S>() && a.get<U>() <= b.get<U>()}; } }; };
template<> struct FoldICmp<ICmp::f  > { template<int w> struct Fold { static Box run(Box  , Box  ) { return {false}; } }; };

template<RCmp> struct FoldRCmp {};
template<> struct FoldRCmp<RCmp::t  > { template<int w> struct Fold { static Box run(Box  , Box  ) { return {true}; } }; };
template<> struct FoldRCmp<RCmp::ult> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <  b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::ugt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >  b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::une> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() != b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::ueq> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() == b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::ule> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() <= b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::uge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>()) || a.get<T>() >= b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::uno> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return { std::isunordered(a.get<T>(), b.get<T>())}; } }; };
template<> struct FoldRCmp<RCmp::ord> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>())}; } }; };
template<> struct FoldRCmp<RCmp::olt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <  b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::ogt> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >  b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::one> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() != b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::oeq> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() == b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::ole> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() <= b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::oge> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return {!std::isunordered(a.get<T>(), b.get<T>()) && a.get<T>() >= b.get<T>()}; } }; };
template<> struct FoldRCmp<RCmp::f  > { template<int w> struct Fold { static Box run(Box  , Box  ) { return {false}; } }; };

}

#endif
