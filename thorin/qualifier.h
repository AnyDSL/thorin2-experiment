#ifndef THORIN_QUALIFIER_H
#define THORIN_QUALIFIER_H

namespace thorin {

/* This implements the substructural type qualifier lattice:
 *       Linear
 *       /    \
 *   Affine  Relevant
 *       \    /
 *      Unlimited
 * Where Linear is the largest element in the partial orders </<= and Unlimited the smallest.
 */
enum class QualifierTag : int32_t {
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
    return q == QualifierTag::Unlimited ? "ᵁ" :
        q == QualifierTag::Relevant ? "ᴿ" :
        q == QualifierTag::Affine ? "ᴬ" :
        "ᴸ";
}

inline std::ostream& operator<<(std::ostream& ostream, const QualifierTag q) {
    return ostream << qualifier2str(q);
}

/// Also known as the least upper bound.
inline QualifierTag join(QualifierTag lhs, QualifierTag rhs) {
    return QualifierTag(static_cast<int>(lhs) | static_cast<int>(rhs));
}

/// Also known as the greatest lower bound.
inline QualifierTag meet(QualifierTag lhs, QualifierTag rhs) {
    return QualifierTag(static_cast<int>(lhs) & static_cast<int>(rhs));
}

}

#endif
