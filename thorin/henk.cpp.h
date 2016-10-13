#ifndef HENK_TABLE_NAME
#error "please define the type table name HENK_TABLE_NAME"
#endif

#ifndef HENK_TABLE_TYPE
#error "please define the type table type HENK_TABLE_TYPE"
#endif

size_t Def::gid_counter_ = 1;

//------------------------------------------------------------------------------

const Def* Def::op(size_t i) const { return i < num_ops() ? ops()[i] : HENK_TABLE_NAME().error(); }

void Def::set(Defs defs) {
    assert(defs.size() == num_ops());
    for (size_t i = 0, e = defs.size(); i != e; ++i)
        set(i, defs[i]);
}

void Def::set(size_t i, const Def* def) {
    assert(!op(i) && "already set");
    assert(def && "setting null pointer");
    ops_[i] = def;
    assert(!def->uses_.contains(Use(i, this)));
    const auto& p = def->uses_.emplace(i, this);
    assert_unused(p.second);

    order_        = std::max(order_, def->order());
    monomorphic_ &= def->is_monomorphic();
    known_       &= def->is_known();
}

void Def::unregister_uses() const {
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        unregister_use(i);
}

void Def::unregister_use(size_t i) const {
    auto def = ops_[i];
    assert(def->uses_.contains(Use(i, this)));
    def->uses_.erase(Use(i, this));
    assert(!def->uses_.contains(Use(i, this)));
}

void Def::unset(size_t i) {
    assert(ops_[i] && "must be set");
    unregister_use(i);
    ops_[i] = nullptr;
}

std::string Def::unique_name() const {
    std::ostringstream oss;
    oss << name() << '_' << gid();
    return oss.str();
}

/*
 * constructors
 */

Lambda::Lambda(HENK_TABLE_TYPE& table, const Def* domain, const Def* body, const Location& loc, const std::string& name)
    : Connective(table, Node_Lambda, table.pi(domain, body, loc, name), {domain, body}, loc, name)
{}

Pi::Pi(HENK_TABLE_TYPE& table, const Def* domain, const Def* body, const Location& loc, const std::string& name)
    : Quantifier(table, Node_Pi, body->type(), {domain, body}, loc, name)
{}

Tuple::Tuple(HENK_TABLE_TYPE& table, Defs ops, const Location& loc, const std::string& name)
    : Connective(table, Node_Tuple, nullptr, ops, loc, name)
{
    Array<const Def*> types(ops.size());
    for (size_t i = 0, e = ops.size(); i != e; ++i)
        types[i] = ops[i]->type();
    set_type(table.sigma(types, loc, name));
}

//------------------------------------------------------------------------------

/*
 * hash
 */

uint64_t Def::vhash() const {
    if (is_nominal())
        return gid();

    uint64_t seed = thorin::hash_combine(thorin::hash_begin(int(tag())), num_ops(), type() ? type()->gid() : 0);
    for (auto op : ops_)
        seed = thorin::hash_combine(seed, op->hash());
    return seed;
}

uint64_t Var::vhash() const {
    return thorin::hash_combine(thorin::hash_begin(int(tag())), depth());
}

//------------------------------------------------------------------------------

/*
 * equal
 */

bool Def::equal(const Def* other) const {
    if (is_nominal())
        return this == other;

    bool result = this->tag() == other->tag() && this->num_ops() == other->num_ops();

    if (result) {
        for (size_t i = 0, e = num_ops(); result && i != e; ++i)
            result &= this->op(i) == other->op(i);
    }

    return result;
}

bool Var::equal(const Def* other) const {
    return other->isa<Var>() ? this->as<Var>()->depth() == other->as<Var>()->depth() : false;
}

//------------------------------------------------------------------------------

/*
 * rebuild
 */

const Def* Def::rebuild(HENK_TABLE_TYPE& to, Defs ops) const {
    assert(num_ops() == ops.size());
    if (ops.empty() && &HENK_TABLE_NAME() == &to)
        return this;
    return vrebuild(to, ops);
}

const Def* Sigma::vrebuild(HENK_TABLE_TYPE& to, Defs ops) const {
    if (is_nominal()) {
        auto sigma = to.sigma(ops.size(), loc(), name());
        for (size_t i = 0, e = ops.size(); i != e; ++i)
            sigma->set(i, ops[i]);
        return sigma;
    } else
        return to.sigma(ops, loc(), name());
}

const Def* App   ::vrebuild(HENK_TABLE_TYPE& to, Defs ops) const { return to.app(ops[0], ops[1], loc(), name()); }
const Def* Tuple ::vrebuild(HENK_TABLE_TYPE& to, Defs ops) const { return to.tuple(ops, loc(), name()); }
const Def* Lambda::vrebuild(HENK_TABLE_TYPE& to, Defs ops) const { return to.lambda(ops[0], ops[1], loc(), name()); }
const Def* Var   ::vrebuild(HENK_TABLE_TYPE& to, Defs ops) const { return to.var(ops[0], depth(), loc(), name()); }
const Def* Error ::vrebuild(HENK_TABLE_TYPE& to, Defs ops) const { return to.error(ops[1]); }

//------------------------------------------------------------------------------

/*
 * reduce
 */

const Def* Def::reduce(int depth, const Def* def, Def2Def& map) const {
    if (auto result = find(map, this))
        return result;
    if (is_monomorphic())
        return this;
    return map[this] = vreduce(depth, def, map);
}

Array<const Def*> Def::reduce_ops(int depth, const Def* def, Def2Def& map) const {
    Array<const Def*> result(num_ops());
    for (size_t i = 0, e = num_ops(); i != e; ++i)
        result[i] = op(i)->reduce(depth, def, map);
    return result;
}

const Def* Lambda::vreduce(int depth, const Def* def, Def2Def& map) const {
    return HENK_TABLE_NAME().lambda(domain(), body()->reduce(depth+1, def, map), loc(), name());
}

const Def* Pi::vreduce(int depth, const Def* def, Def2Def& map) const {
    return HENK_TABLE_NAME().pi(domain(), body()->reduce(depth+1, def, map), loc(), name());
}

const Def* Tuple::vreduce(int depth, const Def* def, Def2Def& map) const {
    return HENK_TABLE_NAME().tuple(reduce_ops(depth, def, map), loc(), name());
}

const Def* Sigma::vreduce(int depth, const Def* def, Def2Def& map) const {
    if (is_nominal()) {
        auto sigma = HENK_TABLE_NAME().sigma(num_ops(), loc(), name());
        map[this] = sigma;

        for (size_t i = 0, e = num_ops(); i != e; ++i)
            sigma->set(i, op(i)->reduce(depth+i, def, map));

        return sigma;
    }  else {
        Array<const Def*> ops(num_ops());
        for (size_t i = 0, e = num_ops(); i != e; ++i)
            ops[i] = op(i)->reduce(depth+i, def, map);
        return map[this] = HENK_TABLE_NAME().sigma(ops, loc(), name());
    }
}

const Def* Var::vreduce(int depth, const Def* def, Def2Def&) const {
    if (this->depth() == depth)
        return def;
    else if (this->depth() > depth)
        return HENK_TABLE_NAME().var(type(), this->depth()-1, loc(), name());   // this is a free variable - shift by one
    else
        return this;                                                            // this variable is not free - don't adjust
}

const Def* App::vreduce(int depth, const Def* def, Def2Def& map) const {
    auto ops = reduce_ops(depth, def, map);
    return HENK_TABLE_NAME().app(ops[0], ops[1], loc(), name());
}

//const Def* TypeError::vreduce(int, const Def*, Def2Def&) const { return this; }

//------------------------------------------------------------------------------

template<class T>
const Def* TableBase<T>::app(const Def* callee, const Def* arg, const Location& loc, const std::string& name) {
    if (auto sigma = arg->type()->isa<Sigma>()) {
        Array<const Def*> args;
        for (size_t i = 0, e = sigma->num_ops(); i != e; ++i)
            args[i] = app(arg, arg, loc, name);
        return app(callee, args, loc, name);
    }
    return app(callee, {arg}, loc, name);
}

template<class T>
const Def* TableBase<T>::app(const Def* callee, Defs args, const Location& loc, const std::string& name) {
    if (args.size() == 1 && args.front()->type()->isa<Sigma>())
        return app(callee, args.front(), loc, name);

    auto app = unify(new App(HENK_TABLE_NAME(), callee, args, loc, name));

    if (auto cache = app->cache_)
        return cache;
    if (auto lambda = app->callee()->template isa<Lambda>()) {
        Def2Def map;
        return app->cache_ = lambda->body()->reduce(1, args.front(), map); // TODO reduce args
    } else
        return app->cache_ = app;

    return app;
}

template<class T>
const Def* TableBase<T>::unify_base(const Def* def) {
    assert(!def->is_nominal());
    auto p = defs_.emplace(def);
    if (p.second)
        return def;

    --Def::gid_counter_;
    delete def;
    return *p.first;
}

template class TableBase<HENK_TABLE_TYPE>;

//------------------------------------------------------------------------------

#undef HENK_TABLE_NAME
#undef HENK_TABLE_TYPE
