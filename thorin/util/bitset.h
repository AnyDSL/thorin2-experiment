#ifndef THORIN_UTIL_BITSET_H
#define THORIN_UTIL_BITSET_H

#include <algorithm>

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
        : num_words_(1)
        , word_(0)
    {}

    ~BitSet() { dealloc(); }

    //@{ get, set, clear, toggle, and test bits
    bool test(size_t i) {
        make_room(i);
        return *(words() + i/size_t(64)) & (UINT64_C(1) << i%UINT64_C(64));
    }

    BitSet& set(size_t i) {
        make_room(i);
        *(words() + i/size_t(64)) |= (UINT64_C(1) << i%UINT64_C(64));
        return *this;
    }

    BitSet& clear(size_t i) {
        make_room(i);
        *(words() + i/size_t(64)) &= ~(UINT64_C(1) << i%UINT64_C(64));
        return *this;
    }

    BitSet& toggle(size_t i) {
        make_room(i);
        *(words() + i/size_t(64)) ^= UINT64_C(1) << i%UINT64_C(64);
        return *this;
    }

    reference operator[](size_t i) {
        make_room(i);
        return reference(words() + i/size_t(64), i%UINT64_C(64));
    }

    bool operator[](size_t i) const { return (*const_cast<BitSet*>(this))[i]; }
    //@}

    size_t count() const {
        size_t result = 0;
        auto w = words();
        for (size_t i = 0, e = num_words(); i != e; ++i)
            result += bitcount(w[i]);
        return result;
    }

    bool any() const {
        bool result = false;
        auto w = words();
        for (size_t i = 0, e = num_words(); !result && i != e; ++i)
            result |= w[i] & uint64_t(-1);
        return result;
    }

private:
    void dealloc() {
        if (num_words_ != 1)
            delete[] words_;
    }

    void make_room(size_t i) {
        size_t num_new_words = (i+size_t(64)) / size_t(64);
        if (num_new_words > num_words_) {
            num_new_words = round_to_power_of_2(num_new_words);
            assert(num_new_words >= num_words_ * size_t(2)
                    && "num_new_words must be a power of two at least twice of num_words_");
            uint64_t* new_words = new uint64_t[num_new_words];

            // copy over and fill rest with zero
            std::fill(std::copy(words(), words() + num_words_, new_words), new_words + num_new_words, 0);

            // record new num_words and words_ pointer
            dealloc();
            num_words_ = num_new_words;
            words_ = new_words;
        }
    }

    const uint64_t* words() const { return num_words_ == 1 ? &word_ : words_; }
    uint64_t* words() { return num_words_ == 1 ? &word_ : words_; }
    size_t num_words() const { return num_words_; }

    size_t num_words_;
    union {
        uint64_t* words_;
        uint64_t word_;
    };
};

}

#endif
