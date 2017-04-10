#include "parser.h"

namespace thorin {

const Def* Parser::parse_def() {
    if (ahead_.isa(Token::Tag::Pi))         return parse_pi();
    if (ahead_.isa(Token::Tag::Sigma))      return parse_sigma();
    if (ahead_.isa(Token::Tag::Lambda))     return parse_lambda();
    if (ahead_.isa(Token::Tag::Star))       return parse_star();
    if (ahead_.isa(Token::Tag::Sharp) ||
        ahead_.isa(Token::Tag::Identifier)) return parse_var();
    if (ahead_.isa(Token::Tag::L_Angle))    return parse_tuple();
    if (ahead_.isa(Token::Tag::L_Brace))    return parse_assume();
    if (ahead_.isa(Token::Tag::L_Paren)) {
        eat(Token::Tag::L_Paren);
        auto def = parse_def();
        expect(Token::Tag::R_Paren);
        return def;
    }

    assertf(false, "definition expected in {}", ahead_.location());
}

const Pi* Parser::parse_pi() {
    Tracker tracker(this);
    eat(Token::Tag::Pi);

    expect(Token::Tag::L_Paren);
    auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_param(); });
    expect(Token::Tag::Dot);
    auto body = parse_def();

    pop_identifiers();

    return world_.pi(domains, body, tracker.location());
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

const Var* Parser::parse_var() {
    Tracker tracker(this);

    const Def* def = nullptr;
    size_t index = 0;
    if (ahead_.isa(Token::Tag::Sharp)) {
        eat(Token::Tag::Sharp);
        if (!ahead_.isa(Token::Tag::Literal) ||
            ahead_.literal().tag != Literal::Tag::Lit_untyped)
            assertf(false, "DeBruijn index expected in {}", ahead_.location());

        index = ahead_.literal().box.get_u64();
        eat(Token::Tag::Literal);

        expect(Token::Tag::Colon);
        def = parse_def();
    } else if (ahead_.isa(Token::Tag::Identifier)) {
        auto it = id_map_.find(ahead_.identifier());
        if (it == id_map_.end())
            assertf(false, "unknown identifier '{}' in {}", ahead_.identifier(), ahead_.location());

        std::tie(std::ignore, def, index) = id_stack_[it->second];
        def = shift_def(def, depth_ - index);
        index = depth_ - index - 1;

        eat(Token::Tag::Identifier);
    }

    return world_.var(def, index, tracker.location());
}

const Def* Parser::parse_tuple() {
    Tracker tracker(this);
    eat(Token::Tag::L_Angle);

    auto defs = parse_list(Token::Tag::R_Angle, Token::Tag::Comma, [&] { return parse_def(); });
    expect(Token::Tag::Colon);
    auto type = parse_def();

    return world_.tuple(type, defs, tracker.location());
}

const Axiom* Parser::parse_assume() {
    Tracker tracker(this);

    eat(Token::Tag::L_Brace);
    if (!ahead_.isa(Token::Tag::Literal))
        ELOG_LOC(ahead_.location(), "literal expected in {}");

    auto box = ahead_.literal().box;
    eat(Token::Tag::Literal);
    expect(Token::Tag::R_Brace);

    expect(Token::Tag::Colon);
    auto type = parse_def();

    return world_.assume(type, box, tracker.location());
}

const Def* Parser::parse_param() {
    const Def* def;
    if (ahead_.isa(Token::Tag::Identifier)) {
        std::string ident = ahead_.identifier();
        eat(Token::Tag::Identifier);
        expect(Token::Tag::Colon);
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

void Parser::next() {
    ahead_ = lexer_.next();
}

void Parser::eat(Token::Tag tag) {
    assert(ahead_.isa(tag));
    next();
}

void Parser::expect(Token::Tag tag) {
    if (!ahead_.isa(tag))
        ELOG_LOC(ahead_.location(), "'{}' expected in {}", Token::tag_to_string(tag));

    next();
}

}
