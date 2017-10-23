#include "parser.h"

namespace thorin {

const Def* Parser::parse_def() {
    Tracker tracker(this);
    const Def* def = nullptr;

    if (false) {}
    else if (ahead_[0].isa(Token::Tag::Pi))         def = parse_pi();
    else if (ahead_[0].isa(Token::Tag::Sigma))      def = parse_sigma();
    else if (ahead_[0].isa(Token::Tag::Lambda))     def = parse_lambda();
    else if (ahead_[0].isa(Token::Tag::Star))       def = parse_star();
    else if (ahead_[0].isa(Token::Tag::Sharp) ||
             ahead_[0].isa(Token::Tag::Identifier)) def = parse_var();
    else if (ahead_[0].isa(Token::Tag::L_Bracket))  def = parse_tuple();
    else if (ahead_[0].isa(Token::Tag::L_Brace))    def = parse_assume();
    else if (ahead_[0].isa(Token::Tag::L_Paren)) {
        eat(Token::Tag::L_Paren);
        def = parse_def();
        expect(Token::Tag::R_Paren);
    }
    else if (accept(Token::Tag::Qualifier_Type)) def = world_.qualifier_type();
    else if (accept(Token::Tag::Qualifier_Kind)) def = world_.qualifier_kind();
    else if (accept(Token::Tag::Arities))        def = world_.arities();
    else if (accept(Token::Tag::Multi_Arities))  def = world_.multi_arities();

    switch (ahead_[0].tag()) {
        case Token::Tag::Pi:
        case Token::Tag::Sigma:
        case Token::Tag::Lambda:
        case Token::Tag::Star:
        case Token::Tag::Sharp:
        case Token::Tag::Identifier:
        case Token::Tag::L_Bracket:
        case Token::Tag::L_Brace:
        case Token::Tag::L_Paren:
        case Token::Tag::Qualifier_Type:
        case Token::Tag::Qualifier_Kind:
        case Token::Tag::Arities:
        case Token::Tag::Multi_Arities: {
            auto arg = parse_def();
            return world_.app(def, arg, tracker.location());
        }
    }

    if (def != nullptr)
        return def;

    assertf(false, "definition expected in {}", ahead_[0].location());
}

const Pi* Parser::parse_pi() {
    Tracker tracker(this);
    eat(Token::Tag::Pi);

    if (accept(Token::Tag::L_Paren)) {
        auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_param(); });
        expect(Token::Tag::Dot);
        auto body = parse_def();

        pop_identifiers();

        return world_.pi(domains, body, tracker.location());
    }

    auto domain = parse_param();
    expect(Token::Tag::Dot);
    auto body = parse_def();
    pop_identifiers();
    return world_.pi(domain, body, tracker.location());
}

const Def* Parser::parse_sigma() {
    Tracker tracker(this);
    eat(Token::Tag::Sigma);

    expect(Token::Tag::L_Paren);
    auto defs = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); });

    return world_.sigma(defs, tracker.location());
}

const Def* Parser::parse_lambda() {
    Tracker tracker(this);
    eat(Token::Tag::Lambda);

    expect(Token::Tag::L_Paren);
    auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_param(); });
    expect(Token::Tag::Dot);
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

        expect(Token::Tag::Colon);
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

const Def* Parser::parse_tuple() {
    Tracker tracker(this);
    eat(Token::Tag::L_Bracket);

    auto defs = parse_list(Token::Tag::R_Bracket, Token::Tag::Comma, [&] { return parse_def(); });

    if (accept(Token::Tag::Colon)) {
        auto type = parse_def();
        return world_.tuple(type, defs, tracker.location());
    }

    return world_.tuple(defs, tracker.location());
}

const Axiom* Parser::parse_assume() {
    Tracker tracker(this);

    eat(Token::Tag::L_Brace);
    if (!ahead_[0].isa(Token::Tag::Literal))
        ELOG_LOC(ahead_[0].location(), "literal expected in {}");

    auto box = ahead_[0].literal().box;
    eat(Token::Tag::Literal);
    expect(Token::Tag::R_Brace);

    expect(Token::Tag::Colon);
    auto type = parse_def();

    return world_.assume(type, box, tracker.location());
}

const Def* Parser::parse_param() {
    const Def* def;
    if (ahead_[0].isa(Token::Tag::Identifier) && ahead_[1].isa(Token::Tag::Colon)) {
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
    ahead_[1] = lexer_.next();
    return result;
}

void Parser::eat(Token::Tag tag) {
    assert(ahead_[0].isa(tag));
    next();
}

void Parser::expect(Token::Tag tag) {
    if (!ahead_[0].isa(tag))
        ELOG_LOC(ahead_[0].location(), "'{}' expected", Token::tag_to_string(tag));

    next();
}

bool Parser::accept(Token::Tag tag) {
    if (!ahead_[0].isa(tag))
        return false;
    next();
    return true;
}

const Def* parse(WorldBase& world, const std::string& str, Env env) {
    std::istringstream is(str);
    Lexer lexer(is, "stdin");
    return Parser(world, lexer, env).parse_def();
}

}
