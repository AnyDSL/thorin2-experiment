#ifndef THORIN_UTIL_STREAM_H
#define THORIN_UTIL_STREAM_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

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

namespace detail {
    template<class S, class T> auto stream(S& s, const T& val) -> decltype(s << val) { return s << val; }
    template<class S, class T> S& stream(S& s, const std::unique_ptr<T>& p) { return p->stream(s); }
    template<class S>          S& stream(S& s, const Streamable<S>* p) { return p->stream(s); }

    template<class S, class T>
    const char* handle_fmt_specifier(S& s, const char* fmt, const T& val) {
        fmt++; // skip opening brace {
        char specifier = *fmt;
        std::string spec_fmt;
        while (*fmt && *fmt != '}') {
            spec_fmt.push_back(*fmt++);
        }
        if (*fmt != '}')
            throw std::invalid_argument("unmatched closing brace '}' in format string");
        if (specifier == '}')
            detail::stream(s, val);
        // TODO possibly handle some format specifiers here that don't require major template trickery (e.g. floats)
        return ++fmt;
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
            while (*fmt && *fmt != '}') {
                fmt++;
            }
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
 * fprintf-like function which works on C++ @c S.
 * Each @c "{}" in @p fmt corresponds to one of the variadic arguments in @p args.
 * The type of the corresponding argument must either support @c operator<< for C++ @c S or inherit from @p Streamable.
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
            fmt = detail::handle_fmt_specifier(s, fmt, val);
            // call even when *fmt == 0 to detect extra arguments
            return streamf(s, fmt, args...);
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

template<class S, class F, class List>
S& stream_list(S& s, const List& list, const char* delim_l, const char* delim_r, F f, const char* sep = ", ") {
    return stream_list(s << delim_l, list, f, sep) << delim_r;
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
