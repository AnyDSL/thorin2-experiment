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
enum class Qualifier {
  Unlimited,
  Relevant = 1 << 0,
  Affine   = 1 << 1,
  Linear = Affine | Relevant,
};

/// Linear is the largest, Unlimited the smallest.
inline bool operator<(Qualifier lhs, Qualifier rhs) {
  if (lhs == rhs) return false;
  if (lhs == Qualifier::Unlimited) return true;
  if (rhs == Qualifier::Linear) return true;
  return false;
}

inline bool operator<=(Qualifier lhs, Qualifier rhs) {
  return lhs == rhs || lhs < rhs;
}

constexpr const char* qualifier_cstr(Qualifier q) {
  return q == Qualifier::Unlimited ? "" :
    q == Qualifier::Relevant ? "ᴿ" :
    q == Qualifier::Affine ? "ᴬ" :
    "ᴸ";
}

inline std::ostream& operator<<(std::ostream& ostream, const Qualifier q) {
  return ostream << qualifier_cstr(q);
}

/// Also known as the least upper bound.
inline Qualifier join(Qualifier lhs, Qualifier rhs) {
  return Qualifier(static_cast<int>(lhs) | static_cast<int>(rhs));
}

/// Also known as the greatest lower bound.
inline Qualifier meet(Qualifier lhs, Qualifier rhs) {
    return Qualifier(static_cast<int>(lhs) & static_cast<int>(rhs));
}

}

#endif
