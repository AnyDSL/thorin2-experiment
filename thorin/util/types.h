#ifndef THORIN_UTIL_BOX_H
#define THORIN_UTIL_BOX_H

#define HALF_ENABLE_CPP11_CONSTEXPR 1
#define HALF_ENABLE_CPP11_USER_LITERALS 1

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include <half.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace thorin {

using namespace half_float::literal;
using half_float::half;

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef  int8_t s8; typedef  int16_t s16; typedef  int32_t s32; typedef  int64_t s64;
/*       */ typedef half_float::half r16; typedef    float r32; typedef   double r64;

#define THORIN_S_TYPES(m) m(s8)  m(s16) m(s32) m(s64)
#define THORIN_U_TYPES(m) m(u8)  m(u16) m(u32) m(u64)
#define THORIN_R_TYPES(m)        m(r16) m(r32) m(r64)

#define THORIN_TYPES(m) THORIN_S_TYPES(m) THORIN_U_TYPES(m) THORIN_R_TYPES(m)

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

/// A @c size_t literal. Use @c 0_s to disambiguate @c 0 from @c nullptr.
constexpr size_t operator""_s(unsigned long long int i) { return size_t(i); }

constexpr  s8 operator"" _s8(unsigned long long int i) { return  s8(i); }
constexpr s16 operator""_s16(unsigned long long int i) { return s16(i); }
constexpr s32 operator""_s32(unsigned long long int i) { return s32(i); }
constexpr s64 operator""_s64(unsigned long long int i) { return s64(i); }
constexpr  u8 operator"" _u8(unsigned long long int i) { return  u8(i); }
constexpr u16 operator""_u16(unsigned long long int i) { return u16(i); }
constexpr u32 operator""_u32(unsigned long long int i) { return u32(i); }
constexpr u64 operator""_u64(unsigned long long int i) { return u64(i); }
inline /*constexpr*/ r16 operator""_r16(long double d) { return r16(d); } // wait till fixed upstream
constexpr r32 operator""_r32(long double d) { return r32(d); }
constexpr r64 operator""_r64(long double d) { return r64(d); }

union Box {
public:
    Box() { u64_ = 0; }

#define CODE(T) \
    Box(T val) { u64_ = 0; T ## _ = val; }
    THORIN_TYPES(CODE)
#undef CODE

#define CODE(T) \
    T get_ ## T() const { return T ## _; }
    THORIN_TYPES(CODE)
#undef CODE

    bool operator==(Box other) const { return this->u64_ == other.get_u64(); }
    template<typename T> T& get() { return *((T*)this); }

private:
#define CODE(T) \
    T T ## _;
    THORIN_TYPES(CODE)
#undef CODE
};

}

#endif
