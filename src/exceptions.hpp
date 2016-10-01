/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

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

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

namespace piranha
{
namespace detail
{

template <typename Exception>
struct ex_thrower {
    // Determine the type of the __LINE__ macro.
    using line_type = std::decay<decltype(__LINE__)>::type;
    explicit ex_thrower(const char *file, line_type line, const char *func) : m_file(file), m_line(line), m_func(func)
    {
    }
    template <typename... Args,
              typename = typename std::enable_if<std::is_constructible<Exception, Args...>::value>::type>
    [[noreturn]] void operator()(Args &&... args) const
    {
        throw Exception(std::forward<Args>(args)...);
    }
    template <
        typename Str, typename... Args,
        typename
        = typename std::enable_if<std::is_constructible<Exception, std::string, Args...>::value
                                  && (std::is_same<typename std::decay<Str>::type, std::string>::value
                                      || std::is_same<typename std::decay<Str>::type, char *>::value
                                      || std::is_same<typename std::decay<Str>::type, const char *>::value)>::type>
    [[noreturn]] void operator()(Str &&desc, Args &&... args) const
    {
        std::string msg("\nfunction: ");
        msg += m_func;
        msg += "\nwhere: ";
        msg += m_file;
        msg += ", ";
        msg += std::to_string(m_line);
        msg += "\nwhat: ";
        msg += desc;
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
 *
 * - if the first variadic argument \p arg0 is a string type (either C or C++),
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
 */
// NOTE: we need the struct here because we need to split off the __VA_ARGS__ into a separate function call, otherwise
// there could be situations in which the throwing function would be called with a set of arguments (a,b,c,), which
// would be invalid syntax.
#define piranha_throw(exception_type, ...)                                                                             \
    piranha::detail::ex_thrower<exception_type>(__FILE__, __LINE__, __func__)(__VA_ARGS__)

namespace piranha
{

/// Base exception class.
/**
 * All piranha exceptions derive from this class.
 */
class base_exception : public std::exception
{
public:
    /// Constructor.
    /**
     * The string parameter is an error message that will be stored intenally.
     *
     * @param[in] s std::string representing an error message.
     *
     * @throws unspecified any exception thrown by the copy constructor of \p std::string.
     */
    explicit base_exception(const std::string &s) : m_what(s)
    {
    }
    /// Defaulted copy constructor.
    base_exception(const base_exception &) = default;
    /// Defaulted move constructor.
    base_exception(base_exception &&) = default;
    /// Error description.
    /**
     * @return const pointer to the internal error message.
     */
    virtual const char *what() const noexcept override final
    {
        return m_what.c_str();
    }
    /// Trivial destructor.
    virtual ~base_exception()
    {
    }

private:
    const std::string m_what;
};

/// Exception for functionality not implemented or not available on the current platform.
struct not_implemented_error : public base_exception {
    /// Constructor.
    /**
     * @param[in] s std::string representing an error message.
     *
     * @throws unspecified any exception thrown by the constructor from string of piranha::base_exception.
     */
    explicit not_implemented_error(const std::string &s) : base_exception(s)
    {
    }
};

/// Exception for signalling division by zero.
struct zero_division_error : public base_exception {
    /// Constructor.
    /**
     * @param[in] s std::string representing an error message.
     *
     * @throws unspecified any exception thrown by the constructor from string of piranha::base_exception.
     */
    explicit zero_division_error(const std::string &s) : base_exception(s)
    {
    }
};
}

#endif
