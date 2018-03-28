#ifndef THORIN_UTILITY_H
#define THORIN_UTILITY_H

#include <cassert>
#include <memory>
#include <queue>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "thorin/util/types.h"

#ifndef _MSC_VER
#define THORIN_UNREACHABLE do { assert(true && "unreachable"); abort(); } while(0)
#else // _MSC_VER
inline __declspec(noreturn) void thorin_dummy_function() { abort(); }
#define THORIN_UNREACHABLE do { assert(true && "unreachable"); thorin_dummy_function(); } while(0)
#endif

#ifndef NDEBUG
#define assert_unused(x) assert(x)
#else
#define assert_unused(x) ((void) (false && (x)))
#endif

#if (defined(__clang__) || defined(__GNUC__)) && (defined(__x86_64__) || defined(__i386__))
#define THORIN_BREAK { asm("int3"); }
#elif defined(_MSC_VER)
#define THORIN_BREAK { DebugBreak(); }
#else
#define THORIN_BREAK { static_cast<volatile int*>(nullptr) = 42; }
#endif

namespace thorin {

/// Use to initialize an @c std::unique_ptr<T> in a lazy way.
template<class This, class T>
inline T& lazy_init(const This* self, std::unique_ptr<T>& ptr) {
    return *(ptr ? ptr : ptr = std::make_unique<T>(*self));
}

template<class T>
T pop(std::queue<T>& queue) {
    auto val = queue.front();
    queue.pop();
    return val;
}

template<class T>
struct Push {
    Push(T& t, T new_val)
        : old_(t)
        , ref_(t)
    {
        t = new_val;
    }
    ~Push() { ref_ = old_; }

private:
    T old_;
    T& ref_;
};

template<class T, class U>
inline Push<T> push(T& t, U new_val) { return Push<T>(t, new_val); }

#define THORIN_LNAME__(name, line) name##__##line
#define THORIN_LNAME_(name, line)  THORIN_LNAME__(name, line)
#define THORIN_LNAME(name)         THORIN_LNAME_(name, __LINE__)

#define THORIN_PUSH(what, with) auto THORIN_LNAME(thorin_push) = thorin::push((what), (with))

/**
 * A tagged pointer: first 16 bits is tag (index), remaining 48 bits is the actual pointer.
 * For non-x86_64 there is a fallback implementation.
 */
template<class T, class I = size_t>
class TaggedPtr {
public:
    TaggedPtr() {}
    TaggedPtr(T* ptr, I index)
#if defined(__x86_64__) || (_M_X64)
        : ptr_(reinterpret_cast<int64_t>(ptr))
#else
        : ptr_(ptr)
#endif
        , index_(index)
    {}
    TaggedPtr(const TaggedPtr& other)
        : ptr_(other.ptr_)
        , index_(other.index_)
    {}
    TaggedPtr(TaggedPtr&& other)
        : TaggedPtr()
    {
        swap(*this, other);
    }

    //@{ getters
    T* ptr() const { return reinterpret_cast<T*>(ptr_); }
    T* operator->() const { return ptr(); }
    operator T*() const { return ptr(); }
    I index() const { return index_; }
    //@}

    //@{ setters
    void set_index(I index) { index_ = index; }
    void set_ptr(T* ptr) { ptr_ = reinterpret_cast<int64_t>(ptr); }
    //@}

    bool operator==(TaggedPtr other) const { return this->ptr() == other.ptr() && this->index() == other.index(); }

    //@{ swap and assign
    TaggedPtr& operator=(TaggedPtr other) { swap(*this, other); return *this; }
    friend void swap(TaggedPtr& a, TaggedPtr& b) {
        using std::swap;
#if defined(__x86_64__) || (_M_X64)
        swap(a.u64_, b.u64_);
#else
        swap(a.ptr_,   b.ptr_);
        swap(a.index_, b.index_);
#endif
    }
    //@}

private:
#if defined(__x86_64__) || (_M_X64)
    union {
        struct {
            int64_t ptr_   : 48; // sign extend to make pointer canonical
            int64_t index_ : 16;
        };
        uint64_t u64_;
    };
#else
    T* ptr_;
    I index_;
#endif
};

#if defined(__x86_64__) || (_M_X64)
static_assert(sizeof(TaggedPtr<void*,int>) == 8, "a tagged ptr on x86_64 is supposed to be 8 bytes big");
#endif

//@{ bit fiddling

/// Determines whether @p i is a power of two.
constexpr uint64_t is_power_of_2(uint64_t i) { return ((i != 0) && !(i & (i - 1))); }

constexpr uint64_t log2(uint64_t n, uint64_t p = 0) { return (n <= 1_u64) ? p : log2(n / 2_u64, p + 1_u64); }

inline uint64_t round_to_power_of_2(uint64_t i) {
    i--;
    i |= i >>  1_u64;
    i |= i >>  2_u64;
    i |= i >>  4_u64;
    i |= i >>  8_u64;
    i |= i >> 16_u64;
    i |= i >> 32_u64;
    i++;
    return i;
}

inline size_t bitcount(uint64_t v) {
#if defined(__GNUC__) | defined(__clang__)
    return __builtin_popcountll(v);
#elif defined(_MSC_VER)
    return __popcnt64(v);
#else
    // see https://stackoverflow.com/questions/3815165/how-to-implement-bitcount-using-only-bitwise-operators
    auto c = v - ((v >>  1_u64)      & 0x5555555555555555_u64);
    c =          ((c >>  2_u64)      & 0x3333333333333333_u64) + (c & 0x3333333333333333_u64);
    c =          ((c >>  4_u64) + c) & 0x0F0F0F0F0F0F0F0F_u64;
    c =          ((c >>  8_u64) + c) & 0x00FF00FF00FF00FF_u64;
    c =          ((c >> 16_u64) + c) & 0x0000FFFF0000FFFF_u64;
    return       ((c >> 32_u64) + c) & 0x00000000FFFFFFFF_u64;
#endif
}

//@}

}

#endif
