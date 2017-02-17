#include "parser.h"

namespace thorin {

const Def* Parser::parse_def() {
    if (ahead_.isa(Token::Tag::Pi))      return parse_pi();
    if (ahead_.isa(Token::Tag::Sigma))   return parse_sigma();
    if (ahead_.isa(Token::Tag::Lambda))  return parse_lambda();
    if (ahead_.isa(Token::Tag::Star))    return parse_star();
    if (ahead_.isa(Token::Tag::L_Angle)) return parse_var();
    if (ahead_.isa(Token::Tag::L_Paren)) return parse_tuple();
    if (ahead_.isa(Token::Tag::L_Brace)) return parse_assume();
    ELOG("definition expected in {}", ahead_.location());
}

const Pi* Parser::parse_pi() {
    Anchor anchor(this);
    eat(Token::Tag::Pi);

    expect(Token::Tag::L_Paren);
    auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); });
    expect(Token::Tag::Dot);

    auto body = parse_def();

    return world_.pi(domains, body, anchor.location());
}

const Def* Parser::parse_sigma() {
    Anchor anchor(this);
    eat(Token::Tag::Sigma);

    expect(Token::Tag::L_Paren);
    auto defs = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); });

    return world_.sigma(defs, anchor.location());
}

const Lambda* Parser::parse_lambda() {
    Anchor anchor(this);
    eat(Token::Tag::Lambda);

    expect(Token::Tag::L_Paren);
    auto domains = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); });
    expect(Token::Tag::Dot);
    auto body = parse_def();

    return world_.lambda(domains, body, anchor.location());
}

const Star* Parser::parse_star() {
    eat(Token::Tag::Star);
    return world_.star();
}

const Var* Parser::parse_var() {
    Anchor anchor(this);
    eat(Token::Tag::L_Angle);

    if (!ahead_.isa(Token::Tag::Literal) ||
        ahead_.literal().tag != Literal::Tag::Lit_untyped) {
        ELOG("DeBruijn index expected in {}", ahead_.location());
    }
    size_t index = ahead_.literal().box.get_u64();
    eat(Token::Tag::Literal);

    expect(Token::Tag::Colon);
    auto def = parse_def();
    expect(Token::Tag::R_Angle);
    
    return world_.var(def, index, anchor.location());
}

const Def* Parser::parse_tuple() {
    Anchor anchor(this);
    eat(Token::Tag::L_Paren);

    auto defs = parse_list(Token::Tag::R_Paren, Token::Tag::Comma, [&] { return parse_def(); });
    expect(Token::Tag::Colon);
    auto type = parse_def();

    return world_.tuple(defs, type, anchor.location());
}

const Axiom* Parser::parse_assume() {
    Anchor anchor(this);
    eat(Token::Tag::L_Brace);

    if (!ahead_.isa(Token::Tag::Literal)) {
        ELOG("literal expected in {}", ahead_.location());
    }

    auto box = ahead_.literal().box;

    eat(Token::Tag::Literal);
    expect(Token::Tag::Colon);

    auto type = parse_def();

    expect(Token::Tag::R_Brace);
    return world_.assume(type, box, anchor.location());
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
        ELOG("'{}' expected in {}", Token::tag_to_string(tag), ahead_.location());
    next();
}

}
