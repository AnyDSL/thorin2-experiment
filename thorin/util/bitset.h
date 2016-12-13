#ifndef THORIN_UTIL_BITSET_H
#define THORIN_UTIL_BITSET_H

#include "thorin/util/utility.h"

namespace thorin {

/**
 * A tagged pointer: first 16 bits is index, remaining 48 bits is the actual pointer.
 * For non-x86_64 there is a fallback impplementation.
 */
template<class T>
class TaggedPtr {
public:
    TaggedPtr() {}
#if defined(__x86_64__) || (_M_X64)
    TaggedPtr(uint16_t index, T* ptr)
        : uptr_(reinterpret_cast<uint64_t>(ptr) | (uint64_t(index) << 48ull))
    {}

    uint16_t index() const { return uptr_ >> 48ull; }
    void tag(uint16_t index) const { uptr_ = (uint64_t(index) << 48ull) | (uptr_ & 0x0000FFFFFFFFFFFFull); }
    T* ptr() const {
        // sign extend to make pointer canonical
        return reinterpret_cast<T*>((iptr_  << 16) >> 16) ;
    }
    bool operator==(TaggedPtr other) const { return this->uptr_ == other.uptr_; }
#else
    TaggedPtr(uint16_t index, T* ptr)
        : index_(index)
        , ptr_(ptr)
    {}

    void tag(uint16_t index) { index_ = index; }
    uint16_t index() const { return index_; }
    T* ptr() const { return ptr_; }
    bool operator==(TaggedPtr other) const { return this->ptr() == other.def() && this->index() == other.index(); }
#endif
    T* operator->() const { return ptr(); }
    operator T*() const { return ptr(); }

private:
#if defined(__x86_64__) || (_M_X64)
    union {
        uint64_t uptr_;
        int64_t iptr_;
    };
#else
    uint16_t index_;
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
        uint64_t index() const { return tagged_ptr_.index(); }

        TaggedPtr<uint64_t> tagged_ptr_;
        friend class BitSet;
    };

    BitSet()
        : word_(0)
    {}

    reference operator[](size_t i) {
        //if (!on_heap()) {
            //if (i <= 63)
                return reference(i, stack_word_ptr());
            //else {
            //}
        //}
    }

    bool operator[](size_t i) const { return (*const_cast<BitSet*>(this))[i]; }

private:
    //void resize(size_t i) {
        //size_t num = (i+size_t(63)) / size_t(64);
    //}

    bool on_heap() const { return word_ & 0x8000000000000000ull; }
    uint64_t stack_word() const { assert(!on_heap()); return word_; }
    uint64_t* stack_word_ptr() { assert(!on_heap()); return &word_; }
    size_t num_heap_words() const { assert(on_heap()); return (word_ & 0x7FFFFFFFFFFFFFFFull) >> 48ull; }
    uint64_t* heap_word_ptr() const {
        assert(on_heap());
        return reinterpret_cast<uint64_t*>((iptr_ << 16) >> 16);
    }

    union {
        uint64_t word_;
        uint64_t* words_;
        intptr_t iptr_;
    };
};

}

#endif
