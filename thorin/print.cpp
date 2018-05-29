#include "thorin/print.h"

#include "thorin/def.h"

namespace thorin {

//------------------------------------------------------------------------------

Printer& Def::name_stream(Printer& os) const {
    if (name() != "" || is_nominal()) {
        qualifier_stream(os);
        return os << name();
    }
    return stream(os);
}

Printer& Def::stream(Printer& os) const {
    if (is_nominal()) {
        qualifier_stream(os);
        return os << name();
    }
    return vstream(os);
}

std::ostream& Def::stream_out(std::ostream& os) const {
    Printer printer(os);
    stream(printer);
    return os;
}

Printer& Def::qualifier_stream(Printer& p) const {
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

//------------------------------------------------------------------------------

Printer& App::vstream(Printer& p) const {
    auto domain = callee()->type()->as<Pi>()->domain();
    if (domain->is_kind()) {
        qualifier_stream(p);
    }
    callee()->name_stream(p);
    if (!arg()->isa<Tuple>() && !arg()->isa<Pack>())
        p << "(";
    arg()->name_stream(p);
    if (!arg()->isa<Tuple>() && !arg()->isa<Pack>())
        p << ")";
    return p;
}

Printer& Arity::vstream(Printer& p) const {
    return p << name();
}

Printer& ArityKind::vstream(Printer& p) const {
    p << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return p;
    return p << op(0);
}

Printer& Axiom::vstream(Printer& p) const {
    return qualifier_stream(p) << name();
}

Printer& Bottom::vstream(Printer& p) const { return streamf(p, "{{⊥: {}}}", type()); }
Printer& Top   ::vstream(Printer& p) const { return streamf(p, "{{⊤: {}}}", type()); }

Printer& Extract::vstream(Printer& p) const {
    return scrutinee()->name_stream(p) << "#" << index();
}

Printer& Insert::vstream(Printer& p) const {
    return scrutinee()->name_stream(p) << "." << index() << "=" << value();
}

Printer& Intersection::vstream(Printer& p) const {
    return streamf(qualifier_stream(p), "({ ∩ })", stream_list(ops(), [&](auto def) { def->name_stream(p); }));
}

Printer& Lit::vstream(Printer& p) const {
    qualifier_stream(p);
    if (!name().empty())
        return p << name();
    return p << std::to_string(box().get_u64());
}

Printer& Match::vstream(Printer& p) const {
    return streamf(p,"match {} with ({, })", destructee(), handlers());
    //p << "match ";
    //destructee()->name_stream(p);
    //p << " with ";
    //return stream_list(p, handlers(), "(", ")", [&](const Def* def) { def->name_stream(p); });
}

Printer& MultiArityKind::vstream(Printer& p) const {
    p << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return p;
    return p << op(0);
}

Printer& Param::vstream(Printer& p) const { return p << unique_name(); }

Printer& Lambda::vstream(Printer& p) const {
    domain()->name_stream(p << "λ");
    return body()->name_stream(p << ".");
}

Printer& Pack::vstream(Printer& p) const {
    p << "(";
    if (auto var = type()->isa<Variadic>())
        p << var->op(0);
    else {
        assert(type()->isa<Sigma>());
        p << type()->num_ops() << "ₐ";
    }
    return streamf(p, "; {})", body());
}

Printer& Pi::vstream(Printer& p) const {
    qualifier_stream(p);
    p  << "Π";
    domain()->name_stream(p);
    return codomain()->name_stream(p << ".");
}

Printer& Pick::vstream(Printer& p) const {
    p << "pick:";
    type()->name_stream(p);
    destructee()->name_stream(p << "(");
    return p << ")";
}

Printer& Qualifier::vstream(Printer& p) const {
    return p << qualifier_tag();
}

Printer& QualifierType::vstream(Printer& p) const {
    return p << name();
}

Printer& Sigma::vstream(Printer& p) const {
    return streamf(qualifier_stream(p), "[{, }]", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
}

Printer& Singleton::vstream(Printer& p) const {
    return streamf(p, "S({, })", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
}

Printer& Star::vstream(Printer& p) const {
    p << name();
    if (auto q = op(0)->isa<Qualifier>())
        if (q->qualifier_tag() == QualifierTag::u)
            return p;
    return p << op(0);
}

Printer& Universe::vstream(Printer& p) const {
    return p << name();
}

Printer& Tuple::vstream(Printer& p) const {
    return streamf(p, "({, })", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
}

Printer& Var::vstream(Printer& p) const {
    p << "\\\\" << index() << "::";
    return type()->name_stream(p);
}

Printer& Variadic::vstream(Printer& p) const {
    return streamf(p, "[{}; {}]", op(0), body());
}

Printer& Variant::vstream(Printer& p) const {
    return streamf(qualifier_stream(p), "({ ∪ })", stream_list(ops(), [&](const Def* def) { def->name_stream(p); }));
}

}
