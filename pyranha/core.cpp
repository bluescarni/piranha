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
#include <boost/python/stl_iterator.hpp>
#include <cstddef>
#include <iterator>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
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

static std::mutex global_mutex;
static bool inited = false;

// Used for debugging on Python side.
inline integer get_big_int()
{
	return integer(boost::integer_traits<int>::const_max) + 1;
}

static inline auto binomial_integer(const integer &n, const integer &k) -> decltype(math::binomial(n,k))
{
	if (math::abs(n) > 10000 || math::abs(k) > 10000) {
		piranha_throw(std::invalid_argument,"input value is too large");
	}
	return math::binomial(n,k);
}

static inline auto binomial_rational(const rational &q, const integer &k) -> decltype(math::binomial(q,k))
{
	return math::binomial(q,k);
}

BOOST_PYTHON_MODULE(_core)
{
	// NOTE: this is a single big lock to avoid registering types/conversions multiple times and prevent contention
	// if the module is loaded from multiple threads.
	std::lock_guard<std::mutex> lock(global_mutex);
	if (inited) {
		return;
	}
	// Piranha environment setup.
	environment env;
	// Arithmetic converters.
	integer_converter i_c;
	rational_converter ra_c;
	real_converter re_c;
	// Exceptions translation.
	generic_translate<&PyExc_ZeroDivisionError,zero_division_error>();
	generic_translate<&PyExc_NotImplementedError,not_implemented_error>();
	generic_translate<&PyExc_OverflowError,std::overflow_error>();
	generic_translate<&PyExc_OverflowError,boost::numeric::positive_overflow>();
	generic_translate<&PyExc_OverflowError,boost::numeric::negative_overflow>();
	generic_translate<&PyExc_OverflowError,boost::numeric::bad_numeric_cast>();
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
	// Expose the settings class.
	bp::class_<settings> settings_class("_settings",bp::init<>());
	settings_class.def("_get_max_term_output",settings::get_max_term_output).staticmethod("_get_max_term_output");
	settings_class.def("_set_max_term_output",settings::set_max_term_output).staticmethod("_set_max_term_output");
	// Factorial.
	bp::def("_factorial",&math::factorial);
	// Binomial coefficient.
	bp::def("_binomial",&binomial_integer);
	bp::def("_binomial",&binomial_rational);
	// Set the inited flag.
	inited = true;
}
