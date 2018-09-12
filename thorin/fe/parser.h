#ifndef THORIN_FE_PARSER_H
#define THORIN_FE_PARSER_H

#include <algorithm>

#include "thorin/def.h"
#include "thorin/world.h"
#include "thorin/fe/token.h"
#include "thorin/fe/lexer.h"
#include "thorin/util/iterator.h"
#include "thorin/util/utility.h"

namespace thorin::fe {

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
            , line(parser->ahead_[0].loc().front_line())
            , col(parser->ahead_[0].loc().front_col())
        {}

        Loc loc() const {
            auto back_line = parser.ahead_[0].loc().back_line();
            auto back_col  = parser.ahead_[0].loc().back_col();
            return {parser.lexer_.filename(), line, col, back_line, back_col};
        }

        const Parser& parser;
        uint32_t line;
        uint32_t col;
    };

    void parse_curried_defs(std::vector<std::pair<const Def*, Loc>>&);
    const Def* parse_debruijn();
    const Def* parse_cn();
    const Def* parse_pi();
    const Def* parse_sigma();
    const Def* parse_variadic();
    const Def* parse_lambda();
    const Def* parse_optional_qualifier();
    const Def* parse_qualified_kind();
    const Def* parse_tuple_or_pack();
    const Def* parse_variant();
    const Def* parse_param();
    const Def* parse_extract_or_insert(Tracker, const Def*);
    const Def* parse_literal();
    const Def* parse_identifier();
    std::vector<const Def*> parse_sigma_ops();

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
    Token eat(Token::Tag);
    Token expect(Token::Tag, const char* context);
    bool accept(Token::Tag);


    class DefOrIndex {
    public:
        DefOrIndex() {}
        DefOrIndex(const Def* def)
            : data_(def, 0)
        {}
        DefOrIndex(size_t binder)
            : data_((const Def*)-1, binder)
        {}

        bool is_def() const { return data_.ptr() != (const Def*)-1; }
        bool is_index() const { return !is_def(); }
        const Def* def() const { assert(is_def()); return data_.ptr(); }
        size_t index() const { assert(is_index()); return data_.index(); }

    private:
        TaggedPtr<const Def, size_t> data_;
    };

    struct Declaration {
        Symbol name;
        DefOrIndex shadow;

        Declaration(Symbol name, DefOrIndex shadow)
            : name(name), shadow(shadow)
        {}
    };
    struct Binder : Declaration {
        size_t depth;                // binding depth
        std::vector<size_t> indices; // sequence of extract indices

        Binder(Symbol name, size_t depth, DefOrIndex shadow)
            : Declaration(name, shadow), depth(depth)
        {}
    };
    struct Definition : Declaration {
        const Def* def;

        Definition(Symbol name, const Def* def, DefOrIndex shadow)
            : Declaration(name, shadow), def(def)
        {}
    };

    void push_debruijn_type(const Def* type);
    void pop_debruijn_binders();
    void shift_binders(size_t count);
    DefOrIndex lookup(const Tracker&, Symbol);
    void insert_identifier(Symbol, const Def* = nullptr);
    void push_scope();
    void pop_scope();
    template<typename... Args>
    [[noreturn]] void error(Loc loc, const char* fmt, Args... args) {
        std::ostringstream oss;
        streamf(oss, "{}: parse error: ", loc);
        streamf(oss, fmt, std::forward<Args>(args)...);
        throw std::logic_error(oss.str());
    }
    template<typename... Args>
    [[noreturn]] void error(const char* fmt, Args... args) { error(ahead().loc(), fmt, std::forward<Args>(args)...); }

    World& world_;
    Lexer& lexer_;

    Token ahead_[2];

    // manage a stack of scopes for lets, nominals, variables and debruijn
    thorin::HashMap<Symbol, DefOrIndex, Symbol::Hash> id2def_or_index_;
    std::vector<Definition> definitions_;
    std::vector<size_t> definition_scopes_;

    std::vector<const Def*> debruijn_types_;
    std::vector<Binder> binders_;
    size_t depth_ = 0;
};

const Def* parse(World& world, const char* str);

}

#endif
