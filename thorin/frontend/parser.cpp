#include "parser.h"

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
    const Def* def = nullptr;

    if (false) {}
    else if (ahead().isa(Token::Tag::Pi))         def = parse_pi();
    else if (ahead().isa(Token::Tag::L_Bracket))  def = parse_sigma_or_variadic();
    else if (ahead().isa(Token::Tag::Lambda))     def = parse_lambda();
    else if (ahead().isa(Token::Tag::Star)
             || ahead().isa(Token::Tag::Arity_Kind)
             || ahead().isa(Token::Tag::Multi_Arity_Kind)) def = parse_qualified_kind();
    else if (ahead().isa(Token::Tag::Backslash) ||
             ahead().isa(Token::Tag::Identifier)) def = parse_var_or_binder();
    else if (ahead().isa(Token::Tag::L_Paren))    def = parse_tuple_or_pack();
    else if (ahead().isa(Token::Tag::L_Brace))    def = parse_assume();
    else if (ahead().isa(Token::Tag::Literal))    def = parse_literal();
    else if (accept(Token::Tag::Qualifier_Type))    def = world_.qualifier_type();
    else if (accept(Token::Tag::QualifierU))        def = world_.unlimited();
    else if (accept(Token::Tag::QualifierR))        def = world_.relevant();
    else if (accept(Token::Tag::QualifierA))        def = world_.affine();
    else if (accept(Token::Tag::QualifierL))        def = world_.linear();
    else if (accept(Token::Tag::Multi_Arity_Kind))  def = world_.multi_arity_kind();

    if (def != nullptr && accept(Token::Tag::Sharp))
        def = parse_extract_or_insert(tracker, def);

    // if another expression follows - we build an app
    auto tag = ahead().tag();
    if (   tag == Token::Tag::Pi
        || tag == Token::Tag::Lambda
        || tag == Token::Tag::Star
        || tag == Token::Tag::Backslash
        || tag == Token::Tag::Identifier
        || tag == Token::Tag::L_Bracket
        || tag == Token::Tag::L_Brace
        || tag == Token::Tag::L_Paren
        || tag == Token::Tag::Qualifier_Type
        || tag == Token::Tag::Arity_Kind
        || tag == Token::Tag::Multi_Arity_Kind) {
        auto arg = parse_def();
        return world_.app(def, arg, tracker.location());
    }

    if (def != nullptr)
        return def;

    assertf(false, "definition expected at {}", ahead());
}

const Def* Parser::parse_var_or_binder() {
    Tracker tracker(this);
    if (ahead().isa(Token::Tag::Identifier)) {
        auto id = ahead().identifier();
        next();
        if (accept(Token::Tag::Colon)) {
            // binder
            auto def = parse_def();
            binders_.emplace_back(id, depth_);
            return def;
        } else {
            // use
            auto it = std::find_if(binders_.rbegin(), binders_.rend(), [&] (auto& binder) {
                return binder.name == id;
            });
            if (it != binders_.rend()) {
                auto index = depth_ - it->depth;
                auto type = bruijn_[it->depth]->shift_free_vars(world_, index);
                const Def* var = world_.var(type, index - 1, tracker.location());
                for (auto i : it->ids)
                    var = world_.extract(var, i, tracker.location());
                return var;
            } else if (env_.count(id)) {
                return env_[id];
            } else {
                assertf(false, "unknown identifier '{}'", id);
            }
        }
    } else {
        // De Bruijn index
        eat(Token::Tag::Backslash);
        if (ahead().isa(Token::Tag::Literal)) {
            auto lit = ahead().literal();
            eat(Token::Tag::Literal);
            if (lit.tag == Literal::Tag::Lit_untyped) {
                auto index = lit.box.get_u64();
                return world_.var(bruijn_[bruijn_.size() - index - 1], index, tracker.location());
            } else {
                assertf(false, "untyped literal expected after '\'");
            }
        } else {
            assertf(false, "number expected after '\'");
        }
    }
    return nullptr;
}

const Pi* Parser::parse_pi() {
    Tracker tracker(this);
    eat(Token::Tag::Pi);
    auto domain = parse_def();
    push_identifiers(domain);
    expect(Token::Tag::Dot, "Π type");
    auto body = parse_def();
    pop_identifiers();
    return world_.pi(domain, body, tracker.location());
}

const Def* Parser::parse_sigma_or_variadic() {
    Tracker tracker(this);
    eat(Token::Tag::L_Bracket);

    if (accept(Token::Tag::R_Bracket))
        return world_.unit();

    auto first = parse_def();
    push_identifiers(first);

    if (accept(Token::Tag::Semicolon)) {
        auto body = parse_def();
        expect(Token::Tag::R_Bracket, "variadic");
        pop_identifiers();
        return world_.variadic(first, body, tracker.location());
    }

    auto defs = parse_list(Token::Tag::R_Bracket, Token::Tag::Comma,
        [&] {
            auto elem = parse_def();
            push_identifiers(elem);
            return elem;
        }, "elements of sigma type", first);
    auto sigma = world_.sigma(defs, tracker.location());
    // shift identifiers declared within the sigma by generating
    // extracts to the DeBruijn variable \0 (the current binder)
    shift_identifiers(defs.size());
    return sigma;
}

const Def* Parser::parse_lambda() {
    Tracker tracker(this);
    eat(Token::Tag::Lambda);
    auto domain = parse_def();
    push_identifiers(domain);
    expect(Token::Tag::Dot, "λ abstraction");
    auto body = parse_def();
    pop_identifiers();
    return world_.lambda(domain, body, tracker.location());
}

const Def* Parser::parse_qualified_kind() {
    auto kind_tag = ahead().tag();
    eat(kind_tag);
    const Def* qualifier = world_.unlimited();
    auto tag = ahead().tag();
    if (tag == Token::Tag::Backslash
        || tag == Token::Tag::Identifier
        || tag == Token::Tag::L_Paren
        || tag == Token::Tag::QualifierU
        || tag == Token::Tag::QualifierR
        || tag == Token::Tag::QualifierA
        || tag == Token::Tag::QualifierL) {
        qualifier = parse_def();
    }
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
        push_identifiers(first);
        auto body = parse_def();
        expect(Token::Tag::R_Paren, "pack");
        pop_identifiers();
        return world_.pack(first, body, tracker.location());
    }

    auto defs = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); }, "elements of tuple", first);
    return world_.tuple(defs, tracker.location());
}

const Axiom* Parser::parse_assume() {
    Tracker tracker(this);

    eat(Token::Tag::L_Brace);
    if (!ahead_[0].isa(Token::Tag::Literal))
        ELOG_LOC(ahead_[0].location(), "literal expected in {}");

    auto box = ahead_[0].literal().box;
    eat(Token::Tag::Literal);
    expect(Token::Tag::R_Brace, "assumption");

    expect(Token::Tag::Colon, "assumption");
    auto type = parse_def();

    return world_.assume(type, box, tracker.location());
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
    assert(ahead().isa(Token::Tag::Literal));
    auto literal = next().literal();
    if (literal.tag == Literal::Tag::Lit_arity) {
        auto tag = ahead().tag();
        const Def* qualifier = world_.unlimited();
        if (tag == Token::Tag::Backslash
            || tag == Token::Tag::Identifier
            || tag == Token::Tag::L_Paren
            || tag == Token::Tag::QualifierU
            || tag == Token::Tag::QualifierR
            || tag == Token::Tag::QualifierA
            || tag == Token::Tag::QualifierL) {
            qualifier = parse_def();
        }
        return world_.arity(literal.box.get_u64(), qualifier);
    }

    assertf(false, "unhandled literal {}", literal.box.get_u64());
}

Token Parser::next() {
    auto result = ahead_[0];
    ahead_[0] = ahead_[1];
    ahead_[1] = lexer_.lex();
    return result;
}

void Parser::eat(Token::Tag tag) {
    assert(ahead_[0].isa(tag));
    next();
}

void Parser::expect(Token::Tag tag, const char* context) {
    if (!ahead_[0].isa(tag)) {
        ELOG_LOC(ahead_[0].location(), "expected '{}', got '{}' while parsing {}",
                Token::tag_to_string(tag), Token::tag_to_string(ahead_[0].tag()), context);
    }
    next();
}

bool Parser::accept(Token::Tag tag) {
    if (!ahead_[0].isa(tag))
        return false;
    next();
    return true;
}

const Def* parse(World& world, const std::string& str, Env env) {
    std::istringstream is(str, std::ios::binary);
    Lexer lexer(is, "stdin");
    return Parser(world, lexer, env).parse_def();
}

}
