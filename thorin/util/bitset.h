#ifndef THORIN_UTIL_BITSET_H
#define THORIN_UTIL_BITSET_H

#include <limits>

#include "thorin/util/utility.h"

namespace thorin {

class BitSet {
public:
    class reference {
    private:
        reference(uint64_t* word, uint16_t index)
            : tagged_ptr_(word, index)
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
        uint64_t index() const { return tagged_ptr_.index(); }

        TaggedPtr<uint64_t> tagged_ptr_;
        friend class BitSet;
    };

    BitSet()
        : u64_(0)
    {}

    reference operator[](size_t i) {
        if (!on_heap()) {
            if (i <= 63)
                return reference(stack_word_ptr(), i);
            else {
                // TODO
            }
        }
        THORIN_UNREACHABLE;
    }

    bool operator[](size_t i) const { return (*const_cast<BitSet*>(this))[i]; }
    bool test(size_t i) { return operator[](i); }

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
