#include "thorin/fe/parser.h"

#include "thorin/transform/reduce.h"
#include "thorin/util/log.h"

namespace thorin::fe {

// current syntax
// b ::=    x : e       (binder)
//     |    e           (expr)
//
// e ::=    x           (identifier)
//     |    \n          (DeBruijn index)
//     |    Πb.e        (pi type)
//     |    λb.e        (lambda abstraction)
//     |    [b,...]     (sigma type)
//     |    [b; e]      (variadic)
//     |    (e,...)     (tuple)
//     |    (b; e)      (pack)
//     |    e#e         (extract)
//     |    e#e <- e    (insert)

const Def* Parser::parse_def() {
    Tracker tracker(this);
    std::vector<std::pair<const Def*, Loc>> defs;
    parse_curried_defs(defs);
    assert(!defs.empty());
    const Def* def = defs.front().first;
    for (auto [arg, loc] : range(defs.begin()+1, defs.end()))
        def = world_.app(def, arg, loc);
    return def;
}

void Parser::parse_curried_defs(std::vector<std::pair<const Def*, Loc>>& defs) {
    Tracker tracker(this);
    const Def* def = nullptr;

    if (false) {}
    else if (accept(TT::Bool))              def = world_.type_bool();
    else if (ahead().isa(TT::Cn))           def = parse_cn();
    else if (ahead().isa(TT::Pi))           def = parse_pi();
    else if (ahead().isa(TT::D_bracket_l))  def = parse_sigma();
    else if (ahead().isa(TT::D_quote_l))    def = parse_variadic();
    else if (ahead().isa(TT::Lambda))       def = parse_lambda();
    else if (ahead().isa(TT::Star)
            || ahead().isa(TT::Arity_Kind)
            || ahead().isa(TT::Multi_Arity_Kind)) def = parse_qualified_kind();
    else if (ahead().isa(TT::Backslash))    def = parse_debruijn();
    else if (ahead().isa(TT::Identifier))   def = parse_identifier();
    else if (ahead().isa(TT::D_paren_l))    def = parse_tuple();
    else if (ahead().isa(TT::D_angle_l))    def = parse_pack();
    else if (ahead().isa(TT::D_brace_l))    def = parse_variant();
    else if (ahead().isa(TT::Literal))      def = parse_literal();
    else if (accept(TT::Qualifier_Type))    def = world_.qualifier_type();
    else if (accept(TT::Q_u))               def = world_.qualifier_u();
    else if (accept(TT::Q_r))               def = world_.qualifier_r();
    else if (accept(TT::Q_a))               def = world_.qualifier_a();
    else if (accept(TT::Q_l))               def = world_.qualifier_l();
    else if (accept(TT::Multi_Arity_Kind))  def = world_.multi_arity_kind();

    if (def == nullptr)
        error("expected some expression");

    if (accept(TT::Sharp))
        def = parse_extract_or_insert(tracker, def);

    defs.emplace_back(def, tracker.loc());

    // an expression follows that can be an argument for an app
    if (ahead().is_app_arg())
        parse_curried_defs(defs);
}

const Def* Parser::parse_debruijn() {
    Tracker tracker(this);
    // De Bruijn index
    eat(TT::Backslash);
    if (ahead().isa(TT::Literal)) {
        auto lit = ahead().literal();
        eat(TT::Literal);
        if (lit.tag == Literal::Tag::Lit_untyped) {
            auto index = lit.box.get_u64();
            if (accept(TT::ColonColon)) {
                const Def* type = nullptr;
                switch (ahead().tag()) {
                    case TT::Backslash:      type = parse_debruijn(); break;
                    case TT::Identifier:     type = parse_identifier(); break;
                    case TT::Pi:             type = parse_pi(); break;
                    case TT::D_bracket_l:    type = parse_sigma(); break;
                    case TT::D_quote_l:      type = parse_variadic(); break;
                    case TT::Qualifier_Type: type = world_.qualifier_type(); break;
                    case TT::Star:
                    case TT::Arity_Kind:
                    case TT::Multi_Arity_Kind: type = parse_qualified_kind(); break;
                    case TT::Literal:
                        if (ahead().literal().tag == Literal::Tag::Lit_arity) {
                            type = parse_literal();
                            break;
                        }
                        [[fallthrough]];
                    default: assertf(false, "expected type for De Bruijn variable after '::'");
                }
                return world_.var(type, index, tracker.loc());
            }
            return world_.var(debruijn_types_[debruijn_types_.size() - index - 1], index, tracker.loc());
        } else {
            assertf(false, "untyped literal expected after '\'");
            THORIN_UNREACHABLE;
        }
    } else {
        assertf(false, "number expected after '\'");
        THORIN_UNREACHABLE;
    }
}

const Def* Parser::parse_cn() {
    Tracker tracker(this);
    eat(TT::Cn);
    auto domain = parse_def();
    return world_.cn(domain, tracker.loc());
}

const Def* Parser::parse_pi() {
    Tracker tracker(this);
    eat(TT::Pi);
    auto domain = parse_def();
    push_debruijn_type(domain);
    expect(TT::Dot, "Π type");
    auto body = parse_def();
    pop_debruijn_binders();
    return world_.pi(domain, body, tracker.loc());
}

const Def* Parser::parse_sigma() {
    Tracker tracker(this);
    eat(TT::D_bracket_l);

    auto elems = parse_list(TT::D_bracket_r, TT::Comma, "elements of sigma type", [&]{
            auto elem = parse_def();
            push_debruijn_type(elem);
            return elem;
    });
    // shift binders declared within the sigma by generating
    // extracts to the DeBruijn variable \0 (the current binder)
    shift_binders(elems.size());

    return world_.sigma(elems, tracker.loc());
}

const Def* Parser::parse_variadic() {
    Tracker tracker(this);
    eat(TT::D_quote_l);

    auto arity = parse_def();
    push_debruijn_type(arity);

    expect(TT::Semicolon, "variadic");
    auto body = parse_def();
    expect(TT::D_quote_r, "variadic");
    pop_debruijn_binders();
    return world_.variadic(arity, body, tracker.loc());
}

const Def* Parser::parse_variant() {
    Tracker tracker(this);
    eat(TT::D_brace_l);

    auto defs = parse_list(TT::D_brace_r, TT::Comma, "elements of variant type", [&]{ auto elem = parse_def(); return elem; });
    if (defs.empty())
        error("expected variant type with at least one operand at {}", tracker.loc());
    return world_.variant(defs, tracker.loc());
}

const Def* Parser::parse_lambda() {
    Tracker tracker(this);
    eat(TT::Lambda);
    auto domain = parse_def();
    push_debruijn_type(domain);
    expect(TT::Dot, "λ abstraction");
    auto body = parse_def();
    pop_debruijn_binders();
    return world_.lambda(domain, body, tracker.loc());
}

const Def* Parser::parse_optional_qualifier() {
    const Def* q = world_.qualifier_u();
    switch (ahead().tag()) {
        case TT::Backslash:   q = parse_debruijn();   break;
        case TT::Identifier:  q = parse_identifier(); break;
        case TT::Q_u: next(); q = world_.qualifier_u(); break;
        case TT::Q_r: next(); q = world_.qualifier_r(); break;
        case TT::Q_a: next(); q = world_.qualifier_a(); break;
        case TT::Q_l: next(); q = world_.qualifier_l(); break;
        default: break;
    }

    return q;
}

const Def* Parser::parse_qualified_kind() {
    auto kind_tag = ahead().tag();
    eat(kind_tag);
    const Def* qualifier = parse_optional_qualifier();
    switch (kind_tag) {
        case TT::Star:
            return world_.star(qualifier);
        case TT::Arity_Kind:
            return world_.arity_kind(qualifier);
        case TT::Multi_Arity_Kind:
            return world_.multi_arity_kind(qualifier);
        default:
            THORIN_UNREACHABLE;
    }
}

const Def* Parser::parse_tuple() {
    Tracker tracker(this);
    eat(TT::D_paren_l);
    auto elems = parse_list(TT::D_paren_r, TT::Comma, "elements of tuple", [&]{ return parse_def(); });
    return world_.tuple(elems, tracker.loc());
}

const Def* Parser::parse_pack() {
    Tracker tracker(this);
    eat(TT::D_angle_l);

    auto arity = parse_def();
    expect(TT::Semicolon, "pack");
    push_debruijn_type(arity);
    auto body = parse_def();
    expect(TT::D_angle_r, "pack");
    pop_debruijn_binders();

    return world_.pack(arity, body, tracker.loc());
}

const Def* Parser::parse_extract_or_insert(Tracker tracker, const Def* a) {
    auto b = parse_def();
    if (accept(TT::Arrow)) {
        auto c = parse_def();
        return world_.insert(a, b, c, tracker.loc());
    }

    return world_.extract(a, b, tracker.loc());
}

const Def* Parser::parse_literal() {
    Tracker tracker(this);
    assert(ahead().isa(TT::Literal));
    auto literal = next().literal();
    if (literal.tag == Literal::Tag::Lit_index) {
        assert(ahead().isa(TT::Literal));
        auto index_arity = next().literal();
        assert(index_arity.tag == Literal::Tag::Lit_index_arity);
        const Def* qualifier = parse_optional_qualifier();
        return world_.index(world_.arity(qualifier, index_arity.box.get_u64()), literal.box.get_u64(), tracker.loc());
    } else if (literal.tag == Literal::Tag::Lit_arity) {
        const Def* qualifier = parse_optional_qualifier();
        return world_.arity(qualifier, literal.box.get_u64(), tracker.loc());
    }

    expect(TT::ColonColon, "literal");
    auto type = parse_def();
    return world_.lit(type, literal.box, tracker.loc());
}

Token Parser::next() {
    auto result = ahead();
    ahead_[0] = ahead_[1];
    ahead_[1] = lexer_.lex();
    return result;
}

Token Parser::eat(TT tag) {
    auto result = ahead();
    assert_unused(result.isa(tag));
    next();
    return result;
}

Token Parser::expect(TT tag, const char* context) {
    auto result = ahead();
    if (!ahead().isa(tag)) {
        error("expected '{}', got '{}' while parsing {}", Token::tag_to_string(tag), Token::tag_to_string(ahead().tag()), context);
    }
    next();
    return result;
}

bool Parser::accept(TT tag) {
    if (!ahead().isa(tag))
        return false;
    next();
    return true;
}

const Def* Parser::parse_identifier() {
    Tracker tracker(this);
    auto symbol = eat(TT::Identifier).symbol();
    const Def* def = nullptr;
    if (accept(TT::Colon)) {
        // binder
        def = parse_def();
        insert_identifier(symbol);
    } else if (accept(TT::Equal)) { // id = e; def
        // let
        // TODO parse multiple lets/nominals into one scope for mutual recursion and such
        push_scope();
        auto let = parse_def();
        expect(TT::Semicolon, "a let definition");
        insert_identifier(symbol, let);
        def = parse_def();
        pop_scope();
    } else if (accept(TT::ColonColon)) { // id :: type := e; def
        // TODO parse multiple lets/nominals into one scope for mutual recursion and such
        auto type = parse_def();
        expect(TT::ColonEqual, "a nominal definition");

        if (accept(TT::Lambda)) {
            auto lambda = world_.lambda(type->as<Pi>(), tracker.loc()); // TODO properly set back location
            insert_identifier(symbol, lambda);
            Tracker tracker(this);
            push_scope();
            auto symbol = expect(TT::Identifier, "parameter name for nomimal λ").symbol();
            insert_identifier(symbol, lambda->param({tracker.loc(), symbol}));
            expect(TT::Dot, "λ abstraction");
            auto body = parse_def();
            pop_scope();
            lambda->set(body);
        } else {
            assert(false && "TODO");
        }

        eat(TT::Semicolon);
        push_scope();
        def = parse_def();
        pop_scope();
    } else {
        // use
        auto def_or_index = lookup(tracker, symbol);
        if (def_or_index.is_index()) {
            const Binder& it = binders_[def_or_index.index()];
            auto index = depth_ - it.depth;
            assert(index > 0);
            auto type = shift_free_vars(debruijn_types_[it.depth], index);
            const Def* var = world_.var(type, index - 1, tracker.loc());
            for (auto i : reverse_range(it.indices))
                var = world_.extract(var, i, tracker.loc());
            def = var;
        } else { // nominal, let, or axiom
            return def_or_index.def();
        }
    }
    return def;
}

void Parser::push_debruijn_type(const Def* bruijn) {
    depth_++;
    debruijn_types_.push_back(bruijn);
}

void Parser::pop_debruijn_binders() {
    depth_--;
    while (!binders_.empty()) {
        auto binder = binders_.back();
        if (binder.depth < depth_) break;
        if (binder.shadow.is_def() && binder.shadow.def() == nullptr)
            id2def_or_index_.erase(binder.name);
        else
            id2def_or_index_[binder.name] = binder.shadow;
        binders_.pop_back();
    }
    debruijn_types_.pop_back();
}

void Parser::shift_binders(size_t count) {
    // shift binders declared within a sigma by adding extracts
    size_t new_depth = depth_ - count;
    for (Binder& binder : reverse_range(binders_)) {
        if (binder.depth < new_depth) break;
        binder.indices.push_back(binder.depth - new_depth);
        binder.depth = new_depth;
    }
    debruijn_types_.resize(debruijn_types_.size() - count);
    depth_ = new_depth;
}

Parser::DefOrIndex Parser::lookup(const Tracker& tracker, Symbol identifier) {
    assert(!identifier.empty() && "identifier is empty");

    auto def_or_index = id2def_or_index_.find(identifier);
    if (def_or_index == id2def_or_index_.end()) {
        if (auto a = world_.lookup_axiom(identifier))
            return a;
        else if (auto e = world_.lookup_external(identifier))
            return e;
        else {
            error(tracker.loc(), "'{}' not found in current scope", identifier.str());
        }
    }
    return def_or_index->second;
}

void Parser::insert_identifier(Symbol identifier, const Def* def) {
    assert(!identifier.empty() && "identifier is empty");

    DefOrIndex shadow = (const Def*) nullptr;
    auto def_or_index = id2def_or_index_.find(identifier);
    if (def_or_index != id2def_or_index_.end())
        shadow = def_or_index->second;

    DefOrIndex mapping = def;
    if (def)
        definitions_.emplace_back(identifier, def, shadow);
    else {
        // nullptr means we want a binder
        mapping = binders_.size();
        binders_.emplace_back(identifier, depth_, shadow);
    }

    id2def_or_index_[identifier] = mapping;
}

void Parser::push_scope() {
    definition_scopes_.push_back(definitions_.size());
    // binder_scopes_.push_back(binders_.size());
}

void Parser::pop_scope() {
    size_t scope = definition_scopes_.back();
    // iterate from the back, such that earlier shadowing declarations re-set the shadow correctly
    for (size_t i = definitions_.size() - 1, e = scope - 1; i != e; --i) {
        auto decl = definitions_[i];
        auto current = id2def_or_index_.find(decl.name);
        if (current != id2def_or_index_.end()) {
            if (current->second.is_index()) {
                // if a binder has been declared in this scope and it is still around,
                // it overrides this id and also whatever we shadowed, but instead of
                // shadowing this id after this scope it instead shadows the previous shadow
                auto binder = binders_[current->second.index()];
                assert(binder.shadow.def() == decl.def);
                binder.shadow = decl.shadow;
                continue; // nothing else to unbind/rebind
            } else {
                assert(current->second.def() == decl.def);
            }
        }

        if (decl.shadow.is_def() && decl.shadow.def() == nullptr) {
            id2def_or_index_.erase(decl.name);
        }
        else {
            id2def_or_index_[decl.name] = decl.shadow;
        }
    }

    definitions_.resize(scope, {Symbol("dummy"), nullptr, 1});
    definition_scopes_.pop_back();
}

//------------------------------------------------------------------------------

const Def* parse(World& world, const char* str) {
    std::istringstream is(str, std::ios::binary);
    Lexer lexer(is, "stdin");
    return Parser(world, lexer).parse_def();
}

}
