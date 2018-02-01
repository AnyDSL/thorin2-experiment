#ifndef THORIN_ANALYSES_SCOPE_H
#define THORIN_ANALYSES_SCOPE_H

#include <vector>

#include "thorin/def.h"
#include "thorin/util/array.h"
#include "thorin/util/stream.h"

namespace thorin {

template<bool> class CFG;
typedef CFG<true>  F_CFG;
typedef CFG<false> B_CFG;

class CFA;
class CFNode;

/**
 * A @p Scope represents a region of @p Cn%s which are live from the view of an @p entry @p Cn.
 * Transitively, all user's of the @p entry's parameters are pooled into this @p Scope.
 */
class Scope : public Streamable {
public:
    Scope(const Scope&) = delete;
    Scope& operator=(Scope) = delete;

    explicit Scope(Cn* entry);
    ~Scope();

    /// Invoke if you have modified sth in this Scope. @see for_each.
    Scope& update();

    //@{ misc getters
    Cn* entry() const { return entry_; }
    Cn* exit() const { return exit_; }
    World& world() const { return entry_->world(); }
    //@}

    //@{ get Def%s contained/not contained in this Scope
    const DefSet& defs() const { return defs_; }
    bool contains(const Def* def) const { return defs_.contains(def); }
    /// All @p Def%s referenced but @em not contained in this @p Scope.
    const DefSet& free() const;
    //@}

    //@{ simple CFA to construct a CFG
    const CFA& cfa() const;
    const F_CFG& f_cfg() const;
    const B_CFG& b_cfg() const;
    //@}

    //@{ dump
    // Note that we don't use overloading for the following methods in order to have them accessible from gdb.
    virtual std::ostream& stream(std::ostream&) const override;  ///< Streams thorin to file @p out.
    void write_thorin(const char* filename) const;               ///< Dumps thorin to file with name @p filename.
    void thorin() const;                                         ///< Dumps thorin to a file with an auto-generated file name.
    //@}

    /**
     * Transitively visits all @em reachable Scope%s in @p world that do not have free variables.
     * We call these Scope%s @em top-level Scope%s.
     * Select with @p elide_empty whether you want to visit trivial Scope%s of Cn%s without body.
     * @attention { If you change anything in the Scope passed to @p f, you must invoke @p update to recompute the Scope. }
     */
    template<bool elide_empty = true> static void for_each(const World& world, std::function<void(Scope&)> f);

private:
    void run();

    Cn* entry_;
    Cn* exit_;
    DefSet defs_;
    mutable std::unique_ptr<DefSet> free_;
    mutable std::unique_ptr<const CFA> cfa_;
};

}

#endif
