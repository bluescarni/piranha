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

#include <boost/lexical_cast.hpp>
#include <exception>
#include <string>

#include "config.hpp" // For piranha_override and visibility.

namespace piranha
{
namespace detail
{

struct thrower
{
	template <typename ExceptionType, typename N>
	static void impl(const char *, N &&, const char *, int &&)
	{
		ExceptionType ex;
		throw ex;
	}
	template <typename ExceptionType, typename N, typename String>
	static void impl(const char *file, N &&line, const char *func, String &&desc)
	{
		std::string msg("\nfunction: ");
		msg += func;
		msg += "\nwhere: ";
		msg += file;
		msg += ", ";
		msg += boost::lexical_cast<std::string>(line);
		msg += "\nwhat: ";
		msg += desc;
		msg += "\n";
		throw ExceptionType(msg);
	}
};

}}

#define piranha_throw(exc_type,...) piranha::detail::thrower::impl<exc_type>(__FILE__,__LINE__,__func__,__VA_ARGS__);throw

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
