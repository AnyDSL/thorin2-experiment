#ifndef THORIN_UTIL_FOLD_H
#define THORIN_UTIL_FOLD_H

#include "thorin/util/box.h"

namespace thorin {

// This code assumes two-complement arithmetic for unsigned operations.
// This is *implementation-defined* but *NOT* *undefined behavior*.

struct BottomException {};

template<class ST, bool no_wrap>
class SInt {
public:
    typedef typename std::make_unsigned<ST>::type UT;
    static_assert(std::is_signed<ST>::value, "ST must be a signed type");

    SInt(ST data)
        : data_(data)
    {}

    SInt operator-() const {
        if (data_ == std::numeric_limits<ST>::min()) {
            if (no_wrap)
                throw BottomException();
            else
                return SInt(0);
        }
        return SInt(-data_);
    }

    SInt operator+(SInt other) const {
        SInt res(UT(this->data_) + UT(other.data_));
        if (no_wrap && (this->is_neg() == other.is_neg()) && (this->is_neg() != res.is_neg()))
            throw BottomException();
        return res;
    }

    SInt operator-(SInt other) const { return *this + SInt(other.minus()); }

    SInt operator*(SInt other) const {
        ST a = this->data_, b = other.data_;

        if (no_wrap) {
            if (a > ST(0)) {
                if (b > ST(0)) {
                    if (a > (std::numeric_limits<ST>::max() / b))
                        throw BottomException();
                } else {
                    if (b < (std::numeric_limits<ST>::min() / a))
                        throw BottomException();
                }
            } else {
                if (b > ST(0)) {
                    if (a < (std::numeric_limits<ST>::min() / b))
                        throw BottomException();
                } else {
                    if ( (a != ST(0)) && (b < (std::numeric_limits<ST>::max() / a)))
                        throw BottomException();
                }
            }
        }

        return UT(a) * UT(b);
    }

    void div_check(SInt other) const {
        if (other.data_ == ST(0) || (this->data_ == std::numeric_limits<ST>::min() && other.data_ == ST(-1)))
            throw BottomException();
    }

    SInt operator/(SInt other) const { div_check(other); return SInt(this->data_ / other.data_); }
    SInt operator%(SInt other) const { div_check(other); return SInt(this->data_ % other.data_); }

    SInt operator<<(SInt other) const {
        if (other.data_ >= std::numeric_limits<ST>::digits+1 || other.is_neg())
            throw BottomException();

        // TODO: actually this is correct, but see: https://github.com/AnyDSL/impala/issues/34
        //if (no_wrap && (this->is_neg() || this->data_ > std::numeric_limits<ST>::max() >> other.data_))
        //    throw BottomException();

        return ST(UT(this->data_) << UT(other.data_));
    }

    SInt operator& (SInt other) const { return this->data_ & other.data_; }
    SInt operator| (SInt other) const { return this->data_ | other.data_; }
    SInt operator^ (SInt other) const { return this->data_ ^ other.data_; }
    SInt operator>>(SInt other) const { return this->data_ >>other.data_; }
    bool operator< (SInt other) const { return this->data_ < other.data_; }
    bool operator<=(SInt other) const { return this->data_ <=other.data_; }
    bool operator> (SInt other) const { return this->data_ > other.data_; }
    bool operator>=(SInt other) const { return this->data_ >=other.data_; }
    bool operator==(SInt other) const { return this->data_ ==other.data_; }
    bool operator!=(SInt other) const { return this->data_ !=other.data_; }
    bool is_neg() const { return data_ < ST(0); }
    operator ST() const { return data_; }
    ST data() const { return data_; }

private:
    SInt minus() const { return SInt(~UT(data_)+UT(1u)); }
    SInt abs() const { return is_neg() ? minus() : *this; }

    ST data_;
};

template<class UT, bool no_wrap>
class UInt {
public:
    static_assert(std::is_unsigned<UT>::value, "UT must be an unsigned type");

    UInt(UT data)
        : data_(data)
    {}

    UInt operator-() const { return UInt(0u) - *this; }

    UInt operator+(UInt other) const {
        UInt res(UT(this->data_) + UT(other.data_));
        if (no_wrap && res.data_ < this->data_)
            throw BottomException();
        return res;
    }

    UInt operator-(UInt other) const {
        UInt res(UT(this->data_) - UT(other.data_));
        if (no_wrap && res.data_ > this->data_)
            throw BottomException();
        return res;
    }

    UInt operator*(UInt other) const {
        if (no_wrap && other.data_ && this->data_ > std::numeric_limits<UT>::max() / other.data_)
            throw BottomException();
        return UT(this->data_) * UT(other.data_);
    }

    void div_check(UInt other) const { if (other.data_ == UT(0u)) throw BottomException(); }
    UInt operator/(UInt other) const { div_check(other); return UInt(this->data_ / other.data_); }
    UInt operator%(UInt other) const { div_check(other); return UInt(this->data_ % other.data_); }

    UInt operator<<(UInt other) const {
        if (no_wrap && other.data_ >= std::numeric_limits<UT>::digits)
            throw BottomException();
        return this->data_ << other.data_;
    }

    UInt operator& (UInt other) const { return this->data_ & other.data_; }
    UInt operator| (UInt other) const { return this->data_ | other.data_; }
    UInt operator^ (UInt other) const { return this->data_ ^ other.data_; }
    UInt operator>>(UInt other) const { return this->data_ >>other.data_; }
    bool operator< (UInt other) const { return this->data_ < other.data_; }
    bool operator<=(UInt other) const { return this->data_ <=other.data_; }
    bool operator> (UInt other) const { return this->data_ > other.data_; }
    bool operator>=(UInt other) const { return this->data_ >=other.data_; }
    bool operator==(UInt other) const { return this->data_ ==other.data_; }
    bool operator!=(UInt other) const { return this->data_ !=other.data_; }
    bool is_neg() const { return data_ < UT(0u); }
    operator UT() const { return data_; }
    UT data() const { return data_; }

private:
    UInt minus() const { return UInt(~UT(data_)+UT(1u)); }
    UInt abs() const { return is_neg() ? minus() : *this; }

    UT data_;
};

inline half        rem(half a, half b)               { return      fmod(a, b); }
inline float       rem(float a, float b)             { return std::fmod(a, b); }
inline double      rem(double a, double b)           { return std::fmod(a, b); }
inline long double rem(long double a, long double b) { return std::fmod(a, b); }

#if 0
    SInt operator+(SInt other) const {
        SInt res(UT(this->data_) + UT(other.data_));
        if (no_wrap && (this->is_neg() == other.is_neg()) && (this->is_neg() != res.is_neg()))
            throw BottomException();
        return res;
    }

    UInt operator+(UInt other) const {
        UInt res(UT(this->data_) + UT(other.data_));
        if (no_wrap && res.data_ < this->data_)
            throw BottomException();
        return res;
    }
#endif

template<int> struct w2u {};
template<> struct w2u< 8> { typedef  u8 type; };
template<> struct w2u<16> { typedef u16 type; };
template<> struct w2u<32> { typedef u32 type; };
template<> struct w2u<64> { typedef u64 type; };

template<int> struct w2s {};
template<> struct w2s< 8> { typedef  s8 type; };
template<> struct w2s<16> { typedef s16 type; };
template<> struct w2s<32> { typedef s32 type; };
template<> struct w2s<64> { typedef s64 type; };

template<int> struct w2r {};
template<> struct w2r<16> { typedef r16 type; };
template<> struct w2r<32> { typedef r32 type; };
template<> struct w2r<64> { typedef r64 type; };

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

template<int w> struct FoldRAdd { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T{a.template get<T>() + b.template get<T>()}; } };
template<int w> struct FoldRSub { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T{a.template get<T>() - b.template get<T>()}; } };
template<int w> struct FoldRMul { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T{a.template get<T>() * b.template get<T>()}; } };
template<int w> struct FoldRDiv { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T{a.template get<T>() / b.template get<T>()}; } };
template<int w> struct FoldRRem { static Box run(Box a, Box b) { typedef typename w2r<w>::type T; return T{rem(a.template get<T>(), b.template get<T>())}; } };

}

#endif
