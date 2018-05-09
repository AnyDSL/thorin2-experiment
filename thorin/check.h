#ifndef THORIN_CHECk_H
#define THORIN_CHECk_H

#include "thorin/def.h"

namespace thorin {

class TypeCheck {
public:
    void check(const Def* def) {
        DefVector types;
        check(def, types);
    }

    void check(const Def* def, DefVector& types);

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
