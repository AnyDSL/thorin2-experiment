#include "thorin/print.h"

#include "thorin/def.h"

namespace thorin {

//------------------------------------------------------------------------------

static bool descend(const Def* def) {
    return def->num_ops() != 0 && get_axiom(def) == nullptr;
}

bool DefPrinter::push(const Def* def) {
    if (descend(def) && done_.emplace(def).second) {
        stack_.emplace(def);
        return true;
    }
    return false;
}

DefPrinter& DefPrinter::recurse(const Def* def) {
    stack_.emplace(def);

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
            def->stream_assign(*this);
            stack_.pop();
        }
    }

    return *this;
}

//------------------------------------------------------------------------------

#if 0
DefPrinter& Def::name_stream(DefPrinter& os) const {
    if (name() != "" || is_nominal()) {
        qualifier_stream(os);
        return os << name();
    }
    return stream(os);
}
#endif

DefPrinter& Def::stream(DefPrinter& p) const {
    if (descend(this))
        return p << unique_name();
    return vstream(p);
#if 0
    if (is_nominal()) {
        qualifier_stream(os);
        return os << name();
    }
    return vstream(os);
#endif
}

DefPrinter& Def::qualifier_stream(DefPrinter& p) const {
    if (!has_values() || tag() == Tag::QualifierType)
        return p;
    if (type()->is_kind()) {
        auto q = type()->op(0);
        if (auto qual = q->isa<Qualifier>()) {
            if (qual->qualifier_tag() != QualifierTag::u)
                q->stream(p);
        } else
            q->stream(p);
    }
    return p;
}

DefPrinter& Def::stream_assign(DefPrinter& p) const {
    return vstream(p << unique_name() << " = ").endl();
}

void Def::dump_assign() const {
    DefPrinter printer;
    stream_assign(printer);
}

void Def::dump_rec() const {
    DefPrinter printer;
    printer.recurse(this);
}

//------------------------------------------------------------------------------

DefPrinter& App::vstream(DefPrinter& p) const {
    if (descend(callee()))
        streamf(p, "{}", callee()->unique_name());
    else
        streamf(p, "{}", callee());

    if (arg()->isa<Tuple>() || arg()->isa<Pack>())
        return arg()->vstream(p << ' ');
    return streamf(p, " {}", arg());

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

DefPrinter& Arity::vstream(DefPrinter& p) const {
    return p << name();
}

DefPrinter& ArityKind::vstream(DefPrinter& p) const {
    p << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return p;
    return p << op(0);
}

DefPrinter& Axiom::vstream(DefPrinter& p) const {
    return qualifier_stream(p) << name();
}

DefPrinter& Bottom::vstream(DefPrinter& p) const { return streamf(p, "{{⊥: {}}}", type()); }
DefPrinter& Top   ::vstream(DefPrinter& p) const { return streamf(p, "{{⊤: {}}}", type()); }

DefPrinter& Extract::vstream(DefPrinter& p) const {
    return streamf(p, "{}#{}", scrutinee(), index());
#if 0
    return scrutinee()->name_stream(p) << "#" << index();
#endif
}

DefPrinter& Insert::vstream(DefPrinter& p) const {
    return streamf(p, "{}#{} <- ", scrutinee(), index(), value());
#if 0
    return scrutinee()->name_stream(p) << "." << index() << "=" << value();
#endif
}

DefPrinter& Intersection::vstream(DefPrinter& p) const {
    //return streamf(qualifier_stream(p), "({ ∩ })", stream_list(ops(), [&](auto def) { def->name_stream(p); }));
    return streamf(qualifier_stream(p), "({ ∩ })", ops());
}

DefPrinter& Lit::vstream(DefPrinter& p) const {
    qualifier_stream(p);
    if (!name().empty())
        return p << name();
    return p << std::to_string(box().get_u64());
}

DefPrinter& Match::vstream(DefPrinter& p) const {
    return streamf(p,"match {} with ({, })", destructee(), handlers());
    //p << "match ";
    //destructee()->name_stream(p);
    //p << " with ";
    //return stream_list(p, handlers(), "(", ")", [&](const Def* def) { def->name_stream(p); });
}

DefPrinter& MultiArityKind::vstream(DefPrinter& p) const {
    p << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return p;
    return p << op(0);
}

DefPrinter& Param::vstream(DefPrinter& p) const {
    return streamf(p, "param {}", lambda());
}

DefPrinter& Lambda::vstream(DefPrinter& p) const {
    streamf(p, "λ{} -> {}", domain(), codomain()).indent().endl();
    return streamf(p, "{}", body()).dedent().endl();
#if 0
    streamf("λ{} -> {}", domain(), codomain());
    p.indent().endl();
    streamf("{}");
    domain()->name_stream(p << "λ");
    return body()->name_stream(p << ".");
#endif
}

DefPrinter& Pack::vstream(DefPrinter& p) const {
    p << "(";
    if (auto var = type()->isa<Variadic>())
        p << var->op(0);
    else {
        assert(type()->isa<Sigma>());
        p << type()->num_ops() << "ₐ";
    }
    return streamf(p, "; {})", body());
}

DefPrinter& Pi::vstream(DefPrinter& p) const {
#if 0
    qualifier_stream(p);
    p  << "Π";
    domain()->name_stream(p);
    return codomain()->name_stream(p << ".");
#endif
    if (codomain()->isa<Bottom>())
        return streamf(p, "Cn {}", domain());
    return streamf(p, "Π{} -> {}", domain(), codomain());
}

DefPrinter& Pick::vstream(DefPrinter& p) const {
#if 0
    p << "pick:";
    type()->name_stream(p);
    destructee()->name_stream(p << "(");
    return p << ")";
#endif
    return p << "TODO";
}

DefPrinter& Qualifier::vstream(DefPrinter& p) const {
    return p << qualifier_tag();
}

DefPrinter& QualifierType::vstream(DefPrinter& p) const {
    return p << name();
}

DefPrinter& Sigma::vstream(DefPrinter& p) const {
    //return streamf(qualifier_stream(p), "[{, }]", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
    return streamf(qualifier_stream(p), "[{, }]", ops());
}

DefPrinter& Singleton::vstream(DefPrinter& p) const {
    //return streamf(p, "S({, })", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
    return streamf(p, "S({, })", ops());
}

DefPrinter& Star::vstream(DefPrinter& p) const {
    p << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return p;
    return p << op(0);
}

DefPrinter& Universe::vstream(DefPrinter& p) const {
    return p << name();
}

DefPrinter& Unknown::vstream(DefPrinter& p) const {
    return streamf(p, "<?_{}>", gid());
}

DefPrinter& Tuple::vstream(DefPrinter& p) const {
    return streamf(p, "({, })", ops());
}

DefPrinter& Var::vstream(DefPrinter& p) const {
#if 0
    p << "\\\\" << index() << "::";
    return type()->name_stream(p);
#endif
    return streamf(p, "\\\\{}::{}", index(), type());
}

DefPrinter& Variadic::vstream(DefPrinter& p) const {
    return streamf(p, "[{}; {}]", op(0), body());
}

DefPrinter& Variant::vstream(DefPrinter& p) const {
    //return streamf(qualifier_stream(p), "({ ∪ })", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
    return streamf(qualifier_stream(p), "({ ∪ })", ops());
}

}
