#include "thorin/print.h"

#include <stack>

#include "thorin/def.h"

namespace thorin {

//------------------------------------------------------------------------------

static bool descend(const Def* def) {
    return def->num_ops() != 0 && get_axiom(def) == nullptr;
}

void Printer::print(const Def* def) {
    std::stack<const Def*> stack;
    DefSet done;

    auto push = [&](const Def* def) {
        if (descend(def) && done.emplace(def).second) {
            stack.emplace(def);
            return true;
        }
        return false;
    };

    stack.emplace(def);

    while (!stack.empty()) {
        auto def = stack.top();

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
            stack.pop();
        }
    }
}

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

Printer& Def::stream_assign(Printer& p) const {
    return streamf(p, "{} = {};", unique_name(), this).endl();
}

void Def::dump_assign() const {
    Printer printer;
    stream_assign(printer);
}

void Def::dump_rec() const {
    Printer printer;
    printer.print(this);
}

//------------------------------------------------------------------------------

Printer& App::vstream(Printer& p) const {
    if (descend(callee()))
        streamf(p, "{}", callee()->unique_name());
    else
        streamf(p, "{}", callee());

    if (arg()->isa<Tuple>() || arg()->isa<Pack>() || !descend(arg()))
        streamf(p, " {}", arg());
    else
        streamf(p, " {}", arg()->unique_name()); // descend

    return p;
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

Printer& Unknown::vstream(Printer& p) const {
    return streamf(p, "<?_{}>", gid());
}

Printer& Tuple::vstream(Printer& p) const {
    return streamf(p, "({, })", stream_list(ops(), [&](const Def* def) { descend(def) ? streamf(p, "{}", def->unique_name()) : def->name_stream(p); }));
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
