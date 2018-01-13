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

/** \file exceptions.hpp
 * \brief Exceptions.
 *
 * This header contains custom exceptions used within piranha and related utilities.
 */

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <piranha/type_traits.hpp>

namespace piranha
{
inline namespace impl
{

template <typename Exception>
struct ex_thrower {
    // Determine the type of the __LINE__ macro.
    using line_type = uncvref_t<decltype(__LINE__)>;
    explicit ex_thrower(const char *file, line_type line, const char *func) : m_file(file), m_line(line), m_func(func)
    {
    }
    template <typename... Args, typename = enable_if_t<std::is_constructible<Exception, Args &&...>::value>>
    [[noreturn]] void operator()(Args &&... args) const
    {
        throw Exception(std::forward<Args>(args)...);
    }
    template <typename Str>
    using str_add_t = decltype(std::declval<std::string &>() += std::declval<Str>());
    template <typename Str, typename... Args,
              typename = enable_if_t<conjunction<std::is_constructible<Exception, std::string, Args &&...>,
                                                 is_detected<str_add_t, Str &&>>::value>>
    [[noreturn]] void operator()(Str &&desc, Args &&... args) const
    {
        std::string msg("\nfunction: ");
        msg += m_func;
        msg += "\nwhere: ";
        msg += m_file;
        msg += ", ";
        msg += std::to_string(m_line);
        msg += "\nwhat: ";
        msg += std::forward<Str>(desc);
        msg += "\n";
        throw Exception(msg, std::forward<Args>(args)...);
    }
    const char *m_file;
    const line_type m_line;
    const char *m_func;
};
}
}

/// Exception-throwing macro.
/**
 * By default, this variadic macro will throw an exception of type \p exception_type, using the variadic
 * arguments for the construction of the exception object. The macro will check if the exception can be constructed
 * from the variadic arguments, and will produce a compilation error in case no suitable constructor is found.
 *
 * Additionally, given a set of variadic arguments <tt>[arg0,arg1,...]</tt>, and
 * - if the first variadic argument \p arg0 can be concatenated to an \p std::string via the <tt>+=</tt> operator,
 * - and if the exception can be constructed from the set of arguments <tt>[str,arg1,...]</tt>,
 *   where \p str is an instance of \p std::string,
 *
 * then the first argument \p arg0 is interpreted as the error message associated to the exception object, and it will
 * be decorated with information about the context in which the exception was thrown (file, line, function) before being
 * passed on for construction.
 *
 * Note that, in order to be fully standard-compliant, for use with exceptions that take no arguments on construction
 * the invocation must include a closing comma. E.g.,
 * @code
 * piranha_throw(std::bad_alloc);
 * @endcode
 * is not correct, whereas
 * @code
 * piranha_throw(std::bad_alloc,);
 * @endcode
 * is correct.
 *
 * @param exception_type the type of the exception to be thrown.
 */
// NOTE: we need the struct here because we need to split off the __VA_ARGS__ into a separate function call, otherwise
// there could be situations in which the throwing function would be called with a set of arguments (a,b,c,), which
// would be invalid syntax.
#define piranha_throw(exception_type, ...)                                                                             \
    piranha::ex_thrower<exception_type>(__FILE__, __LINE__, __func__)(__VA_ARGS__)

namespace piranha
{

/// Exception for functionality not implemented or not available on the current platform.
/**
 * This class inherits the constructors from \p std::runtime_error.
 */
struct not_implemented_error final : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Exception for signalling division by zero.
/**
 * This class inherits the constructors from \p std::domain_error.
 */
struct zero_division_error final : std::domain_error {
    using std::domain_error::domain_error;
};
}

#endif
