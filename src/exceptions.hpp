/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PIRANHA_EXCEPTIONS_HPP
#define PIRANHA_EXCEPTIONS_HPP

/** \file exceptions.hpp
 * \brief Exceptions.
 *
 * This header contains custom exceptions used within piranha and related utilities.
 */

#include <boost/lexical_cast.hpp>
#include <exception>
#include <string>
#include <type_traits>
#include <utility>

#include "config.hpp" // For piranha_override and visibility.

namespace piranha
{
namespace detail
{

template <typename Exception>
struct ex_thrower
{
	// Determine the type of the __LINE__ macro.
	typedef std::decay<decltype(__LINE__)>::type line_type;
	explicit ex_thrower(const char *file, line_type line, const char *func):m_file(file),m_line(line),m_func(func)
	{}
	template <typename ... Args, typename = typename std::enable_if<std::is_constructible<Exception,Args...>::value>::type>
	void operator()(Args && ... args) const
	{
		Exception e(std::forward<Args>(args)...);
		throw e;
	}
	template <typename Str, typename ... Args, typename = typename std::enable_if<
		std::is_constructible<Exception,std::string,Args...>::value && (
		std::is_same<typename std::decay<Str>::type,std::string>::value ||
		std::is_same<typename std::decay<Str>::type,char *>::value ||
		std::is_same<typename std::decay<Str>::type,const char *>::value)>::type>
	void operator()(Str &&desc, Args && ... args) const
	{
		std::string msg("\nfunction: ");
		msg += m_func;
		msg += "\nwhere: ";
		msg += m_file;
		msg += ", ";
		msg += boost::lexical_cast<std::string>(m_line);
		msg += "\nwhat: ";
		msg += desc;
		msg += "\n";
		throw Exception(msg,std::forward<Args>(args)...);
	}
	const char	*m_file;
	const line_type	m_line;
	const char	*m_func;
};

}}

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
 * then the first argument \p arg0 is interpreted as the error message associated to the exception object, and it will be decorated
 * with information about the context in which the exception was thrown (file, line, function) before being
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
#define piranha_throw(exception_type,...) piranha::detail::ex_thrower<exception_type>(__FILE__,__LINE__,__func__)(__VA_ARGS__);throw

namespace piranha
{

// NOTE: all exception classes must be declared as visible:
// http://gcc.gnu.org/wiki/Visibility

/// Base exception class.
/**
 * All piranha exceptions derive from this class.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class PIRANHA_PUBLIC base_exception: public std::exception
{
        public:
		explicit base_exception(const std::string &);
		virtual const char *what() const throw() piranha_override;
		virtual ~base_exception() throw();
	private:
		const std::string m_what;
};

/// Exception for functionality not implemented or not available on the current platform.
struct PIRANHA_PUBLIC not_implemented_error: public base_exception
{
	explicit not_implemented_error(const std::string &s);
};

/// Exception for signalling division by zero.
struct PIRANHA_PUBLIC zero_division_error: public base_exception
{
	explicit zero_division_error(const std::string &s);
};


}

#endif
