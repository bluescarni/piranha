/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_EXCEPTIONS_HPP
#define PIRANHA_EXCEPTIONS_HPP

#include <piranha/config.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
#include <atomic>
#include <sstream>
#if PIRANHA_CPLUSPLUS >= 201703L
#include <optional>
#endif
#endif

#if defined(PIRANHA_WITH_BOOST_STACKTRACE) && PIRANHA_CPLUSPLUS < 201703L
#include <boost/optional.hpp>
#endif

#include <piranha/detail/init.hpp>
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
#include <piranha/detail/stacktrace.hpp>
#endif
#include <piranha/type_traits.hpp>

namespace piranha
{
inline namespace impl
{

#if defined(PIRANHA_WITH_BOOST_STACKTRACE)

template <typename = void>
struct stacktrace_statics {
    // Global flag to enable/disable the generation of stacktraces.
    static std::atomic<bool> enabled;
};

// On startup, stacktraces are disabled.
template <typename T>
std::atomic<bool> stacktrace_statics<T>::enabled = ATOMIC_VAR_INIT(false);

// We will represent the stacktrace with a boost/std optional, as
// the generation of stacktraces is disabled by default at runtime
// due to cpu cost.
using optional_st_t =
#if PIRANHA_CPLUSPLUS >= 201703L
    std::
#else
    boost::
#endif
        optional<stacktrace>;

#endif

template <typename Exception>
struct ex_thrower {
    // Determine the type of the __LINE__ macro.
    using line_type = uncvref_t<decltype(__LINE__)>;
    // The non-decorating version of the call operator.
    template <typename... Args, enable_if_t<std::is_constructible<Exception, Args &&...>::value, int> = 0>
    [[noreturn]] void operator()(Args &&... args) const
    {
        throw Exception(std::forward<Args>(args)...);
    }
    // The decorating version of the call operator. This is preferred to the above wrt overload resolution
    // if there is at least one argument (the previous overload is "more" variadic), but it is disabled
    // if Str is not a string type or the construction of the decorated exception is not possible.
    template <typename Str, typename... Args,
              enable_if_t<conjunction<is_string_type<uncvref_t<Str>>,
                                      std::is_constructible<Exception, std::string, Args &&...>>::value,
                          int> = 0>
    [[noreturn]] void operator()(Str &&desc, Args &&... args) const
    {
        std::ostringstream oss;
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
        if (m_st) {
            stream_stacktrace(oss, *m_st);
        } else {
#endif
            // This is what is printed if stacktraces are not available/disabled.
            oss << "\nFunction name    : " << m_func;
            oss << "\nLocation         : " << m_file << ", line " << m_line;
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
        }
#endif
        oss << "\nException message: " << std::forward<Str>(desc) << "\n";
        throw Exception(oss.str(), std::forward<Args>(args)...);
    }
    const char *m_file;
    const line_type m_line;
    const char *m_func;
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
    optional_st_t m_st;
#endif
};
}
}

// Exception-throwing macro.
/**
 * By default, this variadic macro will throw an exception of type exception_type, using the variadic
 * arguments for the construction of the exception object. The macro will check if the exception can be constructed
 * from the variadic arguments, and will produce a compilation error in case no suitable constructor is found.
 *
 * Additionally, given a set of variadic arguments [arg0,arg1,...], and
 * - if the first variadic argument arg0 is some string-like type (see piranha::is_string_type),
 * - and if the exception can be constructed from the set of arguments [str,arg1,...],
 *   where str is a std::string,
 *
 * then the first argument arg0 is interpreted as the error message associated to the exception object, and it will
 * be decorated with information about the context in which the exception was thrown (file, line, function - or a
 * stacktrace if possible) before being passed on for construction.
 *
 * Note that, in order to be fully standard-compliant, for use with exceptions that take no arguments on construction
 * the invocation must include a closing comma.
 */
// NOTE: we need the struct here because we need to split off the __VA_ARGS__ into a separate function call, otherwise
// there could be situations in which the throwing function would be called with a set of arguments (a,b,c,), which
// would be invalid syntax.
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
#define piranha_throw(exception_type, ...)                                                                             \
    (piranha::ex_thrower<exception_type>{__FILE__, __LINE__, __func__,                                                 \
                                         piranha::stacktrace_statics<>::enabled.load(std::memory_order_relaxed)        \
                                             ? piranha::optional_st_t(piranha::stacktrace{})                           \
                                             : piranha::optional_st_t{}}(__VA_ARGS__))
#else
#define piranha_throw(exception_type, ...)                                                                             \
    (piranha::ex_thrower<exception_type>{__FILE__, __LINE__, __func__}(__VA_ARGS__))
#endif

namespace piranha
{

/// Exception for functionality not implemented or not available on the current platform.
/**
 * This class inherits the constructors from \p std::runtime_error.
 */
struct not_implemented_error final : std::runtime_error {
    using std::runtime_error::runtime_error;
};
}

#endif
