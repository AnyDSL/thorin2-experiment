#include "thorin/print.h"

#include "thorin/def.h"

namespace thorin {

//------------------------------------------------------------------------------

static bool descend(const Def* def) {
    return def->num_ops() != 0 && get_axiom(def) == nullptr;
}

inline stream_list<Defs, std::function<void(const Def*)>> DefPrinter::list(Defs defs) {
    auto f = std::function<void(const Def*)>([&](const Def* def) { return *this << this->str(def); });
    return stream_list(defs, f);
}

#if 0
DefPrinter& Def::name_stream(DefPrinter& os) const {
    if (name() != "" || is_nominal()) {
        qualifier_stream(os);
        return os << name();
    }
    return stream(os);
}
#endif

std::string DefPrinter::str(const Def* def) const {
    if (descend(def))
        return def->unique_name();

    std::ostringstream os;
    DefPrinter printer(os, tab());
    def->stream(printer);
    return os.str();
#if 0
    if (is_nominal()) {
        qualifier_stream(os);
        return os << name();
    }
    return vstream(os);
#endif
}


void DefPrinter::_push(const Def* def) {
    if (auto lambda = def->isa<Lambda>())
        lambdas_.emplace(lambda);
    else
        stack_.emplace(def);
    done_.emplace(def);
}

bool DefPrinter::push(const Def* def) {
    if (descend(def) && done_.emplace(def).second) {
        _push(def);
        return true;
    }
    return false;
}

DefPrinter& DefPrinter::recurse(const Lambda* lambda) {
    _push(lambda);

    while (!lambdas_.empty()) {
        auto lambda = pop(lambdas_);

        if (lambda->codomain()->isa<Bottom>())
            streamf(*this, "{} := cn {} {{", lambda->unique_name(), str(lambda->domain())).indent();
        else
            streamf(*this, "{} := λ{} -> {} {{", lambda->unique_name(), str(lambda->domain()), str(lambda->codomain())).indent();

        recurse(lambda->body());
        (dedent().endl() << "}").endl().endl();
    }

    return *this;
}

DefPrinter& DefPrinter::recurse(const Def* def) {
    if (auto lambda = def->isa<Lambda>())
        return recurse(lambda);

    _push(def);

    while (!stack_.empty()) {
        auto def = stack_.top();

        bool todo = false;
        if (auto app = def->isa<App>()) {
            todo |= push(app->callee());

            if (app->arg()->isa<Tuple>() || app->arg()->isa<Pack>()) {
                for (auto op : app->arg()->ops())
                    todo |= push(op);
            } else {
                todo |= push(app->arg());
            }
        } else {
            for (auto op : def->ops())
                todo |= push(op);
        }

        if (!todo) {
            def->stream_assign(endl()) << ';';
            stack_.pop();
        }
    }

    return *this;
}

//------------------------------------------------------------------------------

DefPrinter& Def::qualifier_stream(DefPrinter& p) const {
    if (!has_values() || tag() == Tag::QualifierType)
        return p;
    if (type()->is_kind()) {
        auto q = type()->op(0);
        if (auto qual = get_qualifier(q)) {
            if (qual != Qualifier::u)
                q->stream(p);
        } else
            q->stream(p);
    }
    return p;
}

DefPrinter& Def::stream_assign(DefPrinter& p) const {
    return stream(streamf(p, "{}: {} = ", unique_name(), type()));
}

void Def::dump_assign() const {
    DefPrinter printer;
    stream_assign(printer).endl();
}

void Def::dump_rec() const {
    DefPrinter p;
    p.recurse(this).endl();
}

//------------------------------------------------------------------------------

DefPrinter& App::stream(DefPrinter& p) const {
    streamf(p, "{}", p.str(callee()));

    if (arg()->isa<Tuple>() || arg()->isa<Pack>())
        return arg()->stream(p << ' ');
    return streamf(p, " {}", p.str(arg()));

#if 0
    auto domain = callee_type();
    if (domain->is_kind()) {
        qualifier_stream(p);
    }
    callee()->name_stream(p);
    if (!arg()->isa<Tuple>() && !arg()->isa<Pack>())
        p << "(";
    arg()->name_stream(p);
    if (!arg()->isa<Tuple>() && !arg()->isa<Pack>())
        p << ")";
#endif
}

DefPrinter& Arity::stream(DefPrinter& p) const {
    return p << name();
}

DefPrinter& Axiom::stream(DefPrinter& p) const {
    return qualifier_stream(p) << name();
}

DefPrinter& Bottom::stream(DefPrinter& p) const { return streamf(p, "{{⊥: {}}}", p.str(type())); }
DefPrinter& Top   ::stream(DefPrinter& p) const { return streamf(p, "{{⊤: {}}}", p.str(type())); }

DefPrinter& Extract::stream(DefPrinter& p) const {
    return streamf(p, "{}#{}", p.str(scrutinee()), p.str(index()));
#if 0
    return scrutinee()->name_stream(p) << "#" << index();
#endif
}

DefPrinter& Insert::stream(DefPrinter& p) const {
    return streamf(p, "{}#{} <- ", p.str(scrutinee()), p.str(index()), p.str(value()));
#if 0
    return scrutinee()->name_stream(p) << "." << index() << "=" << value();
#endif
}

DefPrinter& Intersection::stream(DefPrinter& p) const {
    return streamf(qualifier_stream(p), "({ ∩ })", p.list(ops()));
}

DefPrinter& Kind::stream(DefPrinter& p) const {
    p << name();
    if (auto q = get_qualifier(op(0)))
        if (q == Qualifier::u)
            return p;
    return p << op(0);
}

DefPrinter& Lit::stream(DefPrinter& p) const {
    qualifier_stream(p);
    if (!name().empty())
        return p << name();
    return p << std::to_string(box().get_u64());
}

DefPrinter& Match::stream(DefPrinter& p) const {
    return streamf(p,"match {} with ({, })", p.str(destructee()), p.list(handlers()));
}

DefPrinter& Param::stream(DefPrinter& p) const {
    return streamf(p, "param {}", p.str(lambda()));
}

DefPrinter& Lambda::stream(DefPrinter& p) const {
    if (codomain()->isa<Bottom>())
        return streamf(p, "{} := cn {} {{ {} }}", unique_name(), p.str(domain()), body());
    return streamf(p, "{} := λ{} -> {} {{ {} }}", unique_name(), p.str(domain()), p.str(codomain()), body());
#if 0
    streamf("λ{} -> {}", domain(), codomain());
    p.indent().endl();
    streamf("{}");
    domain()->name_stream(p << "λ");
    return body()->name_stream(p << ".");
#endif
}

DefPrinter& Pack::stream(DefPrinter& p) const {
    p << "‹";
    if (auto var = type()->isa<Variadic>())
        p << var->op(0);
    else {
        assert(type()->isa<Sigma>());
        p << type()->num_ops() << "ₐ";
    }
    return streamf(p, "; {}›", p.str(body()));
}

DefPrinter& Pi::stream(DefPrinter& p) const {
#if 0
    qualifier_stream(p);
    p  << "Π";
    domain()->name_stream(p);
    return codomain()->name_stream(p << ".");
#endif
    if (codomain()->isa<Bottom>())
        return streamf(p, "Cn {}", p.str(domain()));
    return streamf(p, "Π{} -> {}", p.str(domain()), p.str(codomain()));
}

DefPrinter& Pick::stream(DefPrinter& p) const {
#if 0
    p << "pick:";
    type()->name_stream(p);
    destructee()->name_stream(p << "(");
    return p << ")";
#endif
    return p << "TODO";
}

DefPrinter& QualifierType::stream(DefPrinter& p) const {
    return p << name();
}

DefPrinter& Sigma::stream(DefPrinter& p) const {
    return streamf(qualifier_stream(p), "[{, }]", p.list((ops())));
}

DefPrinter& Singleton::stream(DefPrinter& p) const {
    return streamf(p, "S({, })", p.list(ops()));
}

DefPrinter& Universe::stream(DefPrinter& p) const {
    return p << name();
}

DefPrinter& Unknown::stream(DefPrinter& p) const {
    return streamf(p, "<?_{}>", gid());
}

DefPrinter& Tuple::stream(DefPrinter& p) const {
    return streamf(p, "({, })", p.list(ops()));
}

DefPrinter& Var::stream(DefPrinter& p) const {
#if 0
    p << "\\\\" << index() << "::";
    return type()->name_stream(p);
#endif
    return streamf(p, "\\\\{}::{}", index(), p.str(type()));
}

DefPrinter& Variadic::stream(DefPrinter& p) const {
    return streamf(p, "«{}; {}»", p.str(arity()), p.str(body()));
}

DefPrinter& Variant::stream(DefPrinter& p) const {
    return streamf(qualifier_stream(p), "({ ∪ })", p.list(ops()));
}

}
