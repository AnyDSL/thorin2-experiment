#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <memory>
#include <string>

#include "thorin/def.h"
#include "thorin/tables.h"
#include "thorin/traits.h"

namespace thorin {

class WorldBase {
public:
    struct DefHash {
        static uint64_t hash(const Def* def) { return def->hash(); }
        static bool eq(const Def* d1, const Def* d2) { return d2->equal(d1); }
        static const Def* sentinel() { return (const Def*)(1); }
    };

    typedef HashSet<const Def*, DefHash> DefSet;

    WorldBase& operator=(const WorldBase&) = delete;
    WorldBase(const WorldBase&) = delete;

    WorldBase();
    ~WorldBase();

    //@{ create universe and kinds
    const Universe* universe() const { return universe_; }
    const ArityKind* arity_kind(Qualifier q = Qualifier::Unlimited) const { return arity_kind_[size_t(q)]; }
    const ArityKind* arity_kind(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return arity_kind(cq->box().get_qualifier());
        return unify<ArityKind>(1, *this, q);
    }
    const MultiArityKind* multi_arity_kind(Qualifier q = Qualifier::Unlimited) const { return multi_arity_kind_[size_t(q)]; }
    const MultiArityKind* multi_arity_kind(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return multi_arity_kind(cq->box().get_qualifier());
        return unify<MultiArityKind>(1, *this, q);
    }
    const Star* star(Qualifier q = Qualifier::Unlimited) const { return star_[size_t(q)]; }
    const Star* star(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return star(cq->box().get_qualifier());
        return unify<Star>(1, *this, q);
    }
    //@}

    //@{ create qualifier
    const Axiom* qualifier_kind() const { return qualifier_kind_; }
    const Axiom* qualifier_type() const { return qualifier_type_; }
    const Axiom* qualifier(Qualifier q = Qualifier::Unlimited) const { return qualifier_[size_t(q)]; }
    const Axiom* unlimited() const { return qualifier(Qualifier::Unlimited); }
    const Axiom* affine() const { return qualifier(Qualifier::Affine); }
    const Axiom* linear() const { return qualifier(Qualifier::Linear); }
    const Axiom* relevant() const { return qualifier(Qualifier::Relevant); }
    const std::array<const Axiom*, 4>& qualifiers() const { return qualifier_; }
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
    //@}

    //@{ create App
    const Def* app(const Def* callee, Defs args, Debug dbg = {}) {
        return app(callee, tuple(args, dbg), dbg);
    }
    const Def* app(const Def* callee, const Def* arg, Debug dbg = {});
    //@}

    //@{ create Units
    const Def* unit(Qualifier q = Qualifier::Unlimited) { return unit_[size_t(q)]; }
    const Def* unit(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return unit(cq->box().get_qualifier());
        return arity(1, q);
    }
    const Def* unit_kind(Qualifier q = Qualifier::Unlimited) { return variadic(arity(0), star(q)); }
    const Def* unit_kind(const Def* q) { return variadic(arity(0), star(q)); }
    //@}

    //@{ create Sigma
    /// @em structural Sigma types or kinds
    const Def* sigma(Defs defs, Debug dbg = {}) { return sigma(nullptr, defs, dbg); }
    const Def* sigma(const Def* qualifier, Defs, Debug dbg = {});
    /// Nominal sigma types or kinds
    Sigma* sigma(const Def* type, size_t num_ops, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, type, num_ops, dbg);
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
    const Def* variadic(size_t a, const Def* body, Debug dbg = {}) {
        return variadic(arity(a, Qualifier::Unlimited, dbg), body, dbg);
    }
    const Def* variadic(ArrayRef<size_t> a, const Def* body, Debug dbg = {}) {
        return variadic(DefArray(a.size(), [&](auto i) {
                    return this->arity(a[i], Qualifier::Unlimited, dbg);
                }), body, dbg);
    }
    //@}

    //@{ create Tuple
    const Def* tuple(Defs defs, Debug dbg = {});
    //@}

    //@{ create unit values
    const Def* val_unit(Qualifier q = Qualifier::Unlimited) { return unit_val_[size_t(q)]; }
    const Def* val_unit(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return val_unit(cq->box().get_qualifier());
        return index_zero(arity(1, q));
    }
    const Def* val_unit_kind(Qualifier q = Qualifier::Unlimited) { return pack(arity(0), unit(q)); }
    const Def* val_unit_kind(const Def* q) { return pack(arity(0), unit(q)); }
    //@}

    //@{ create Pack
    const Def* pack(const Def* arities, const Def* body, Debug dbg = {});
    const Def* pack(Defs arities, const Def* body, Debug dbg = {});
    const Def* pack(size_t a, const Def* body, Debug dbg = {}) {
        return pack(arity(a, Qualifier::Unlimited, dbg), body, dbg);
    }
    const Def* pack(ArrayRef<size_t> a, const Def* body, Debug dbg = {}) {
        return pack(DefArray(a.size(), [&](auto i) {
                    return this->arity(a[i], Qualifier::Unlimited, dbg);
                }), body, dbg);
    }
    //@}

    //@{ misc factory methods
    const Def* any(const Def* type, const Def* def, Debug dbg = {});
    const Axiom* arity(size_t a, Qualifier q = Qualifier::Unlimited, Location location = {}) {
        return arity(a, qualifier(q), location);
    }
    const Axiom* arity(size_t a, const Def* q, Location location = {});
    const Def* arity_succ(const Def* arity, Debug dbg = {});
    /// @em nominal Axiom
    const Axiom* axiom(const Def* type, Debug dbg = {}) { return insert<Axiom>(0, *this, type, dbg); }
    /// @em structural Axiom
    const Axiom* assume(const Def* type, Box box, Debug dbg = {}) {
        return unify<Axiom>(0, *this, type, box, dbg);
    }
    const Def* extract(const Def* def, const Def* index, Debug dbg = {});
    const Def* extract(const Def* def, int index, Debug dbg = {}) {
        return extract(def, size_t(index), dbg);
    }
    const Def* extract(const Def* def, size_t index, Debug dbg = {});
    const Def* index(size_t arity, size_t index, Location location = {});
    const Def* index_zero(const Def* arity, Location location = {});
    const Def* index_succ(const Def* index, Debug dbg = {});
    const Def* insert(const Def* def, const Def* index, const Def* value, Debug dbg = {});
    const Def* insert(const Def* def, int index, const Def* value, Debug dbg = {}) {
        return insert(def, size_t(index), value, dbg);
    }
    const Def* insert(const Def* def, size_t index, const Def* value, Debug dbg = {});
    const Def* intersection(Defs defs, Debug dbg = {});
    const Def* intersection(const Def* type, Defs defs, Debug dbg = {});
    const Error* error(const Def* type) { return unify<Error>(0, *this, type); }
    const Def* match(const Def* def, Defs handlers, Debug dbg = {});
    const Def* pick(const Def* type, const Def* def, Debug dbg = {});
    const Def* singleton(const Def* def, Debug dbg = {});
    const Var* var(Defs types, size_t index, Debug dbg = {}) { return var(sigma(types), index, dbg); }
    const Var* var(const Def* type, size_t index, Debug dbg = {}) {
        return unify<Var>(0, *this, type, index, dbg);
    }
    const Def* variant(Defs defs, Debug dbg = {});
    const Def* variant(const Def* type, Defs defs, Debug dbg = {});
    Variant* variant(const Def* type, size_t num_ops, Debug dbg = {}) {
        assert(num_ops > 1 && "It should not be necessary to build empty/unary variants.");
        return insert<Variant>(num_ops, *this, type, num_ops, dbg);
    }
    //@}

    const DefSet& defs() const { return defs_; }

    friend void swap(WorldBase& w1, WorldBase& w2) {
        using std::swap;
        swap(w1.defs_, w2.defs_);
        w1.fix();
        w2.fix();
    }

private:
    template<bool glb, class I>
    const Def* bound(Range<I> ops, const Def* q, bool require_qualifier = true);
    template<class I>
    const Def* lub(Range<I> ops, const Def* q, bool require_qualifier = true) { return bound<false>(ops, q, require_qualifier); }
    const Def* lub(Defs ops, const Def* q, bool require_qualifier = true) { return lub(range(ops), q, require_qualifier); }
    template<class I>
    const Def* glb(Range<I> ops, const Def* q, bool require_qualifier = true) { return bound<true >(ops, q, require_qualifier); }
    const Def* glb(Defs ops, const Def* q, bool require_qualifier = true) { return glb(range(ops), q, require_qualifier); }

    template<bool glb, class I>
    const Def* qualifier_bound(Range<I> defs, std::function<const Def*(const SortedDefSet&)> f);
    template<class I>
    const Def* qualifier_lub(Range<I> defs, std::function<const Def*(const SortedDefSet&)> f) {
        return qualifier_bound<false>(defs, f);
    }
    const Def* qualifier_lub(Defs defs, std::function<const Def*(const SortedDefSet&)> f) {
        return qualifier_lub(range(defs), f);
    }
    template<class I>
    const Def* qualifier_glb(Range<I> defs, std::function<const Def*(const SortedDefSet&)> f) {
        return qualifier_bound<true >(defs, f);
    }
    const Def* qualifier_glb(Defs defs, std::function<const Def*(const SortedDefSet&)> f) {
        return qualifier_glb(range(defs), f);
    }

    void fix() { for (auto def : defs_) def->world_ = this; }

protected:
    template<class T, class... Args>
    const T* unify(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        assert(!def->is_nominal());
        auto p = defs_.emplace(def);
        if (p.second) {
            def->finalize();
            return def;
        }

        --Def::gid_counter_;
        dealloc(def);
        return static_cast<const T*>(*p.first);
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
        static_assert(sizeof(Def) == sizeof(T), "you are not allowed to introduce any additional data in subclasses of Def");
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
    const Universe* universe_;
    const Axiom* qualifier_kind_;
    const Axiom* qualifier_type_;
    const Axiom* arity_succ_;
    const Axiom* index_succ_;
    std::array<const Axiom*, 4> qualifier_;
    std::array<const Star*,  4> star_;
    std::array<const Def*, 4> unit_;
    std::array<const Def*, 4> unit_val_;
    std::array<const Def*, 4> unit_kind_;
    std::array<const Def*, 4> unit_kind_val_;
    std::array<const ArityKind*, 4> arity_kind_;
    std::array<const MultiArityKind*, 4> multi_arity_kind_;
};

inline const Def* app_arg(const Def* def) { return def->as<App>()->arg(); }
inline const Def* app_arg(const Def* def, size_t i) { return def->world().extract(app_arg(def), i); }

class World : public WorldBase {
public:
    template<class T, size_t N> using array = std::array<T, N>;

    World();

    //@{ types and type constructors
    const Def* type_bool() { return type_bool_; }
    const Def* type_nat() { return type_nat_; }

#define CODE(ir)                                                                                                         \
    const Axiom* type_ ## ir() { return type_ ## ir ## _; }                                                              \
    const App* type_ ## ir(ir ## flags flags, int64_t width) { return type_ ## ir(Qualifier::Unlimited, flags, width); } \
    const App* type_ ## ir(Qualifier q, ir ## flags flags, int64_t width) {                                              \
        auto f = val_nat(int64_t(flags)); auto w = val_nat(width); return type_ ## ir(qualifier(q), f, w);               \
    }                                                                                                                    \
    const App* type_ ## ir(const Def* q, const Def* flags, const Def* width, Debug dbg = {}) {                           \
        return app(type_ ## ir(), {q, flags, width}, dbg)->as<App>();                                                    \
    }
    CODE(i)
    CODE(r)
#undef CODE

    const Axiom* type_mem() { return type_mem_; }
    const Axiom* type_frame() { return type_frame_; }
    const Axiom* type_ptr() { return type_ptr_; }
    const Def* type_ptr(const Def* pointee, Debug dbg = {}) { return type_ptr(pointee, val_nat_0(), dbg); }
    const Def* type_ptr(const Def* pointee, const Def* addr_space, Debug dbg = {}) {
        return app(type_ptr_, {pointee, addr_space}, dbg);
    }
    //@}

    //@{ values
    const Axiom* val_nat(int64_t val, Location location = {});
    const Axiom* val_nat_0() { return val_nat_0_; }
    const Axiom* val_nat_1() { return val_nat_[0]; }
    const Axiom* val_nat_2() { return val_nat_[1]; }
    const Axiom* val_nat_4() { return val_nat_[2]; }
    const Axiom* val_nat_8() { return val_nat_[3]; }
    const Axiom* val_nat_16() { return val_nat_[4]; }
    const Axiom* val_nat_32() { return val_nat_[5]; }
    const Axiom* val_nat_64() { return val_nat_[6]; }

    const Axiom* val_bool(bool val) { return val_bool_[size_t(val)]; }
    const Axiom* val_bool_bot() { return val_bool_[0]; }
    const Axiom* val_bool_top() { return val_bool_[1]; }

    const Axiom* val(iflags f,  uint8_t val) { return assume(type_i(f,  8), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, uint16_t val) { return assume(type_i(f, 16), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, uint32_t val) { return assume(type_i(f, 32), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, uint64_t val) { return assume(type_i(f, 64), {val}, {std::to_string(val)}); }

    const Axiom* val(iflags f,  int8_t val) { return assume(type_i(f,  8), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, int16_t val) { return assume(type_i(f, 16), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, int32_t val) { return assume(type_i(f, 32), {val}, {std::to_string(val)}); }
    const Axiom* val(iflags f, int64_t val) { return assume(type_i(f, 64), {val}, {std::to_string(val)}); }

    const Axiom* val(rflags f, half   val) { return assume(type_r(f, 16), {val}, {std::to_string(val)}); }
    const Axiom* val(rflags f, float  val) { return assume(type_r(f, 32), {val}, {std::to_string(val)}); }
    const Axiom* val(rflags f, double val) { return assume(type_r(f, 64), {val}, {std::to_string(val)}); }
    //@}

    //@{ arithmetic operations
    template<iarithop O> const Axiom* op() { return iarithop_[O]; }
    template<rarithop O> const Axiom* op() { return rarithop_[O]; }
    template<iarithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {});
    template<rarithop O> const Def* op(const Def* a, const Def* b, Debug dbg = {});
    //@}

    //@{ relational operations
    const Axiom* op_icmp() { return op_icmp_; }
    const Axiom* op_rcmp() { return op_rcmp_; }
    const Def* op_icmp(irel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_icmp(val_nat(int64_t(rel)), a, b, dbg); }
    const Def* op_rcmp(rrel rel, const Def* a, const Def* b, Debug dbg = {}) { return op_rcmp(val_nat(int64_t(rel)), a, b, dbg); }
    const Def* op_icmp(const Def* rel, const Def* a, const Def* b, Debug dbg = {});
    const Def* op_rcmp(const Def* rel, const Def* a, const Def* b, Debug dbg = {});
    //@}

    //@{ tuple operations
    const Axiom* op_lea() { return op_lea_; }
    const Def* op_lea(const Def* ptr, const Def* index, Debug dbg = {});
    const Def* op_lea(const Def* ptr, size_t index, Debug dbg = {});
    //@}

    //@{ memory operations
    const Def* op_alloc(const Def* type, const Def* mem, Debug dbg = {});
    const Def* op_alloc(const Def* type, const Def* mem, const Def* extra, Debug dbg = {});
    const Def* op_enter(const Def* mem, Debug dbg = {});
    const Def* op_global(const Def* init, Debug dbg = {});
    const Def* op_global_const(const Def* init, Debug dbg = {});
    const Def* op_load(const Def* mem, const Def* ptr, Debug dbg = {});
    const Def* op_slot(const Def* type, const Def* frame, Debug dbg = {});
    const Def* op_store(const Def* mem, const Def* ptr, const Def* val, Debug dbg = {});
    //@}

private:
    const Def* type_bool_;
    const Def* type_nat_;
    const Axiom* val_nat_0_;
    array<const Axiom*, 2> val_bool_;
    array<const Axiom*, 7> val_nat_;
    const Axiom* type_i_;
    const Axiom* type_r_;
    const Axiom* type_mem_;
    const Axiom* type_frame_;
    const Axiom* type_ptr_;
    const Axiom* op_enter_;
    const Axiom* op_lea_;
    const Axiom* op_load_;
    const Axiom* op_slot_;
    const Axiom* op_store_;
    array<const Axiom*, Num_IArithOp> iarithop_;
    array<const Axiom*, Num_RArithOp> rarithop_;
    const Axiom* op_icmp_;
    const Axiom* op_rcmp_;
};

}

#endif
