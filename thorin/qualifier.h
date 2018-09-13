#ifndef THORIN_QUALIFIER_H
#define THORIN_QUALIFIER_H

#include <ostream>

#include "thorin/util/utility.h"

namespace thorin {

/* This implements the substructural type qualifier lattice:
 *       Linear
 *       /    \
 *   Affine  Relevant
 *       \    /
 *      Unlimited
 * Where Linear is the largest element in the partial orders </<= and Unlimited the smallest.
 */
enum class Qualifier {
    u = 0,      ///< unlimited
    r = 1 << 0, ///< relevant -> min 1 use
    a = 1 << 1, ///< affine   -> max 1 use
    l = a | r   ///< linear   -> must use exactly once
};

constexpr std::array<Qualifier, 4> Qualifiers = {{Qualifier::u, Qualifier::r, Qualifier::a, Qualifier::l}};

/// Linear is the largest, Unlimited the smallest.
inline bool operator<(Qualifier lhs, Qualifier rhs) { return lhs != rhs && (lhs == Qualifier::u || rhs == Qualifier::l); }
inline bool operator>(Qualifier lhs, Qualifier rhs) { return rhs < lhs; }
// DO NOT "OPTIMIZE" (Qualifiers, <=) is a partially ordered set: !(lhs > rhs) does not work!
inline bool operator<=(Qualifier lhs, Qualifier rhs) { return lhs == rhs || lhs < rhs ; }
inline bool operator>=(Qualifier lhs, Qualifier rhs) { return rhs <= lhs; }

constexpr const char* qualifier2str(Qualifier q) {
    switch (q) {
        case Qualifier::u: return "ᵁ";
        case Qualifier::r: return "ᴿ";
        case Qualifier::a: return "ᴬ";
        case Qualifier::l: return "ᴸ";
        default: THORIN_UNREACHABLE;
    }
}

inline std::ostream& operator<<(std::ostream& ostream, const Qualifier q) { return ostream << qualifier2str(q); }
inline Qualifier lub(Qualifier a, Qualifier b) { return Qualifier(int(a) | int(b)); } ///< least upper bound
inline Qualifier glb(Qualifier a, Qualifier b) { return Qualifier(int(a) & int(b)); } ///< greatest lower bound

}

#endif
