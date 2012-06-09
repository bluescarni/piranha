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

// NOTE: the order of inclusion in the first two items here is forced by these two issues:
// http://mail.python.org/pipermail/python-list/2004-March/907592.html
// http://mail.python.org/pipermail/new-bugs-announce/2011-March/010395.html
#if defined(_WIN32)
#include <cmath>
#include <Python.h>
#else
#include <Python.h>
#include <cmath>
#endif

#if PY_MAJOR_VERSION < 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 6)
	#error Minimum supported Python version is 2.6.
#endif

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/python.hpp>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "../src/piranha.hpp"

namespace bp = boost::python;
using namespace piranha;

// NOTE: these headers are not meant to be used anywhere else, they are just being
// used to group together common functionality and not oversize core.cpp.

#include "python_converters.hpp"
#include "exceptions.hpp"
#include "polynomial.hpp"

BOOST_PYTHON_MODULE(_core)
{
	// Arithmetic converters.
	integer_converter ic;
	rational_converter rc;
	// Exceptions translation.
	bp::register_exception_translator<boost::numeric::bad_numeric_cast>(bnc_translator);
	bp::register_exception_translator<boost::numeric::positive_overflow>(po_translator);
	bp::register_exception_translator<boost::numeric::negative_overflow>(no_translator);
	bp::register_exception_translator<std::overflow_error>(oe_translator);
	bp::register_exception_translator<zero_division_error>(zde_translator);
	// Docstring options setup.
	bp::docstring_options doc_options(true,true,false);
	// Expose polynomial types.
	expose_polynomials();
}
