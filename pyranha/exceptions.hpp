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

#ifndef PIRANHA_PYRANHA_EXCEPTIONS_HPP
#define PIRANHA_PYRANHA_EXCEPTIONS_HPP

#include <boost/python/exception_translator.hpp>

#include "../src/exceptions.hpp"

namespace piranha { namespace pyranha {

// inline void std_invalid_argument_translator(const std::invalid_argument &ia)
// {
// 	PyErr_SetString(PyExc_ValueError,ia.what());
// }

// Translate our C++ exceptions into Python exceptions.
inline void translate_exceptions()
{
// 	boost::python::register_exception_translator<std::invalid_argument>(ie_translator);
	
}

}}

#endif
