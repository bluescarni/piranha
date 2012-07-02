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
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/python.hpp>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/piranha.hpp"

namespace bp = boost::python;
using namespace piranha;

// NOTE: these headers are not meant to be used anywhere else, they are just being
// used to group together common functionality and not oversize core.cpp.

#include "python_converters.hpp"
#include "exceptions.hpp"
#include "series_exposer.hpp"

// Used for debugging on Python side.
inline integer get_big_int()
{
	return integer(boost::integer_traits<int>::const_max) + 1;
}

BOOST_PYTHON_MODULE(_core)
{
	// Arithmetic converters.
	integer_converter i_c;
	rational_converter ra_c;
	real_converter re_c;
	// Exceptions translation.
	bp::register_exception_translator<boost::numeric::bad_numeric_cast>(bnc_translator);
	bp::register_exception_translator<boost::numeric::positive_overflow>(po_translator);
	bp::register_exception_translator<boost::numeric::negative_overflow>(no_translator);
	bp::register_exception_translator<std::overflow_error>(oe_translator);
	bp::register_exception_translator<zero_division_error>(zde_translator);
	// Docstring options setup.
	bp::docstring_options doc_options(true,true,false);
	// Debug functions.
	bp::def("_get_big_int",&get_big_int);
	// Polynomials.
	auto poly_cf_types = std::make_tuple(
		std::make_tuple(double(),std::string("double")),
		std::make_tuple(integer(),std::string("integer")),
		std::make_tuple(rational(),std::string("rational")),
		std::make_tuple(real(),std::string("real"))
	);
	auto poly_interop_types = std::make_tuple(double(),integer(),rational(),real());
	series_exposer<polynomial,decltype(poly_cf_types),decltype(poly_interop_types)>
		pe("polynomial",poly_cf_types,poly_interop_types);
	// Poisson series.
	auto ps_cf_types = std::make_tuple(
		std::make_tuple(double(),std::string("double")),
		std::make_tuple(integer(),std::string("integer")),
		std::make_tuple(rational(),std::string("rational")),
		std::make_tuple(real(),std::string("real")),
		std::make_tuple(polynomial<double>(),std::string("polynomial_double")),
		std::make_tuple(polynomial<integer>(),std::string("polynomial_integer")),
		std::make_tuple(polynomial<rational>(),std::string("polynomial_rational")),
		std::make_tuple(polynomial<real>(),std::string("polynomial_real"))
	);
	auto ps_interop_types = std::make_tuple(double(),integer(),rational(),real());
	series_exposer<poisson_series,decltype(ps_cf_types),decltype(ps_interop_types)>
		pse("poisson_series",ps_cf_types,ps_interop_types);
}
