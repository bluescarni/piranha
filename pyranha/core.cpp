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
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "../src/piranha.hpp"

namespace bp = boost::python;
using namespace piranha;

template <typename T>
struct descriptor {};

template <typename T, T N>
struct descriptor<std::integral_constant<T,N>>
{
	static std::string name()
	{
		return "";
	}
};

#define PYRANHA_DECLARE_DESCRIPTOR(type) \
template <> \
struct descriptor<type> \
{ \
	static std::string name() \
	{ \
		return #type; \
	} \
};

#define PYRANHA_DECLARE_T_DESCRIPTOR(t_type) \
template <typename ... Args> \
struct descriptor<t_type<Args...>> \
{ \
	static std::string name() \
	{ \
		return std::string(#t_type) + "<" + iterate<Args...>() + ">"; \
	} \
	template <typename Arg0, typename ... Args2> \
	static std::string iterate() \
	{ \
		const auto tmp1 = descriptor<Arg0>::name(), tmp2 = iterate<Args2...>(); \
		if (tmp2 == "") { \
			return tmp1; \
		} \
		if (tmp1 == "") { \
			return tmp2; \
		} \
		return tmp1 + "," + tmp2; \
	} \
	template <typename ... Args2> \
	static std::string iterate(typename std::enable_if<sizeof...(Args2) == 0>::type * = nullptr) \
	{ \
		return ""; \
	} \
};

PYRANHA_DECLARE_DESCRIPTOR(integer)
PYRANHA_DECLARE_DESCRIPTOR(double)
PYRANHA_DECLARE_DESCRIPTOR(real)
PYRANHA_DECLARE_DESCRIPTOR(rational)
PYRANHA_DECLARE_DESCRIPTOR(signed char)
PYRANHA_DECLARE_DESCRIPTOR(short)
PYRANHA_DECLARE_DESCRIPTOR(int)
PYRANHA_DECLARE_DESCRIPTOR(long)
PYRANHA_DECLARE_DESCRIPTOR(long long)
PYRANHA_DECLARE_T_DESCRIPTOR(polynomial)
PYRANHA_DECLARE_T_DESCRIPTOR(poisson_series)
PYRANHA_DECLARE_T_DESCRIPTOR(kronecker_monomial)

#undef PYRANHA_DECLARE_DESCRIPTOR
#undef PYRANHA_DECLARE_T_DESCRIPTOR

// Archive of series type names.
static std::map<std::string,std::size_t> series_archive;
// Counter to be incremented each time a series is exposed.
static std::size_t series_counter = 0u;

// NOTE: these headers are not meant to be used anywhere else, they are just being
// used to group together common functionality and not oversize core.cpp.

#include "python_converters.hpp"
#include "exceptions.hpp"
#include "exposer.hpp"

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

static inline bp::list get_series_list()
{
	bp::list retval;
	for (auto &p: series_archive) {
		retval.append(bp::make_tuple(p.first,p.second));
	}
	return retval;
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
	// Series list.
	bp::def("_get_series_list",get_series_list);
	// Descriptor for polynomial exposition.
	struct poly_desc
	{
		using params = std::tuple<std::tuple<double,signed char>,std::tuple<double,short>,std::tuple<double,kronecker_monomial<>>,
			std::tuple<integer,signed char>,std::tuple<integer,short>,std::tuple<integer,kronecker_monomial<>>,
			std::tuple<rational,signed char>,std::tuple<rational,short>,std::tuple<rational,kronecker_monomial<>>,
			std::tuple<real,signed char>,std::tuple<real,short>,std::tuple<real,kronecker_monomial<>>>;
		using interop_types = std::tuple<double,integer,real,rational>;
		using pow_types = std::tuple<double,integer>;
		using eval_types = interop_types;
		using subs_types = interop_types;
		// Need to refer to these to silence a warning in GCC.
		interop_types	it;
		pow_types	pt;
		eval_types	et;
		subs_types	st;
	};
	exposer<polynomial,poly_desc> poly_exposer;
	struct ps_desc
	{
		using params = std::tuple<std::tuple<double>,std::tuple<rational>,std::tuple<real>,
			std::tuple<polynomial<double,signed char>>,std::tuple<polynomial<double,short>>,std::tuple<polynomial<double,kronecker_monomial<>>>,
			std::tuple<polynomial<rational,signed char>>,std::tuple<polynomial<rational,short>>,std::tuple<polynomial<rational,kronecker_monomial<>>>,
			std::tuple<polynomial<real,signed char>>,std::tuple<polynomial<real,short>>,std::tuple<polynomial<real,kronecker_monomial<>>>>;
		using interop_types = std::tuple<double,rational,integer,real>;
		using pow_types = std::tuple<double,integer>;
		using eval_types = std::tuple<double,real,rational>;
		using subs_types = eval_types;
		interop_types	it;
		pow_types	pt;
		eval_types	et;
		subs_types	st;
	};
	exposer<poisson_series,ps_desc> ps_exposer;
	// Expose the settings class.
	bp::class_<settings> settings_class("_settings",bp::init<>());
	settings_class.def("_get_max_term_output",settings::get_max_term_output).staticmethod("_get_max_term_output");
	settings_class.def("_set_max_term_output",settings::set_max_term_output).staticmethod("_set_max_term_output");
	settings_class.def("_set_n_threads",settings::set_n_threads).staticmethod("_set_n_threads");
	settings_class.def("_get_n_threads",settings::get_n_threads).staticmethod("_get_n_threads");
	settings_class.def("_reset_n_threads",settings::reset_n_threads).staticmethod("_reset_n_threads");
	// Factorial.
	bp::def("_factorial",&math::factorial);
	// Binomial coefficient.
	bp::def("_binomial",&binomial_integer);
	bp::def("_binomial",&binomial_rational);
	// Set the inited flag.
	inited = true;
}
