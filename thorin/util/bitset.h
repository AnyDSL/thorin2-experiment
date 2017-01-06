#ifndef THORIN_UTIL_BITSET_H
#define THORIN_UTIL_BITSET_H

#include <algorithm>

#include "thorin/util/utility.h"

#define THORIN_BITSET_OPS(m) m(|) m(&) m(^)

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
    BitSet(const BitSet& other)
        : BitSet()
    {
        make_room(other.num_bits()-1);
        std::copy(other.words(), other.words()+other.num_words(), words());
    }
    BitSet(BitSet&& other)
        : num_words_(std::move(other.num_words_))
        , words_(std::move(other.words_))
    {
        other.words_ = nullptr;
    }

    ~BitSet() {
        dealloc();
    }

    /// clears all bits
    void clear() {
        dealloc();
        num_words_ = 1;
        word_ = 0;
    }

    //@{ get, set, clear, toggle, and test bits
    bool test(size_t i) const {
        make_room(i);
        return *(words() + i/size_t(64)) & (uint64_t(1) << i%uint64_t(64));
    }

    BitSet& set(size_t i) {
        make_room(i);
        *(words() + i/size_t(64)) |= (uint64_t(1) << i%uint64_t(64));
        return *this;
    }

    BitSet& clear(size_t i) {
        make_room(i);
        *(words() + i/size_t(64)) &= ~(uint64_t(1) << i%uint64_t(64));
        return *this;
    }

    BitSet& toggle(size_t i) {
        make_room(i);
        *(words() + i/size_t(64)) ^= uint64_t(1) << i%uint64_t(64);
        return *this;
    }

    reference operator[](size_t i) {
        make_room(i);
        return reference(words() + i/size_t(64), i%uint64_t(64));
    }

    bool operator[](size_t i) const { return (*const_cast<BitSet*>(this))[i]; }
    //@}

    //@{ Is any bit (in range) set?
    bool any() const;
    /// Any bit range in @c [begin,end[ set?
    bool any_range(const size_t begin, const size_t end) const;
    /// Any bit range in @c [begin,begin+num[ set?
    bool any_length(const size_t begin, const size_t num) const { return any_range(begin, begin+num); }
    /// Any bit range in @c [0,end[ set?
    bool any_till(const size_t end) const { return any_range(0, end); }
    /// Any bit range in @c [begin,infinity[ set?
    bool any_from(const size_t begin) const { return any_range(begin, num_bits()); }
    //@}

    //@{ Is any bit (in range) set?
    bool none() const;
    /// Is no bit in @c [begin,end[ set?
    bool none_range(const size_t begin, const size_t end) const;
    /// Is no bit in @c [0,end[ set?
    bool none_till(const size_t end) const { return none_range(0, end); }
    /// Is no bit in @c [begin,infinity[ set?
    bool none_from(const size_t begin) const { return none_range(begin, num_bits()); }
    //@}

    //@{ shift
    BitSet& operator>>=(uint64_t shift);
    BitSet operator>>(uint64_t shift) const { BitSet res(*this); res >>= shift; return res; }
    //@}

    //@{ boolean operators
#define CODE(op)                                   \
    BitSet& operator op ## =(const BitSet& other); \
    BitSet operator op (BitSet b) const { BitSet res(*this); res op ## = b; return res; }
THORIN_BITSET_OPS(CODE)
#undef CODE
    //@}

    /// number of bits set
    size_t count() const;

    void friend swap(BitSet& b1, BitSet& b2) {
        using std::swap;
        swap(b1.num_words_, b2.num_words_);
        swap(b1.words_,     b2.words_);
    }

    BitSet& operator=(BitSet other) { swap(*this, other); return *this; }

private:
    void dealloc() const {
        if (num_words_ != 1)
            delete[] words_;
    }

    void make_room(size_t i) const {
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
    size_t num_bits() const { return num_words_*size_t(64); }

    mutable size_t num_words_;
    union {
        mutable uint64_t* words_;
        uint64_t word_;
    };
};

}

#endif
