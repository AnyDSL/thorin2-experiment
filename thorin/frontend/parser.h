#ifndef PARSER_H
#define PARSER_H

#include <type_traits>
#include <unordered_map>

#include "thorin/def.h"
#include "thorin/world.h"
#include "thorin/frontend/token.h"
#include "thorin/frontend/lexer.h"

namespace thorin {

// TODO use thorin::HashMap
typedef std::unordered_map<std::string, const Def*> Env;

class Parser {
public:
    Parser(World& world, Lexer& lexer, Env& env)
        : world_(world)
        , lexer_(lexer)
        , env_(env)
    {
        ahead_[0] = lexer_.lex();
        ahead_[1] = lexer_.lex();
    }

    const Def*    parse_def();

private:
    struct Tracker {
        Tracker(const Parser* parser)
            : parser(*parser)
            , line(parser->ahead_[0].location().front_line())
            , col(parser->ahead_[0].location().front_col())
        {}

        Location location() const {
            auto back_line = parser.ahead_[0].location().back_line();
            auto back_col  = parser.ahead_[0].location().back_col();
            return Location(parser.lexer_.filename(), line, col, back_line, back_col);
        }

        const Parser& parser;
        uint32_t line;
        uint32_t col;
    };

    const Def*    parse_var_or_binder();
    const Pi*     parse_pi();
    const Def*    parse_sigma_or_variadic();
    const Def*    parse_lambda();
    const Star*   parse_star();
    const Def*    parse_tuple_or_pack();
    const Axiom*  parse_assume();
    const Def*    parse_param();
    const Def*    parse_extract_or_insert(Tracker, const Def*);

    struct Binder {
        std::string name;
        size_t depth;               // binding depth
        std::vector<size_t> ids;    // sequence of extract indices

        Binder(std::string name = "", size_t depth = 0)
            : name(name), depth(depth)
        {}
    };

    DefVector parse_list(Token::Tag end, Token::Tag sep, std::function<const Def*()> f, const char* context, const Def* first = nullptr) {
        DefVector elems;

        if (first != nullptr)
            elems.emplace_back(first);

        if (ahead_[0].isa(end)) {
            eat(end);
            return elems;
        }

        if (first != nullptr)
            eat(sep);

        elems.emplace_back(f());
        while (ahead_[0].isa(sep)) {
            eat(sep);
            elems.emplace_back(f());
        }
        expect(end, context);
        return elems;
    }

    Token next();
    const Token& ahead(int i = 0) { return ahead_[i]; }
    void eat(Token::Tag);
    void expect(Token::Tag, const char* context);
    bool accept(Token::Tag);
    void push_identifiers(const Def* bruijn) {
        depth_++;
        bruijn_.push_back(bruijn);
    }
    void pop_identifiers() {
        while (!binders_.empty()) {
            if (binders_.back().depth < depth_) break;
            binders_.pop_back();
        }
        bruijn_.pop_back();
        depth_--;
    }
    void shift_identifiers(size_t count) {
        // shift identifiers declared within a sigma by adding extracts
        std::vector<Binder> shifted;
        size_t new_depth = depth_ - count;
        while (!binders_.empty()) {
            auto& binder = binders_.back();
            if (binder.depth < new_depth) break;
            Binder new_binder;
            new_binder.ids.push_back(binder.depth - new_depth);
            new_binder.ids.insert(new_binder.ids.end(), binder.ids.begin(), binder.ids.end());
            new_binder.depth = new_depth;
            new_binder.name  = binder.name;
            shifted.emplace_back(std::move(new_binder));
            binders_.pop_back();
        }
        binders_.insert(binders_.end(), shifted.begin(), shifted.end());
        bruijn_.resize(bruijn_.size() - count);
        depth_ = new_depth;
    }

    World& world_;
    Lexer& lexer_;
    Env env_;

    std::vector<const Def*> bruijn_;
    std::vector<Binder> binders_;
    Token ahead_[2];
    size_t depth_ = 0;
};

const Def* parse(World& world, const std::string& str, Env env = {});

}

#endif
