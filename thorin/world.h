#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <memory>
#include <string>

#include "thorin/def.h"

namespace thorin {

class World {
public:
    struct DefHash {
        static uint64_t hash(const Def* def) { return def->hash(); }
        static bool eq(const Def* d1, const Def* d2) { return d2->equal(d1); }
        static const Def* sentinel() { return (const Def*)(1); }
    };

    typedef HashSet<const Def*, DefHash> DefSet;

    World& operator=(const World&) = delete;
    World(const World&) = delete;

    World();
    ~World();

    //@{ create universe and kinds
    const Universe* universe() const { return universe_; }
    const ArityKind* arity_kind(QualifierTag q = QualifierTag::Unlimited) const { return arity_kind_[size_t(q)]; }
    const ArityKind* arity_kind(const Def* q) {
        if (auto cq = q->isa<Qualifier>())
            return arity_kind(cq->qualifier_tag());
        return unify<ArityKind>(1, *this, q);
    }
    const MultiArityKind* multi_arity_kind(QualifierTag q = QualifierTag::Unlimited) const { return multi_arity_kind_[size_t(q)]; }
    const MultiArityKind* multi_arity_kind(const Def* q) {
        if (auto cq = q->isa<Qualifier>())
            return multi_arity_kind(cq->qualifier_tag());
        return unify<MultiArityKind>(1, *this, q);
    }
    const Star* star(QualifierTag q = QualifierTag::Unlimited) const { return star_[size_t(q)]; }
    const Star* star(const Def* q) {
        if (auto cq = q->isa<Qualifier>())
            return star(cq->qualifier_tag());
        return unify<Star>(1, *this, q);
    }
    //@}

    //@{ create qualifier
    const QualifierType* qualifier_type() const { return qualifier_type_; }
    const Qualifier* qualifier(QualifierTag q = QualifierTag::Unlimited) const { return qualifier_[size_t(q)]; }
    const Qualifier* unlimited() const { return qualifier(QualifierTag::Unlimited); }
    const Qualifier* affine() const { return qualifier(QualifierTag::Affine); }
    const Qualifier* linear() const { return qualifier(QualifierTag::Linear); }
    const Qualifier* relevant() const { return qualifier(QualifierTag::Relevant); }
    const std::array<const Qualifier*, 4>& qualifiers() const { return qualifier_; }
    //@}

    //@{ create Pi
    const Pi* pi(const Def* domain, const Def* body, Debug dbg = {}) {
        return pi(domain, body, unlimited(), dbg);
    }
    const Pi* pi(const Def* domain, const Def* body, const Def* qualifier, Debug dbg = {});
    //@}

    //@{ create Lambda
    const Def* lambda(const Def* domain, const Def* body, Debug dbg = {}) {
        return lambda(domain, body, unlimited(), dbg);
    }
    const Def* lambda(const Def* domain, const Def* body, const Def* type_qualifier, Debug dbg = {});
    /// @em nominal lambda --- may be recursive
    Lambda* lambda(const Pi* type, Debug dbg = {}) {
        assertf(type->free_vars().none_begin(1),
                "function type {} of a nominal lambda may not contain free variables", type);
        return insert<Lambda>(1, type, dbg);
    }
    //@}

    //@{ create App
    const Def* app(const Def* callee, Defs args, Debug dbg = {}) {
        return app(callee, tuple(args, dbg), dbg);
    }
    const Def* app(const Def* callee, const Def* arg, Debug dbg = {});
    const Def* raw_app(const Def* callee, Defs args, Debug dbg = {}) { return raw_app(callee, tuple(args, dbg), dbg); }
    const Def* raw_app(const Def* callee, const Def* arg, Debug dbg = {}) {
        auto type = callee->type()->as<Pi>()->apply(arg);
        return unify<App>(2, type, callee, arg, dbg);
    }
    //@}

    //@{ create Units
    const Def* unit(QualifierTag q = QualifierTag::Unlimited) { return unit_[size_t(q)]; }
    const Def* unit(const Def* q) {
        if (auto cq = q->isa<Qualifier>())
            return unit(cq->qualifier_tag());
        return arity(1, q);
    }
    const Def* unit_kind(QualifierTag q = QualifierTag::Unlimited) { return variadic(arity(0), star(q)); }
    const Def* unit_kind(const Def* q) { return variadic(arity(0), star(q)); }
    //@}

    //@{ create Sigma
    /// @em structural Sigma types or kinds
    const Def* sigma(Defs defs, Debug dbg = {}) { return sigma(nullptr, defs, dbg); }
    const Def* sigma(const Def* qualifier, Defs, Debug dbg = {});
    /// Nominal sigma types or kinds
    Sigma* sigma(const Def* type, size_t num_ops, Debug dbg = {}) {
        return insert<Sigma>(num_ops, type, num_ops, dbg);
    }
    /// @em nominal Sigma of type Star
    Sigma* sigma_type(size_t num_ops, Debug dbg = {}) {
        return sigma_type(unlimited(), num_ops, dbg);
    }
    /// @em nominal Sigma of type Star
    Sigma* sigma_type(const Def* qualifier, size_t num_ops, Debug dbg = {}) {
        return sigma(star(qualifier), num_ops, dbg);
    }
    /// @em nominal Sigma of type Universe
    Sigma* sigma_kind(size_t num_ops, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, num_ops, dbg);
    }
    //@}

    //@{ create Variadic
    const Def* variadic(const Def* arities, const Def* body, Debug dbg = {});
    const Def* variadic(Defs arities, const Def* body, Debug dbg = {});
    const Def* variadic(u64 a, const Def* body, Debug dbg = {}) {
        return variadic(arity(a, QualifierTag::Unlimited, dbg), body, dbg);
    }
    const Def* variadic(ArrayRef<u64> a, const Def* body, Debug dbg = {}) {
        return variadic(DefArray(a.size(), [&](auto i) {
                    return this->arity(a[i], QualifierTag::Unlimited, dbg);
                }), body, dbg);
    }
    //@}

    //@{ create Tuple
    const Def* tuple(Defs defs, Debug dbg = {});
    //@}

    //@{ create unit values
    const Def* val_unit(QualifierTag q = QualifierTag::Unlimited) { return unit_val_[size_t(q)]; }
    const Def* val_unit(const Def* q) {
        if (auto cq = q->isa<Qualifier>())
            return val_unit(cq->qualifier_tag());
        return index_zero(arity(1, q));
    }
    const Def* val_unit_kind(QualifierTag q = QualifierTag::Unlimited) { return pack(arity(0), unit(q)); }
    const Def* val_unit_kind(const Def* q) { return pack(arity(0), unit(q)); }
    //@}

    //@{ create Pack
    const Def* pack(const Def* arities, const Def* body, Debug dbg = {});
    const Def* pack(Defs arities, const Def* body, Debug dbg = {});
    const Def* pack(u64 a, const Def* body, Debug dbg = {}) {
        return pack(arity(a, QualifierTag::Unlimited, dbg), body, dbg);
    }
    const Def* pack(ArrayRef<u64> a, const Def* body, Debug dbg = {}) {
        return pack(DefArray(a.size(), [&](auto i) {
                    return this->arity(a[i], QualifierTag::Unlimited, dbg);
                }), body, dbg);
    }
    //@}

    //@{ create Extract
    const Def* extract(const Def* def, const Def* index, Debug dbg = {});
    const Def* extract(const Def* def, u64 index, Debug dbg = {});
    //@}

    //@{ create Insert
    const Def* insert(const Def* def, const Def* index, const Def* value, Debug dbg = {});
    const Def* insert(const Def* def, u64 index, const Def* value, Debug dbg = {});
    //@}

    //@{ create Index
    const Lit* index(u64 arity, u64 idx, Location location = {}) {
        return index(this->arity(arity), idx, location);
    }
    const Lit* index(const Arity* arity, u64 index, Location location = {});
    const Def* index_zero(const Def* arity, Location location = {});
    const Def* index_succ(const Def* index, Debug dbg = {});
    //@}

    //@{ create Arity
    const Arity* arity(u64 a, QualifierTag q = QualifierTag::Unlimited, Location location = {}) {
        return arity(a, qualifier(q), location);
    }
    const Arity* arity(u64 a, const Def* q, Location location = {});
    const Def* arity_succ() { return arity_succ_; }
    const Def* arity_succ(const Def* arity, Debug dbg = {});
    const Def* arity_eliminator() const { return arity_eliminator_; }
    //@}

    //@{ misc factory methods
    const Def* any(const Def* type, const Def* def, Debug dbg = {});
    const Axiom* axiom(const Def* type, Debug dbg = {});
    const Axiom* axiom(const char* s) { return find(axioms_, s); }
    const Axiom* axiom(const char* name, const char* s);
    const Lit* lit(const Def* type, Box box, Debug dbg = {}) { return unify<Lit>(0, type, box, dbg); }
    const Def* intersection(Defs defs, Debug dbg = {});
    const Def* intersection(const Def* type, Defs defs, Debug dbg = {});
    const Error* error(const Def* type) { return unify<Error>(0, type); }
    const Def* match(const Def* def, Defs handlers, Debug dbg = {});
    const Def* pick(const Def* type, const Def* def, Debug dbg = {});
    const Def* singleton(const Def* def, Debug dbg = {});
    const Var* var(Defs types, u64 index, Debug dbg = {}) { return var(sigma(types), index, dbg); }
    const Var* var(const Def* type, u64 index, Debug dbg = {}) { return unify<Var>(0, type, index, dbg); }
    const Def* variant(Defs defs, Debug dbg = {});
    const Def* variant(const Def* type, Defs defs, Debug dbg = {});
    Variant* variant(const Def* type, size_t num_ops, Debug dbg = {}) {
        assert(num_ops > 1 && "it should not be necessary to build empty/unary variants");
        return insert<Variant>(num_ops, type, num_ops, dbg);
    }
    //@}

    //@{ bool and nat types
    const Arity* type_bool() { return type_bool_; }
    const Axiom* type_nat() { return type_nat_; }
    //@}

    //@{ values for bool and nat
    const Lit* lit_nat(int64_t val, Location location = {});
    const Lit* lit_nat_0() { return lit_nat_0_; }
    const Lit* lit_nat_1() { return lit_nat_[0]; }
    const Lit* lit_nat_2() { return lit_nat_[1]; }
    const Lit* lit_nat_4() { return lit_nat_[2]; }
    const Lit* lit_nat_8() { return lit_nat_[3]; }
    const Lit* lit_nat_16() { return lit_nat_[4]; }
    const Lit* lit_nat_32() { return lit_nat_[5]; }
    const Lit* lit_nat_64() { return lit_nat_[6]; }

    const Lit* lit(bool val) { return lit_bool_[size_t(val)]; }
    const Lit* lit_false() { return lit_bool_[0]; }
    const Lit* lit_true()  { return lit_bool_[1]; }
    //@}

    //@{ continuations
    const CnType* cn_type(const Def* domain, Debug dbg = {});
    const CnType* cn_type(Defs domain, Debug dbg = {}) { return cn_type(sigma(domain), dbg); }
    const Cn*     cn(const Def* domain, Debug dbg = {});
    //@}

    //@{ externals
    const StrMap<const Def*>& externals() const { return externals_; }
    void make_external(const Def* def) {
        auto [i, success] = externals_.emplace(def->name().c_str(), def);
        assert_unused(success || i->second == def);
    }
    const Def* lookup_external(const char* s) const {
        auto i = externals_.find(s);
        assert(i != externals_.end());
        return i->second;
    }
    //@}

    //@{ misc
    const DefSet& defs() const { return defs_; }
    const App* curry_normalizer(const Def* callee, const Def* arg, Debug dbg) {
        if (auto pi = callee->type()->isa<Pi>(); pi->body()->isa<Pi>())
            return raw_app(callee, arg, dbg)->set_normalizer(callee->normalizer())->as<App>();
        return nullptr;
    }
    //@}

    friend void swap(World& w1, World& w2) {
        using std::swap;
        swap(w1.defs_, w2.defs_);
        swap(w1.universe_->world_, w2.universe_->world_);
    }

private:
    struct Lattice {
        QualifierTag min, max;
        bool (*q_less)(QualifierTag, QualifierTag);
        QualifierTag (*q_join)(QualifierTag, QualifierTag);
        const Def* (World::*join)(const Def*, Defs, Debug);
        const char* short_name;
        const char* full_name;
    };

    static constexpr Lattice LUB{QualifierTag::u, QualifierTag::l,
                                 &thorin::operator<, thorin::lub, &thorin::World::variant,
                                 "less",    "least upper bound"};
    static constexpr Lattice GLB{QualifierTag::l, QualifierTag::u,
                                 &thorin::operator<, thorin::lub, &thorin::World::intersection,
                                 "greater", "greatest lower bound"};

    template<class I>
    const Def* bound(Lattice, Range<I> ops, const Def* q, bool require_qualifier = true);
    const Def* bound(Lattice l, Defs ops, const Def* q, bool require_qualifier = true) {
        return bound(l, range(ops), q, require_qualifier);
    }
    template<class I>
    const Def* type_bound(Lattice l, Range<I> ops, const Def* q, bool require_qualifier = true) {
        return bound(l, map_range(ops, [&] (auto def) { return def->type(); }), q, require_qualifier);
    }
    const Def* type_bound(Lattice l, Defs ops, const Def* q, bool require_qualifier = true) {
        return type_bound(l, range(ops), q, require_qualifier);
    }
    template<class I, class F>
    const Def* qualifier_bound(Lattice l, Range<I> defs, F f);
    template<class F>
    const Def* qualifier_bound(Lattice l, Defs defs, F f) {
        return qualifier_bound(l, range(defs), f);
    }

protected:
    template<class T, class... Args>
    const T* unify(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        assert(!def->is_nominal());
        auto [i, success] = defs_.emplace(def);
        if (success) {
            def->finalize();
            return def;
        }

        --Def::gid_counter_;
        dealloc(def);
        return static_cast<const T*>(*i);
    }

    template<class T, class... Args>
    T* insert(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        auto p = defs_.emplace(def);
        assert_unused(p.second);
        return def;
    }

    struct Zone {
        static const size_t Size = 1024 * 1024 - sizeof(std::unique_ptr<int>); // 1MB - sizeof(next)
        std::unique_ptr<Zone> next;
        char buffer[Size];
    };

    static bool alloc_guard_;

    template<class T, class... Args>
    T* alloc(size_t num_ops, Args&&... args) {
        assert((alloc_guard_ = !alloc_guard_) && "you are not allowed to recursively invoke alloc");
        size_t num_bytes = sizeof(T) + sizeof(const Def*) * num_ops;
        assert(num_bytes < Zone::Size);

        if (buffer_index_ + num_bytes >= Zone::Size) {
            auto page = new Zone;
            cur_page_->next.reset(page);
            cur_page_ = page;
            buffer_index_ = 0;
        }

        auto result = new (cur_page_->buffer + buffer_index_) T(args...);
        buffer_index_ += num_bytes;
        assert(buffer_index_ % alignof(T) == 0);

#ifndef NDEBUG
        alloc_guard_ = !alloc_guard_;
#endif
        return result;
    }

    template<class T>
    void dealloc(const T* def) {
        size_t num_bytes = sizeof(T) + def->num_ops() * sizeof(const Def*);
        def->~T();
        if (ptrdiff_t(buffer_index_ - num_bytes) > 0) // don't care otherwise
            buffer_index_-= num_bytes;
        assert(buffer_index_ % alignof(T) == 0);
    }
    std::unique_ptr<Zone> root_page_;
    Zone* cur_page_;
    size_t buffer_index_ = 0;
    DefSet defs_;
    StrMap<const Axiom*> axioms_;
    StrMap<const Def*> externals_;
    const Universe* universe_;
    const QualifierType* qualifier_type_;
    const Axiom* arity_succ_;
    const Axiom* arity_eliminator_;
    const Axiom* arity_eliminator_arity_;
    const Axiom* arity_eliminator_multi_;
    const Axiom* arity_eliminator_star_;
    const Axiom* index_zero_;
    const Axiom* index_succ_;
    std::array<const Qualifier*, 4> qualifier_;
    std::array<const Star*,  4> star_;
    std::array<const Def*, 4> unit_;
    std::array<const Def*, 4> unit_val_;
    std::array<const Def*, 4> unit_kind_;
    std::array<const Def*, 4> unit_kind_val_;
    std::array<const ArityKind*, 4> arity_kind_;
    std::array<const MultiArityKind*, 4> multi_arity_kind_;
    const Arity* type_bool_;
    const Axiom* type_nat_;
    const Lit* lit_nat_0_;
    std::array<const Lit*, 2> lit_bool_;
    std::array<const Lit*, 7> lit_nat_;
};

inline const Def* app_callee(const Def* def) { return def->as<App>()->callee(); }
inline const Def* app_arg(const Def* def) { return def->as<App>()->arg(); }
inline const Def* app_arg(const Def* def, u64 i) { return def->world().extract(app_arg(def), i); }

}

#endif
