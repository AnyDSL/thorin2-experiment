#include "thorin/frontend/parser.h"

#include "thorin/transform/reduce.h"
#include "thorin/util/log.h"

namespace thorin {

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
    std::vector<std::pair<const Def*, Location>> defs;
    parse_curried_defs(defs);
    assert(!defs.empty());
    const Def* def = defs.front().first;
    for (auto [ arg, loc ] : range(defs.begin()+1, defs.end()))
        def = world_.app(def, arg, loc);
    return def;
}

void Parser::parse_curried_defs(std::vector<std::pair<const Def*, Location>>& defs) {
    Tracker tracker(this);
    const Def* def = nullptr;

    if (false) {}
    else if (accept(Token::Tag::Bool))              def = world_.type_bool();
    else if (ahead().isa(Token::Tag::Cn))           def = parse_cn_type();
    else if (ahead().isa(Token::Tag::Pi))           def = parse_pi();
    else if (ahead().isa(Token::Tag::L_Bracket))    def = parse_sigma_or_variadic();
    else if (ahead().isa(Token::Tag::Lambda))       def = parse_lambda();
    else if (ahead().isa(Token::Tag::Star)
            || ahead().isa(Token::Tag::Arity_Kind)
            || ahead().isa(Token::Tag::Multi_Arity_Kind)) def = parse_qualified_kind();
    else if (ahead().isa(Token::Tag::Backslash))    def = parse_debruijn();
    else if (ahead().isa(Token::Tag::Identifier))   def = parse_identifier();
    else if (ahead().isa(Token::Tag::L_Paren))      def = parse_tuple_or_pack();
    else if (ahead().isa(Token::Tag::L_Brace))      def = parse_lit();
    else if (ahead().isa(Token::Tag::Literal))      def = parse_literal();
    else if (accept(Token::Tag::Qualifier_Type))    def = world_.qualifier_type();
    else if (accept(Token::Tag::QualifierU))        def = world_.unlimited();
    else if (accept(Token::Tag::QualifierR))        def = world_.relevant();
    else if (accept(Token::Tag::QualifierA))        def = world_.affine();
    else if (accept(Token::Tag::QualifierL))        def = world_.linear();
    else if (accept(Token::Tag::Multi_Arity_Kind))  def = world_.multi_arity_kind();

    if (def == nullptr)
        ELOG_LOC(ahead().location(), "expected some expression");

    if (accept(Token::Tag::Sharp))
        def = parse_extract_or_insert(tracker, def);
    // TODO insert <-

    defs.emplace_back(def, tracker.location());

    // an expression follows that can be an argument for an app
    if (ahead().is_app_arg())
        parse_curried_defs(defs);
}

const Def* Parser::parse_debruijn() {
    Tracker tracker(this);
    // De Bruijn index
    eat(Token::Tag::Backslash);
    if (ahead().isa(Token::Tag::Literal)) {
        auto lit = ahead().literal();
        eat(Token::Tag::Literal);
        if (lit.tag == Literal::Tag::Lit_untyped) {
            auto index = lit.box.get_u64();
            return world_.var(debruijn_types_[debruijn_types_.size() - index - 1], index, tracker.location());
        } else
            assertf(false, "untyped literal expected after '\'");
    } else {
        assertf(false, "number expected after '\'");
    }
}

const Def* Parser::parse_cn_type() {
    Tracker tracker(this);
    eat(Token::Tag::Cn);
    auto domain = parse_def();
    return world_.cn_type(domain, tracker.location());
}

const Def* Parser::parse_pi() {
    Tracker tracker(this);
    eat(Token::Tag::Pi);
    auto domain = parse_def();
    push_debruijn_type(domain);
    expect(Token::Tag::Dot, "Π type");
    auto body = parse_def();
    pop_debruijn_binders();
    return world_.pi(domain, body, tracker.location());
}

std::vector<const Def*> Parser::parse_sigma_ops() {
    Tracker tracker(this);
    eat(Token::Tag::L_Bracket);

    if (accept(Token::Tag::R_Bracket))
        return {};

    auto first = parse_def();
    push_debruijn_type(first);

    if (accept(Token::Tag::Semicolon)) {
        auto body = parse_def();
        expect(Token::Tag::R_Bracket, "variadic");
        pop_debruijn_binders();
        return {world_.variadic(first, body, tracker.location())};
    }

    auto defs = parse_list(Token::Tag::R_Bracket, Token::Tag::Comma, [&] {
            auto elem = parse_def();
            push_debruijn_type(elem);
            return elem;
        }, "elements of sigma type", first);
    // shift binders declared within the sigma by generating
    // extracts to the DeBruijn variable \0 (the current binder)
    shift_binders(defs.size());
    return defs;
}

const Def* Parser::parse_sigma_or_variadic() {
    Tracker tracker(this);
    return world_.sigma(parse_sigma_ops(), tracker.location());
}

DefArray Parser::parse_lambda_ops() {
    Tracker tracker(this);
    eat(Token::Tag::Lambda);
    auto domain = parse_def();
    push_debruijn_type(domain);
    expect(Token::Tag::Dot, "λ abstraction");
    auto body = parse_def();
    pop_debruijn_binders();
    return {domain, body};
}

const Def* Parser::parse_lambda() {
    Tracker tracker(this);
    auto ops = parse_lambda_ops();
    return world_.lambda(ops[0], ops[1], tracker.location());
}

const Def* Parser::parse_optional_qualifier() {
    const Def* q = world_.unlimited();
    switch (ahead().tag()) {
        case Token::Tag::Backslash:  q = parse_debruijn(); break;
        case Token::Tag::Identifier: q = parse_identifier(); break;
        case Token::Tag::QualifierU: next(); q = world_.unlimited(); break;
        case Token::Tag::QualifierR: next(); q = world_.relevant(); break;
        case Token::Tag::QualifierA: next(); q = world_.affine(); break;
        case Token::Tag::QualifierL: next(); q = world_.linear(); break;
        default: break;
    }

    return q;
}

const Def* Parser::parse_qualified_kind() {
    auto kind_tag = ahead().tag();
    eat(kind_tag);
    const Def* qualifier = parse_optional_qualifier();
    switch (kind_tag) {
        case Token::Tag::Star:
            return world_.star(qualifier);
        case Token::Tag::Arity_Kind:
            return world_.arity_kind(qualifier);
        case Token::Tag::Multi_Arity_Kind:
            return world_.multi_arity_kind(qualifier);
        default:
            THORIN_UNREACHABLE;
    }
}

const Def* Parser::parse_tuple_or_pack() {
    Tracker tracker(this);
    eat(Token::Tag::L_Paren);

    if (accept(Token::Tag::R_Paren))
        return world_.val_unit();

    auto first = parse_def();
    if (accept(Token::Tag::Semicolon)) {
        push_debruijn_type(first);
        auto body = parse_def();
        expect(Token::Tag::R_Paren, "pack");
        pop_debruijn_binders();
        return world_.pack(first, body, tracker.location());
    }

    auto defs = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); }, "elements of tuple", first);
    return world_.tuple(defs, tracker.location());
}

const Def* Parser::parse_lit() {
    Tracker tracker(this);

    eat(Token::Tag::L_Brace);
    if (!ahead().isa(Token::Tag::Literal))
        ELOG_LOC(ahead().location(), "literal expected");

    auto box = ahead().literal().box;
    eat(Token::Tag::Literal);

    expect(Token::Tag::Colon, "literal");
    auto type = parse_def();
    expect(Token::Tag::R_Brace, "literal");

    return world_.lit(type, box, tracker.location());
}

const Def* Parser::parse_extract_or_insert(Tracker tracker, const Def* a) {
    auto b = parse_def();
    if (accept(Token::Tag::Arrow)) {
        auto c = parse_def();
        return world_.insert(a, b, c, tracker.location());
    }

    return world_.extract(a, b, tracker.location());
}

const Def* Parser::parse_literal() {
    Tracker tracker(this);
    assert(ahead().isa(Token::Tag::Literal));
    auto literal = next().literal();
    if (literal.tag == Literal::Tag::Lit_index) {
        assert(ahead().isa(Token::Tag::Literal));
        auto index_arity = next().literal();
        assert(index_arity.tag == Literal::Tag::Lit_index_arity);
        const Def* qualifier = parse_optional_qualifier();
        return world_.index(world_.arity(index_arity.box.get_u64(), qualifier), literal.box.get_u64(), tracker.location());
    } else if (literal.tag == Literal::Tag::Lit_arity) {
        const Def* qualifier = parse_optional_qualifier();
        return world_.arity(literal.box.get_u64(), qualifier, tracker.location());
    }

    assertf(false, "unhandled literal {}", literal.box.get_u64());
}

Token Parser::next() {
    auto result = ahead();
    ahead_[0] = ahead_[1];
    ahead_[1] = lexer_.lex();
    return result;
}

void Parser::eat(Token::Tag tag) {
    assert_unused(ahead().isa(tag));
    next();
}

void Parser::expect(Token::Tag tag, const char* context) {
    if (!ahead().isa(tag)) {
        ELOG_LOC(ahead().location(), "expected '{}', got '{}' while parsing {}",
                Token::tag_to_string(tag), Token::tag_to_string(ahead().tag()), context);
    }
    next();
}

bool Parser::accept(Token::Tag tag) {
    if (!ahead().isa(tag))
        return false;
    next();
    return true;
}

const Def* Parser::parse_identifier() {
    Tracker tracker(this);
    auto id = ahead().identifier();
    next();
    const Def* def = nullptr;
    if (accept(Token::Tag::Colon)) {
        // binder
        def = parse_def();
        insert_identifier(id);
    } else if (accept(Token::Tag::Equal)) { // id = e; def
        // let
        // TODO parse multiple lets/nominals into one scope for mutual recursion and such
        push_decl_scope();
        auto let = parse_def();
        expect(Token::Tag::Semicolon, "a let definition");
        insert_identifier(id, let);
        def = parse_def();
        pop_decl_scope();
    } else if (accept(Token::Tag::ColonColon)) { // id :: type := e; def
        // TODO parse multiple lets/nominals into one scope for mutual recursion and such
        auto type = parse_def();
        expect(Token::Tag::ColonEqual, "a nominal definition");
        // we first abuse the variable to debruijn lookup to parse the whole def structurally
        insert_identifier(id);
        push_debruijn_type(type);
        auto structural = parse_def();
        pop_debruijn_binders();
        // build the stub
        Def* nominal = nullptr;
        Debug dbg(tracker.location(), id.str());
        if (auto lambda = structural->isa<Lambda>())
            nominal = world_.lambda(lambda->type(), dbg);
        else if (auto sigma = structural->isa<Sigma>())
            nominal = world_.sigma(type, sigma->num_ops(), dbg);
        else
            assertf(false, "TODO unimplemented nominal {} with type {} at {}",
                    structural, type, tracker.location());
        // rebuild the structural def with the real nominal def and finally set the ops of the nominal def
        auto reduced = reduce(structural, nominal);
        nominal->set(reduced->ops());
        // now we can parse the rest with the real mapping
        insert_identifier(id, nominal);
        eat(Token::Tag::Semicolon);
        push_decl_scope();
        def = parse_def();
        pop_decl_scope();
    } else {
        // use
        auto decl = lookup(tracker, id);
        if (std::holds_alternative<size_t>(decl)) {
            auto binder_idx = std::get<size_t>(decl);
            const Binder& it = binders_[binder_idx];
            auto index = depth_ - it.depth;
            auto type = shift_free_vars(debruijn_types_[it.depth], index);
            const Def* var = world_.var(type, index - 1, tracker.location());
            for (auto i : reverse_range(it.indices))
                var = world_.extract(var, i, tracker.location());
            def = var;
        } else { // nominal, let, or axiom
            return std::get<const Def*>(decl);
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
        if (std::holds_alternative<const Def*>(binder.shadow) && std::get<const Def*>(binder.shadow) == nullptr)
            id2defbinder_.erase(binder.name);
        else
            id2defbinder_[binder.name] = binder.shadow;
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

Parser::DefOrBinder Parser::lookup(const Tracker& t, Symbol identifier) {
    assert(!identifier.empty() && "identifier is empty");

    auto decl = id2defbinder_.find(identifier);
    if (decl == id2defbinder_.end()) {
        if (auto a = world_.lookup_axiom(identifier))
            return a;
        else if (auto e = world_.lookup_external(identifier))
            return e;
        else
            assertf(false, "'{}' at {} not found in current scope", identifier.str(), t.location());
    }
    return decl->second;
}

void Parser::insert_identifier(Symbol identifier, const Def* def) {
    assert(!identifier.empty() && "identifier is empty");

    DefOrBinder shadow = (const Def*) nullptr;
    auto decl = id2defbinder_.find(identifier);
    if (decl != id2defbinder_.end())
        shadow = decl->second;

    DefOrBinder mapping = def;
    if (def)
        declarations_.emplace_back(identifier, def, shadow);
    else {
        // nullptr means we want a binder
        mapping = binders_.size();
        binders_.emplace_back(identifier, depth_, shadow);
    }

    id2defbinder_[identifier] = mapping;
}

void Parser::push_decl_scope() { scopes_.push_back(declarations_.size()); }

void Parser::pop_decl_scope() {
    size_t scope = scopes_.back();
    // iterate from the back, such that earlier shadowing declarations re-set the shadow correctly
    for (size_t i = declarations_.size() - 1, e = scope - 1; i != e; --i) {
        auto decl = declarations_[i];

        auto current = id2defbinder_.find(decl.name);
        if (current != id2defbinder_.end()) {
            if (std::holds_alternative<size_t>(current->second)) {
                // if a binder has been declared in this scope and it is still around,
                // it overrides this id and also whatever we shadowed, but instead of
                // shadowing this id it instead shadows the previous shadow
                auto binder = binders_[std::get<size_t>(current->second)];
                assert(std::get<const Def*>(binder.shadow) == decl.def);
                binder.shadow = decl.shadow;
                continue; // nothing else to unbind/rebind
            } else {
                assert(std::get<const Def*>(current->second) == decl.def);
            }
        }

        if (std::holds_alternative<const Def*>(decl.shadow) && std::get<const Def*>(decl.shadow) == nullptr)
            id2defbinder_.erase(decl.name);
        else
            id2defbinder_[decl.name] = decl.shadow;
    }

    declarations_.resize(scope, {Symbol("dummy"), nullptr, 1});
    scopes_.pop_back();
}

//------------------------------------------------------------------------------

const Def* parse(World& world, const char* str) {
    std::istringstream is(str, std::ios::binary);
    Lexer lexer(is, "stdin");
    return Parser(world, lexer).parse_def();
}

}
