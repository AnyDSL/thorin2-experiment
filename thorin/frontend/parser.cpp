#include "parser.h"

namespace thorin {

const Def* Parser::parse_def() {
    if (ahead_.isa(Token::Tag::Pi))     return parse_pi();
    if (ahead_.isa(Token::Tag::Sigma))  return parse_sigma();
    if (ahead_.isa(Token::Tag::Lambda)) return parse_lambda();
    if (ahead_.isa(Token::Tag::Star))   return parse_star();
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
    Anchor anchor(this);
    eat(Token::Tag::Star);
    return world_.star();
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
