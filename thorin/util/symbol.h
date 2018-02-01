#ifndef THORIN_UTIL_SYMBOL_H
#define THORIN_UTIL_SYMBOL_H

#include <cstring>
#include <string>
#include <algorithm>

#include "thorin/util/hash.h"

namespace thorin {

// TODO use some template magic with constants to disambiguate multiple instances of the set of symbols
class Symbol {
public:
    struct Hash {
        static uint64_t hash(thorin::Symbol s) { return thorin::hash(s.str()); }
        static bool eq(thorin::Symbol s1, thorin::Symbol s2) { return s1 == s2; }
        static thorin::Symbol sentinel() { return thorin::Symbol(/*dummy*/23); }
    };

    Symbol() { insert(""); }
    Symbol(const char* str) { insert(str); }
    Symbol(const std::string& str) { insert(str.c_str()); }

    const char* str() const { return str_; }
    operator bool() const { return *this != Symbol(""); }
    bool operator == (Symbol symbol) const { return str() == symbol.str(); }
    bool operator != (Symbol symbol) const { return str() != symbol.str(); }
    bool operator == (const char* s) const { return str() == Symbol(s).str(); }
    bool operator != (const char* s) const { return str() != Symbol(s).str(); }
    bool empty() const { return *str_ == '\0'; }
    bool is_anonymous() { return (*this) == "_"; }
    std::string remove_quotation() const;

    static void destroy();

private:
    Symbol(int /* just a dummy */)
        : str_((const char*)(1))
    {}

    void insert(const char* str);

    const char* str_;
    typedef thorin::HashSet<const char*, thorin::StrHash> Table;
    static Table table_;
};

inline std::ostream& operator << (std::ostream& os, Symbol s) { return os << s.str(); }

}

#endif
