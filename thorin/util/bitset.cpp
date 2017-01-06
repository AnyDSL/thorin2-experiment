#include "thorin/util/bitset.h"

namespace thorin {

size_t BitSet::count() const {
    size_t result = 0;
    auto w = words();
    for (size_t i = 0, e = num_words(); i != e; ++i)
        result += bitcount(w[i]);
    return result;
}

bool BitSet::any() const {
    bool result = false;
    auto w = words();
    for (size_t i = 0, e = num_words(); !result && i != e; ++i)
        result |= w[i] & uint64_t(-1);
    return result;
}

bool BitSet::any_range(const size_t begin, const size_t end) const {
    // TODO optimize
    bool result = false;
    for (size_t i = begin; !result && i != end; ++i)
        result |= test(i);
    return result;
}

bool BitSet::none() const {
    bool result = true;
    auto w = words();
    for (size_t i = 0, e = num_words(); result && i != e; ++i)
        result &= w[i] == uint64_t(0);
    return result;
}

BitSet& BitSet::operator>>=(uint64_t shift) {
    uint64_t div = shift/uint64_t(64);
    uint64_t rem = shift%uint64_t(64);
    auto w = words();

    for (size_t i = 0, e = num_words()-div; i != e; ++i)
        w[i] = w[i+div];
    std::fill(w+num_words()-div, w+num_words(), 0);

    uint64_t carry = 0;
    for (size_t i = num_words()-div; i-- != 0;) {
        uint64_t new_carry = w[i] << (uint64_t(64)-rem);
        w[i] = (w[i] >> rem) | carry;
        carry = new_carry;
    }

    return *this;
}

#define CODE(op)                                        \
BitSet& BitSet::operator op ## =(const BitSet& other) { \
    if (this->num_words() < other.num_words())          \
        this->make_room(other.num_bits()-1);            \
    else if (other.num_words() < this->num_words())     \
        other.make_room(this->num_bits()-1);            \
    auto  this_words = this->words();                   \
    auto other_words = other.words();                   \
    for (size_t i = 0, e = num_words(); i != e; ++i)    \
        this_words[i] op ## = other_words[i];           \
    return *this;                                       \
}
THORIN_BITSET_OPS(CODE)
#undef CODE

}
