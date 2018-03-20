#ifndef THORIN_ME_FOLD_H
#define THORIN_ME_FOLD_H

#include "thorin/me/tables.h"
#include "thorin/util/types.h"

namespace thorin::me {

// This code assumes two-complement arithmetic for unsigned operations.
// This is *implementation-defined* but *NOT* *undefined behavior*.

struct BottomException {};

template<WOp> struct FoldWOp {};

template<> struct FoldWOp<WOp::add> {
    template<int w, bool nsw, bool nuw> struct Fold {
        static Box run(Box a, Box b) {
            auto x = a.template get<typename w2u<w>::type>();
            auto y = b.template get<typename w2u<w>::type>();
            decltype(x) res = x + y;
            if (nuw && res < x) throw BottomException();
            // TODO nsw
            return {res};
        }
    };
};

template<> struct FoldWOp<WOp::sub> {
    template<int w, bool nsw, bool nuw> struct Fold {
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
};

template<> struct FoldWOp<WOp::mul> {
    template<int w, bool nsw, bool nuw> struct Fold {
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
};

template<> struct FoldWOp<WOp::shl> {
    template<int w, bool nsw, bool nuw> struct Fold {
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
};

// TODO handle division by zero
template<MOp> struct FoldMOp {};
template<> struct FoldMOp<MOp::sdiv> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; T r = b.get<T>(); if (r == 0) throw BottomException(); return T(a.get<T>() / r); } }; };
template<> struct FoldMOp<MOp::udiv> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; T r = b.get<T>(); if (r == 0) throw BottomException(); return T(a.get<T>() / r); } }; };
template<> struct FoldMOp<MOp::smod> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2s<w>::type T; T r = b.get<T>(); if (r == 0) throw BottomException(); return T(a.get<T>() % r); } }; };
template<> struct FoldMOp<MOp::umod> { template<int w> struct Fold { static Box run(Box a, Box b) { typedef typename w2u<w>::type T; T r = b.get<T>(); if (r == 0) throw BottomException(); return T(a.get<T>() % r); } }; };

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

template<ICmp cmp> struct FoldICmp {
    template<int w> struct Fold {
        inline static Box run(Box a, Box b) {
            typedef typename w2u<w>::type T;
            auto x = a.get<T>();
            auto y = b.get<T>();
            bool result = false;
            auto pm = !(x >> T(w-1)) &&  (y >> T(w-1));
            auto mp =  (x >> T(w-1)) && !(y >> T(w-1));
            result |= (cmp & ICmp::pm) != ICmp::f && pm;
            result |= (cmp & ICmp::mp) != ICmp::f && mp;
            result |= (cmp & ICmp::g ) != ICmp::f && x > y && !mp;
            result |= (cmp & ICmp::l ) != ICmp::f && x < y && !pm;
            result |= (cmp & ICmp::e ) != ICmp::f && x == y;
            return result;
        }
    };
};

template<RCmp cmp> struct FoldRCmp {
    template<int w> struct Fold {
        inline static Box run(Box a, Box b) {
            typedef typename w2r<w>::type T;
            auto x = a.get<T>();
            auto y = b.get<T>();
            bool result = false;
            result |= (cmp & RCmp::u) != RCmp::f && std::isunordered(x, y);
            result |= (cmp & RCmp::g) != RCmp::f && x > y;
            result |= (cmp & RCmp::l) != RCmp::f && x < y;
            result |= (cmp & RCmp::e) != RCmp::f && x == y;
            return result;
        }
    };
};

}

#endif
