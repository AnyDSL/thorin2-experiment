#ifndef THORIN_UTIL_STREAM_H
#define THORIN_UTIL_STREAM_H

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
    ///< Uses @p stream and @c std::ostringstream to generate a @c std::string.
    std::string to_string() const {
        S s;
        return stream(s).str();
    }
    ///< Uses @p stream in order to dump to @p std::cout.
    void dump() const { stream(std::cout) << std::endl; }
};

template<class S>
S& operator<<(S& stream, const Streamable<S>* p) { return p->stream(stream); }

// TODO remove
template<class S, class F, class List>
S& stream_list(S& s, const List& list, F f, const char* sep = ", ") {
    const char* cur_sep = "";
    for (const auto& elem : list) {
        s << cur_sep;
        f(elem);
        cur_sep = sep;
    }
    return s;
}

// TODO remove
template<class S, class F, class List>
S& stream_list(S& s, const List& list, const char* delim_l, const char* delim_r, F f, const char* sep = ", ") {
    return stream_list(s << delim_l, list, f, sep) << delim_r;
}

namespace detail {
    template<class S, class T> void stream(S& s, const std::string&, const std::unique_ptr<T>& p) { p->stream(s); }
    template<class S>          void stream(S& s, const std::string&, const Streamable<S>* p) { p->stream(s); }
    template<class S, class T> typename std::enable_if<!is_ranged<T>::value, void>::type stream(S& s, const std::string&, const T& val) { s << val; }
    template<class S, class T> typename std::enable_if< is_ranged<T>::value, void>::type stream(S& s, const std::string& spec_fmt, const T& lst) {
        const char* cur_sep = "";
        for (auto&& elem : lst) {
            s << cur_sep;
            stream(s, {""}, elem);
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

}

#endif
