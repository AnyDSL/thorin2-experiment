#ifndef THORIN_CHECk_H
#define THORIN_CHECk_H

#include "thorin/def.h"

namespace thorin {

class Occurrences {
public:
    Occurrences()
        : zero(true), one(false), many(false)
    {}
    Occurrences(bool zero, bool one, bool many)
        : zero(zero), one(one), many(many)
    {}

    QualifierTag to_qualifier() const {
        // glb(L, x) = x, so one not important here
        if (zero) {
            if (many)
                return QualifierTag::Unlimited; // glb(a, r)
            return QualifierTag::Affine;
        } else if (many) {
            return QualifierTag::Relevant;
        }
        return QualifierTag::Linear; // either one, or none set, most precise
    };

    static Occurrences join(const Occurrences& a, const Occurrences& b) {
        return Occurrences(a.zero | b.zero, a.one | b.one, a.many | b.many);
    }

    static Occurrences add(const Occurrences& a, const Occurrences& b) {
        return {(bool) (a.zero & b.zero),
                (bool) ((a.zero & b.one) | (b.zero & a.one)),
                (bool) ((a.one & b.one) | a.many | b.many)};
    }

    static Occurrences with_zero_set(const Occurrences& a) {
        return {true, a.one, a.many};
    }

private:
    bool zero;
    bool one;
    bool many;

    friend std::ostream& operator<<(std::ostream& os, const Occurrences& s);
};

std::ostream& operator<<(std::ostream& os, const Occurrences& s);


class TypeCheck {
public:
    void check(const Def* def) {
        DefVector types;
        check(def, types);
    }

    void check(const Def* def, DefVector& types);

    DefMap<Array<Occurrences>> occurrences;

private:
    typedef std::pair<DefArray, const Def*> EnvDef;

    struct EnvDefHash {
        static inline uint64_t hash(const EnvDef& p) {
            uint64_t hash = hash_begin(p.second->gid());
            for (auto def : p.first)
                hash = hash_combine(hash, def->gid());
            return hash;
        }
        static bool eq(const EnvDef& a, const EnvDef& b) { return a == b; };
        static EnvDef sentinel() { return EnvDef(DefArray(), nullptr); }
    };

    HashSet<EnvDef, EnvDefHash> done_;
};

}

#endif
