#include "parser.h"

namespace thorin {

// current syntax
// e ::=    x : e       (binder)
//     |    x           (identifier)
//     |    \n          (DeBruijn index)
//     |    Πe.e        (pi type)
//     |    λe.e        (lambda abstraction)
//     |    [e,...]     (sigma type)
//     |    [e; e]      (variadic)
//     |    (e,...)     (tuple)
//     |    (e; e)      (pack)
//     |    e#e         (extract)
//     |    e#e <- e    (insert)

const Def* Parser::parse_def() {
    Tracker tracker(this);
    const Def* def = nullptr;

    if (false) {}
    else if (ahead_[0].isa(Token::Tag::Pi))         def = parse_pi();
    else if (ahead_[0].isa(Token::Tag::L_Bracket))  def = parse_sigma_or_variadic();
    else if (ahead_[0].isa(Token::Tag::Lambda))     def = parse_lambda();
    else if (ahead_[0].isa(Token::Tag::Star))       def = parse_star();
    else if (ahead_[0].isa(Token::Tag::Sharp) ||
             ahead_[0].isa(Token::Tag::Identifier)) def = parse_var();
    else if (ahead_[0].isa(Token::Tag::L_Paren))    def = parse_tuple_or_pack();
    else if (ahead_[0].isa(Token::Tag::L_Brace))    def = parse_assume();
    else if (accept(Token::Tag::Qualifier_Type))   def = world_.qualifier_type();
    else if (accept(Token::Tag::Qualifier_Kind))   def = world_.qualifier_kind();
    else if (accept(Token::Tag::Arity_Kind))       def = world_.arity_kind();
    else if (accept(Token::Tag::Multi_Arity_Kind)) def = world_.multi_arity_kind();

    // if another expression follows - we build an app
    auto tag = ahead_[0].tag();
    if (   tag == Token::Tag::Pi
        || tag == Token::Tag::Lambda
        || tag == Token::Tag::Star
        || tag == Token::Tag::Sharp
        || tag == Token::Tag::Identifier
        || tag == Token::Tag::L_Bracket
        || tag == Token::Tag::L_Brace
        || tag == Token::Tag::L_Paren
        || tag == Token::Tag::Qualifier_Type
        || tag == Token::Tag::Qualifier_Kind
        || tag == Token::Tag::Arity_Kind
        || tag == Token::Tag::Multi_Arity_Kind) {
        auto arg = parse_def();
        return world_.app(def, arg, tracker.location());
     }

    if (def != nullptr)
        return def;

    assertf(false, "definition expected in {}", ahead_[0].location());
}

const Pi* Parser::parse_pi() {
    Tracker tracker(this);
    eat(Token::Tag::Pi);

    if (accept(Token::Tag::L_Paren)) {
        auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_param(); }, "parameter list of Π type");
        expect(Token::Tag::Dot, "Π type");
        auto body = parse_def();

        pop_identifiers();

        return world_.pi(domains, body, tracker.location());
    }

    auto domain = parse_param();
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

    auto first = parse_param();

    if (accept(Token::Tag::Semicolon)) {
        auto body = parse_def();
        expect(Token::Tag::R_Bracket, "variadic");
        depth_--;
        pop_identifiers();
        return world_.variadic(first, body, tracker.location());
    }

    auto defs = parse_list(Token::Tag::R_Bracket, Token::Tag::Comma, [&] { return parse_param(); }, "elements of sigma type", first);
    pop_identifiers();
    return world_.sigma(defs, tracker.location());
}

const Def* Parser::parse_lambda() {
    Tracker tracker(this);
    eat(Token::Tag::Lambda);

    expect(Token::Tag::L_Paren, "λ abstraction");
    auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_param(); }, "parameter list of λ abstraction");
    expect(Token::Tag::Dot, "λ abstraction");
    auto body = parse_def();

    pop_identifiers();

    return world_.lambda(domains, body, tracker.location());
}

const Star* Parser::parse_star() {
    eat(Token::Tag::Star);
    return world_.star();
}

const Def* Parser::parse_var() {
    Tracker tracker(this);

    const Def* def = nullptr;
    size_t index = 0;
    if (ahead_[0].isa(Token::Tag::Sharp)) {
        eat(Token::Tag::Sharp);
        if (!ahead_[0].isa(Token::Tag::Literal) ||
            ahead_[0].literal().tag != Literal::Tag::Lit_untyped)
            assertf(false, "DeBruijn index expected in {}", ahead_[0].location());

        index = ahead_[0].literal().box.get_u64();
        eat(Token::Tag::Literal);

        expect(Token::Tag::Colon, "variable");
        def = parse_def();
    } else if (ahead_[0].isa(Token::Tag::Identifier)) {
        auto it = id_map_.find(ahead_[0].identifier());
        if (it != id_map_.end()) {
            std::tie(std::ignore, def, index) = id_stack_[it->second];
            def = shift_def(def, depth_ - index);
            index = depth_ - index - 1;

            eat(Token::Tag::Identifier);
            return world_.var(def, index, tracker.location());
        } else {
            auto it = env_.find(ahead_[0].identifier());
            if (it != env_.end()) {
                eat(Token::Tag::Identifier);
                return it->second;
            }
        }
    }

    assertf(false, "unknown identifier '{}' in {}", ahead_[0].identifier(), ahead_[0].location());
}

const Def* Parser::parse_tuple_or_pack() {
    auto ascribed_type = [&] { return accept(Token::Tag::Colon) ? parse_def() : nullptr; };

    Tracker tracker(this);
    eat(Token::Tag::L_Paren);

    if (accept(Token::Tag::R_Paren))
        return world_.tuple0();

    if (is_param()) { // must be a pack
        auto param = parse_param();
        expect(Token::Tag::Semicolon, "pack");
        auto body = parse_def();
        depth_--;
        pop_identifiers();

        if (auto type = ascribed_type())
            return world_.pack_nominal_sigma(type->as<Sigma>(), body, tracker.location());
        return world_.pack(param, body, tracker.location());
    }

    auto first = parse_def();

    if (accept(Token::Tag::Semicolon)) {
        auto body = parse_def();
        expect(Token::Tag::R_Bracket, "pack");
        depth_--;
        pop_identifiers();

        if (auto type = ascribed_type())
            return world_.pack(type->as<Sigma>(), body, tracker.location());
        return world_.pack(first, body, tracker.location());
    }

    auto defs = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); }, "elements of tuple", first);

    if (auto type = ascribed_type())
        return world_.tuple(type, defs, tracker.location());
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

const Def* Parser::parse_param() {
    const Def* def;
    if (is_param()) {
        std::string ident = ahead_[0].identifier();
        eat(Token::Tag::Identifier);
        eat(Token::Tag::Colon);
        def = parse_def();

        id_map_.emplace(ident, id_stack_.size());
        id_stack_.emplace_back(ident, def, depth_);
    } else
        def = parse_def();

    depth_++;
    return def;
}

const Def* Parser::shift_def(const Def* def, size_t shift) {
    if (def->is_nominal()) return def;
    if (auto var = def->isa<Var>())
        return world_.var(shift_def(var->type(), shift), var->index() + shift);
    Array<const Def*> ops(def->num_ops());
    for (size_t i = 0; i < def->num_ops(); i++) ops[i] = shift_def(def->op(i), shift);
    return def->rebuild(shift_def(def->type(), shift), ops);
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

const Def* parse(WorldBase& world, const std::string& str, Env env) {
    std::istringstream is(str, std::ios::binary);
    Lexer lexer(is, "stdin");
    return Parser(world, lexer, env).parse_def();
}

}
