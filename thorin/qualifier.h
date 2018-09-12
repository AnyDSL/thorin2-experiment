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
enum class QualifierTag {
    u = 0,      ///< unlimited
    r = 1 << 0, ///< relevant -> min 1 use
    a = 1 << 1, ///< affine   -> max 1 use
    l = a | r   ///< linear -> must use exactly once
};

constexpr std::array<QualifierTag, 4> QualifierTags = {{QualifierTag::u, QualifierTag::r, QualifierTag::a, QualifierTag::l}};

/// Linear is the largest, Unlimited the smallest.
inline bool operator<(QualifierTag lhs, QualifierTag rhs) {
    if (lhs == rhs) return false;
    if (lhs == QualifierTag::u) return true;
    if (rhs == QualifierTag::l) return true;
    return false;
}
inline bool operator> (QualifierTag lhs, QualifierTag rhs) { return rhs < lhs; }
// DO NOT "OPTIMIZE" (QualifierTags, <=) is a partially ordered set: !(lhs > rhs) does not work!
inline bool operator<=(QualifierTag lhs, QualifierTag rhs) { return lhs == rhs || lhs < rhs ; }
inline bool operator>=(QualifierTag lhs, QualifierTag rhs) { return rhs <= lhs; }

constexpr const char* qualifier2str(QualifierTag q) {
    switch (q) {
        case QualifierTag::u: return "ᵁ";
        case QualifierTag::r: return "ᴿ";
        case QualifierTag::a: return "ᴬ";
        case QualifierTag::l: return "ᴸ";
        default: THORIN_UNREACHABLE;
    }
}

inline std::ostream& operator<<(std::ostream& ostream, const QualifierTag q) {
    return ostream << qualifier2str(q);
}

/// least upper bound
inline QualifierTag lub(QualifierTag a, QualifierTag b) {
    return QualifierTag(static_cast<int>(a) | static_cast<int>(b));
}

/// greatest lower bound
inline QualifierTag glb(QualifierTag a, QualifierTag b) {
    return QualifierTag(static_cast<int>(a) & static_cast<int>(b));
}

}

#endif
