#ifndef THORIN_UTIL_STREAM_H
#define THORIN_UTIL_STREAM_H

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "thorin/util/iterator.h"

namespace thorin {

/// Inherit from this class and implement @p stream in order to use @p streamf.
template<class S>
class Streamable {
public:
    virtual ~Streamable() {}

    virtual S& stream(S&) const = 0;
};

template<class L, class F>
struct stream_list {
    struct stream_list_tag {};

    stream_list(const L& list, F f)
        : list(list)
        , f(f)
    {}

    const L& list;
    F f;
};


template<class T, class = void> struct is_stream_list : std::false_type {};
template<class T> struct is_stream_list<T, std::void_t<typename T::stream_list_tag>> : std::true_type {};

namespace detail {
    template<class S, class T> void stream(S& s, const std::string&, const std::unique_ptr<T>& p) { p->stream(s); }
    template<class S>          void stream(S& s, const std::string&, const Streamable<S>* p) { p->stream(s); }
    template<class S, class T> typename std::enable_if<!is_stream_list<T>::value && !is_ranged<T>::value, void>::type stream(S& s, const std::string&, const T& val) { s << val; }
    template<class S, class T> typename std::enable_if<!is_stream_list<T>::value &&  is_ranged<T>::value, void>::type stream(S& s, const std::string& spec_fmt, const T& list) {
        const char* cur_sep = "";
        for (const auto& elem : list) {
            s << cur_sep;
            stream(s, {""}, elem);
            cur_sep = spec_fmt.c_str();
        }
    }

    template<class S, class T> typename std::enable_if< is_stream_list<T>::value && !is_ranged<T>::value, void>::type stream(S& s, const std::string& spec_fmt, const T& stream_list) {
        const char* cur_sep = "";
        for (const auto& elem : stream_list.list) {
            s << cur_sep;
            stream_list.f(elem);
            cur_sep = spec_fmt.c_str();
        }
    }
}

/// Base case.
template<class S> S& streamf(S& s, const char* fmt) {
    while (*fmt) {
        auto next = fmt + 1;
        if (*fmt == '{') {
            if (*next == '{') {
                s << '{';
                fmt += 2;
                continue;
            }
            while (*fmt && *fmt != '}') fmt++;

            if (*fmt == '}')
                throw std::invalid_argument("invalid format string for 'streamf': missing argument(s); use 'catch throw' in 'gdb'");
            else
                throw std::invalid_argument("invalid format string for 'streamf': missing closing brace and argument; use 'catch throw' in 'gdb'");

        } else if (*fmt == '}') {
            if (*next == '}') {
                s << '}';
                fmt += 2;
                continue;
            }
            // TODO give exact position
            throw std::invalid_argument("nmatched/unescaped closing brace '}' in format string");
        }
        s << *fmt++;
    }
    return s;
}

/**
 * fprintf-like function.
 * Each @c "{}" in @p fmt corresponds to one of the variadic arguments in @p args.
 * The type of the corresponding argument must either support @c operator<< or inherit from @p Streamable.
 * Furthermore, an argument may also be a range-based container where its elements fulfill the condition above.
 * You can specify the separator within the braces:
@code{.cpp}
    streamf(s, "({, })", list) // yields "(a, b, c)"
@endcode
 */
template<class S, class T, class... Args>
S& streamf(S& s, const char* fmt, const T& val, const Args&... args) {
    while (*fmt) {
        auto next = fmt + 1;
        if (*fmt == '{') {
            if (*next == '{') {
                s << '{';
                fmt += 2;
                continue;
            }

            fmt++; // skip opening brace '{'
            std::string spec_fmt;
            while (*fmt != '\0' && *fmt != '}')
                spec_fmt.push_back(*fmt++);
            if (*fmt != '}')
                throw std::invalid_argument("unmatched closing brace '}' in format string");
            detail::stream(s, spec_fmt, val);
            ++fmt;
            return streamf(s, fmt, args...); // call even when *fmt == 0 to detect extra arguments
        } else if (*fmt == '}') {
            if (*next == '}') {
                s << '}';
                fmt += 2;
                continue;
            }
            // TODO give exact position
            throw std::invalid_argument("nmatched/unescaped closing brace '}' in format string");
        } else
            s << *fmt++;
    }
    throw std::invalid_argument("invalid format string for 'streamf': runaway arguments; use 'catch throw' in 'gdb'");
}

template<class S, class... Args> S& streamln(S& s, const char* fmt, Args... args) { return streamf(s, fmt, std::forward<Args>(args)...) << std::endl; }
template<class... Args> std::ostream& outf (const char* fmt, Args... args) { return streamf (std::cout, fmt, std::forward<Args>(args)...); }
template<class... Args> std::ostream& outln(const char* fmt, Args... args) { return streamln(std::cout, fmt, std::forward<Args>(args)...); }
template<class... Args> std::ostream& errf (const char* fmt, Args... args) { return streamf (std::cerr, fmt, std::forward<Args>(args)...); }
template<class... Args> std::ostream& errln(const char* fmt, Args... args) { return streamln(std::cerr, fmt, std::forward<Args>(args)...); }

#ifdef NDEBUG
#   define assertf(condition, ...) //do { (void)sizeof(condition); } while (false)
#else
#   define assertf(condition, ...) do { \
        if (!(condition)) { \
            std::cerr << "Assertion '" #condition "' failed in " << __FILE__ << ":" << __LINE__ << ": "; \
            streamln(std::cerr, __VA_ARGS__); \
            std::abort(); \
        } \
    } while (false)
#endif

template<class P>
class IndentPrinter {
public:
    explicit IndentPrinter(std::ostream& ostream, const char* tab = "    ")
        : ostream_(ostream)
        , tab_(tab)
    {}

    std::ostream& ostream() { return ostream_; };
    const char* tab() const { return tab_; }
    P& indent() { ++level_; return child(); }
    P& dedent() { assert(level_ > 0); --level_; return child(); }
    P& endl() {
        ostream() << std::endl;
        for (int i = 0; i != level_; ++i) ostream() << tab();
        return child();
    }
    template<class T> P& operator<<(const T& val) { ostream() << val; return child(); }

private:
    P& child() { return static_cast<P&>(*this); }

    std::ostream& ostream_;
    const char* tab_;
    int level_ = 0;
};

}

#endif
