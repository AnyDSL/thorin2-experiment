#ifndef PARSER_H
#define PARSER_H

#include <algorithm>
#include <variant>

#include "thorin/def.h"
#include "thorin/world.h"
#include "thorin/frontend/token.h"
#include "thorin/frontend/lexer.h"
#include "thorin/util/iterator.h"

namespace thorin {

class Parser {
public:
    Parser(World& world, Lexer& lexer)
        : world_(world)
        , lexer_(lexer)
    {
        ahead_[0] = lexer_.lex();
        ahead_[1] = lexer_.lex();
    }

    const Def* parse_def();

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

    void parse_curried_defs(std::vector<std::pair<const Def*, Location>>&);
    const Def* parse_debruijn();
    const Def* parse_cn_type();
    const Def* parse_pi();
    const Def* parse_sigma_or_variadic();
    const Def* parse_lambda();
    const Def* parse_optional_qualifier();
    const Def* parse_qualified_kind();
    const Def* parse_tuple_or_pack();
    const Def* parse_lit();
    const Def* parse_param();
    const Def* parse_extract_or_insert(Tracker, const Def*);
    const Def* parse_literal();
    const Def* parse_identifier();
    std::vector<const Def*> parse_sigma_ops();
    DefArray parse_lambda_ops();

    DefVector parse_list(Token::Tag end, Token::Tag sep, std::function<const Def*()> f, const char* context, const Def* first = nullptr) {
        DefVector elems;

        if (first != nullptr)
            elems.emplace_back(first);

        if (ahead().isa(end)) {
            eat(end);
            return elems;
        }

        if (first != nullptr)
            eat(sep);

        elems.emplace_back(f());
        while (ahead().isa(sep)) {
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


    typedef std::variant<const Def*, size_t> DefOrBinder;
    struct Binder {
        Symbol name;
        size_t depth;               // binding depth
        std::vector<size_t> indices;    // sequence of extract indices
        DefOrBinder shadow;

        Binder(Symbol name, size_t depth, DefOrBinder shadow)
            : name(name), depth(depth), shadow(shadow)
        {}
    };
    struct Declaration {
        Symbol name;
        const Def* def;
        DefOrBinder shadow;

        Declaration(Symbol name, const Def* def, DefOrBinder shadow)
            : name(name), def(def), shadow(shadow)
        {}
    };

    void push_debruijn_type(const Def* type);
    void pop_debruijn_binders();
    void shift_binders(size_t count);
    DefOrBinder lookup(const Tracker&, Symbol);
    void insert_identifier(Symbol, const Def* = nullptr);
    void push_decl_scope();
    void pop_decl_scope();

    World& world_;
    Lexer& lexer_;

    Token ahead_[2];

    // manage a stack of scopes for lets, nominals, variables and debruijn
    thorin::HashMap<Symbol, DefOrBinder, Symbol::Hash> id2defbinder_;
    std::vector<Declaration> declarations_;
    std::vector<size_t> scopes_;

    std::vector<const Def*> debruijn_types_;
    std::vector<Binder> binders_;
    size_t depth_ = 0;
};

const Def* parse(World& world, const char* str);

}

#endif
