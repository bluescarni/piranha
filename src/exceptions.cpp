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

#include <exception>
#include <string>

#include "exceptions.hpp"

namespace piranha
{

/// Constructor.
/**
 * The string parameter is an error message that will be stored intenally.
 * 
 * @param[in] s std::string representing an error message.
 */
base_exception::base_exception(const std::string &s):m_what(s) {}

/// Error description.
/**
 * @return const pointer to the internal error message.
 * 
 * \todo c++0x explicit virtual function override once it gets available.
 */
const char *base_exception::what() const throw()
{
	return m_what.c_str();
}

/// Trivial destructor.
base_exception::~base_exception() throw() {}

/// Constructor.
/**
 * @param[in] s std::string representing an error message.
 */
not_implemented_error::not_implemented_error(const std::string &s): base_exception(s) {}

}
