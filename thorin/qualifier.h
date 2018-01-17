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
    Unlimited,
    Relevant = 1 << 0,
    Affine   = 1 << 1,
    Linear = Affine | Relevant,
    u = Unlimited, r = Relevant, a = Affine, l = Linear,
    Num
};

/// Linear is the largest, Unlimited the smallest.
inline bool operator<(QualifierTag lhs, QualifierTag rhs) {
    if (lhs == rhs) return false;
    if (lhs == QualifierTag::Unlimited) return true;
    if (rhs == QualifierTag::Linear) return true;
    return false;
}

inline bool operator<=(QualifierTag lhs, QualifierTag rhs) {
    return lhs == rhs || lhs < rhs;
}

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
inline QualifierTag lub(QualifierTag lhs, QualifierTag rhs) {
    return QualifierTag(static_cast<int>(lhs) | static_cast<int>(rhs));
}

/// greatest lower bound
inline QualifierTag glb(QualifierTag lhs, QualifierTag rhs) {
    return QualifierTag(static_cast<int>(lhs) & static_cast<int>(rhs));
}

}

#endif
