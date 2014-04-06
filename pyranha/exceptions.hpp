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

#ifndef PYRANHA_EXCEPTIONS_HPP
#define PYRANHA_EXCEPTIONS_HPP

#include "python_includes.hpp"

#include <boost/python/errors.hpp>

namespace pyranha
{

namespace bp = boost::python;

// NOTE: the idea here is to use non-type template parameters to handle generically exceptions.
// The standard Python exceptions are pointers with external linkage and as such they are usable as non-type template
// parameters.
// http://docs.python.org/extending/extending.html#intermezzo-errors-and-exceptions
// http://stackoverflow.com/questions/1655271/explanation-of-pyapi-data-macro
// http://publib.boulder.ibm.com/infocenter/comphelp/v8v101/index.jsp?topic=%2Fcom.ibm.xlcpp8a.doc%2Flanguage%2Fref%2Ftemplate_non-type_arguments.htm

template <PyObject **PyEx,typename CppEx>
inline void generic_translator(const CppEx &cpp_ex)
{
	::PyErr_SetString(*PyEx,cpp_ex.what());
}

template <PyObject **PyEx,typename CppEx>
inline void generic_translate()
{
	bp::register_exception_translator<CppEx>(generic_translator<PyEx,CppEx>);
}

}

#endif
