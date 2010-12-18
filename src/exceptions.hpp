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

#include <exception>
#include <string>

#define _PIRANHA_EXCEPTION_QUOTEME(x) #x
#define PIRANHA_EXCEPTION_QUOTEME(x) _PIRANHA_EXCEPTION_QUOTEME(x)
#define PIRANHA_EXCEPTION_EXCTOR(s) ((std::string(__FILE__ "," PIRANHA_EXCEPTION_QUOTEME(__LINE__) ": ") + s) + ".")
#define piranha_throw(ex,s) (throw ex(PIRANHA_EXCEPTION_EXCTOR(s)))

namespace piranha
{

/// Base exception class.
/**
 * All piranha exceptions derive from this class.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class base_exception: public std::exception
{
        public:
		explicit base_exception(const std::string &);
		virtual const char *what() const throw();
		virtual ~base_exception() throw();
	private:
		const std::string m_what;
};

/// Exception for functionality not implemented or not available on the current platform.
struct not_implemented_error: public base_exception
{
	explicit not_implemented_error(const std::string &s);
};

/// Exception for invalid values as function arguments.
struct value_error: public base_exception
{
	explicit value_error(const std::string &s);
};

}

#endif
