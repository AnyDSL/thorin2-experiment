#ifndef PARSER_H
#define PARSER_H

#include <type_traits>
#include <unordered_map>

#include "thorin/def.h"
#include "thorin/world.h"
#include "thorin/frontend/token.h"
#include "thorin/frontend/lexer.h"

namespace thorin {

class Parser {
public:
    Parser(WorldBase& world, Lexer& lexer)
        : world_(world)
        , lexer_(lexer)
        , ahead_(lexer.next())
        , depth_(0)
    {}

    const Def*    parse_def();
    const Pi*     parse_pi();
    const Def*    parse_sigma();
    const Def*    parse_lambda();
    const Star*   parse_star();
    const Var*    parse_var();
    const Def*    parse_tuple();
    const Axiom*  parse_assume();
    const Def*    parse_param();

private:
    struct Tracker {
        Tracker(const Parser* parser)
            : parser(*parser)
            , line(parser->ahead_.location().front_line())
            , col(parser->ahead_.location().front_col())
        {}

        Location location() const {
            auto back_line = parser.ahead_.location().back_line();
            auto back_col  = parser.ahead_.location().back_col();
            return Location(parser.lexer_.filename().c_str(), line, col, back_line, back_col);
        }

        const Parser& parser;
        uint32_t line;
        uint32_t col;
    };

    template <typename F>
    std::vector<typename std::result_of<F()>::type>
    parse_list(Token::Tag end, Token::Tag sep, F f) {
        typedef typename std::result_of<F()>::type T;
        if (ahead_.isa(end)) {
            eat(end);
            return std::vector<T>();
        }

        std::vector<T> elems;
        elems.emplace_back(f());
        while (ahead_.isa(sep)) {
            eat(sep);
            elems.emplace_back(f());
        }
        expect(end);
        return elems;
    }

    void next();
    void eat(Token::Tag);
    void expect(Token::Tag);
    bool accept(Token::Tag);
    void pop_identifiers() {
        while (!id_stack_.empty()) {
            std::string ident;
            size_t index;
            std::tie(ident, std::ignore, index) = id_stack_.back();
            if (index < depth_) break;
            id_stack_.pop_back();
            id_map_.erase(ident);
        }
    }
    const Def* shift_def(const Def*, size_t);

    std::unordered_map<std::string, size_t> id_map_;
    std::vector<std::tuple<std::string, const Def*, size_t>> id_stack_;

    WorldBase& world_;
    Lexer&     lexer_;
    Token      ahead_;
    size_t     depth_;
};

const Def* parse(WorldBase& world, const std::string& str);

}

#endif
