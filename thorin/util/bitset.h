#ifndef THORIN_UTIL_BITSET_H
#define THORIN_UTIL_BITSET_H

#include <limits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "thorin/util/utility.h"

namespace thorin {

inline size_t bitcount(uint64_t v) {
#if defined(__GNUC__) | defined(__clang__)
    return __builtin_popcountll(v);
#elif defined(_MSC_VER)
    return __popcnt64(v);
#else
    // see https://stackoverflow.com/questions/3815165/how-to-implement-bitcount-using-only-bitwise-operators
    auto c = v - ((v >>  1ull)      & 0x5555555555555555ull);
    c =          ((c >>  2ull)      & 0x3333333333333333ull) + (c & 0x3333333333333333ull);
    c =          ((c >>  4ull) + c) & 0x0F0F0F0F0F0F0F0Full;
    c =          ((c >>  8ull) + c) & 0x00FF00FF00FF00FFull;
    c =          ((c >> 16ull) + c) & 0x0000FFFF0000FFFFull;
    return       ((c >> 32ull) + c) & 0x00000000FFFFFFFFull;
#endif
}

/**
 * A tagged pointer: first 16 bits is tag, remaining 48 bits is the actual pointer.
 * For non-x86_64 there is a fallback implementation.
 */
template<class T>
class TaggedPtr {
public:
    TaggedPtr() {}
#if defined(__x86_64__) || (_M_X64)
    TaggedPtr(uint16_t tag, T* ptr)
        : tag_(tag)
        , ptr_(reinterpret_cast<int64_t>(ptr))
    {}
#else
    TaggedPtr(uint16_t tag, T* ptr)
        : tag_(tag)
        , ptr_(ptr)
    {}
#endif

    T* ptr() const { return reinterpret_cast<T*>(ptr_); }
    T* operator->() const { return ptr(); }
    operator T*() const { return ptr(); }
    void tag(uint16_t tag) { tag_ = tag; }
    uint16_t tag() const { return tag_; }
    bool operator==(TaggedPtr other) const { return this->ptr() == other.ptr() && this->tag() == other.tag(); }

private:
    uint16_t tag_;
#if defined(__x86_64__) || (_M_X64)
    int64_t ptr_ : 48; // sign extend to make pointer canonical
#else
    T* ptr_;
#endif
};

class BitSet {
public:
    class reference {
    private:
        reference(uint16_t index, uint64_t* word)
            : tagged_ptr_(index, word)
        {}

    public:
        reference operator=(bool b) {
            uint64_t mask = uint64_t(1) << index();
            if (b)
                word() |= mask;
            else
                word() &= ~mask;
            return *this;
        }
        operator bool() const { return word() & (uint64_t(1) << index()); }

    private:
        const uint64_t& word() const { return *tagged_ptr_.ptr(); }
        uint64_t& word() { return *tagged_ptr_.ptr(); }
        uint64_t index() const { return tagged_ptr_.tag(); }

        TaggedPtr<uint64_t> tagged_ptr_;
        friend class BitSet;
    };

    BitSet()
        : u64_(0)
    {}

    reference operator[](size_t i) {
        if (!on_heap()) {
            if (i <= 63)
                return reference(i, stack_word_ptr());
            else {
                // TODO
            }
        }
        THORIN_UNREACHABLE;
    }

    bool operator[](size_t i) const { return (*const_cast<BitSet*>(this))[i]; }

    size_t count() const {
        if (on_heap()) {
            auto words = heap_word_ptr();
            size_t result = 0;
            for (size_t i = 0, e = num_heap_words(); i != e; ++i)
                result += bitcount(words[i]);
            return result;
        }

        return bitcount(stack_word());
    }

    size_t all() const {
        if (on_heap()) {
            auto words = heap_word_ptr();
            size_t result = true;
            for (size_t i = 0, e = num_heap_words(); result && i != e; ++i)
                result &= words[i] == uint64_t(-1);
            return result;
        }

        return idata_ == std::numeric_limits<int64_t>::min();
    }

private:
    //void resize(size_t i) {
        //size_t num = (i+size_t(63)) / size_t(64);
    //}

    bool on_heap() const { return heap_; }
    uint64_t stack_word() const { assert(!on_heap()); return u64_; }
    uint64_t* stack_word_ptr() { assert(!on_heap()); return &u64_; }
    uint64_t* heap_word_ptr() const { assert(on_heap()); return reinterpret_cast<uint64_t*>(ptr_); }
    size_t num_heap_words() const { assert(on_heap()); return num_; }

    union {
        struct {
            unsigned heap_  :  1;
            unsigned num_   : 15;
            int64_t  ptr_   : 48; ///< sign-extend to make pointer canonical
        };

        struct {
            unsigned dummy_ :  1; ///< same as @p heap_ - just there for layouting
            int64_t idata_  : 63; ///< for sign extension.
        };

        uint64_t u64_;            ///< whole 64-bit word.
    };
};

}

#endif
