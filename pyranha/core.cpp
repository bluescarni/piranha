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
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../src/detail/is_digit.hpp"
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
PYRANHA_DECLARE_DESCRIPTOR(long)
PYRANHA_DECLARE_DESCRIPTOR(long long)
PYRANHA_DECLARE_T_DESCRIPTOR(polynomial)
PYRANHA_DECLARE_T_DESCRIPTOR(poisson_series)
PYRANHA_DECLARE_T_DESCRIPTOR(kronecker_monomial)

// Series archive, will store the description of exposed series.
static std::map<std::string,std::set<std::vector<std::string>>> series_archive;

// Descriptor for template parameters to be exposed to Python.
template <typename>
struct p_descriptor {};

static inline const std::string &check_name(const std::string &str)
{
	if (str.empty()) {
		piranha_throw(std::runtime_error,"invalid template parameter name: empty string");
	}
	if (str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != std::string::npos) {
		piranha_throw(std::runtime_error,"invalid template parameter name: invalid character detected");
	}
	if (str.front() == '_' || str.back() == '_') {
		piranha_throw(std::runtime_error,"invalid template parameter name: name cannot start or end with underscore");
	}
	if (detail::is_digit(*str.begin())) {
		piranha_throw(std::runtime_error,"invalid template parameter name: name cannot start with a digit ");
	}
	// This will be used as separator.
	if (str.find("___") != std::string::npos) {
		piranha_throw(std::runtime_error,"invalid template parameter name: name cannot contain '___'");
	}
	return str;
}

#define PYRANHA_DECLARE_P_DESCRIPTOR(type,...) \
template <> \
struct p_descriptor<type> \
{ \
	static const std::string name; \
}; \
const std::string p_descriptor<type>::name = check_name(std::string(#__VA_ARGS__) == "" ? #type : #__VA_ARGS__);

PYRANHA_DECLARE_P_DESCRIPTOR(double,)
PYRANHA_DECLARE_P_DESCRIPTOR(integer,)
PYRANHA_DECLARE_P_DESCRIPTOR(rational,)
PYRANHA_DECLARE_P_DESCRIPTOR(real,)
PYRANHA_DECLARE_P_DESCRIPTOR(signed char,signed_char)
PYRANHA_DECLARE_P_DESCRIPTOR(short,)
PYRANHA_DECLARE_P_DESCRIPTOR(kronecker_monomial<>,kronecker)
using p_rat_sc = polynomial<rational,signed char>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_rat_sc,polynomial_rational_signed_char)
using p_rat_short = polynomial<rational,short>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_rat_short,polynomial_rational_short)
using p_double_sc = polynomial<double,signed char>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_double_sc,polynomial_double_signed_char)
using p_double_short = polynomial<double,short>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_double_short,polynomial_double_short)
using p_real_short = polynomial<real,short>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_real_short,polynomial_real_short)
using p_real_sc = polynomial<real,signed char>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_real_sc,polynomial_real_signed_char)
using p_double_k = polynomial<double,kronecker_monomial<>>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_double_k,polynomial_double_kronecker)
using p_real_k = polynomial<real,kronecker_monomial<>>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_real_k,polynomial_real_kronecker)
using p_rational_k = polynomial<rational,kronecker_monomial<>>;
PYRANHA_DECLARE_P_DESCRIPTOR(p_rational_k,polynomial_rational_kronecker)

#undef PYRANHA_DECLARE_P_DESCRIPTOR

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
		bp::list tmp1;
		for (auto &v: p.second) {
			bp::list tmp2;
			for (auto &str: v) {
				tmp2.append(str);
			}
			tmp1.append(tmp2);
		}
		retval.append(bp::make_tuple(p.first,tmp1));
	}
	return retval;
}

BOOST_PYTHON_MODULE(_core)
{
	std::cout << descriptor<integer>::name() << '\n';
	std::cout << descriptor<polynomial<double,signed char>>::name() << '\n';
	std::cout << descriptor<poisson_series<polynomial<double,signed char>>>::name() << '\n';
	std::cout << descriptor<kronecker_monomial<>>::name() << '\n';
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
	//exposer<polynomial,poly_desc> poly_exposer("polynomial");
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
	//exposer<poisson_series,ps_desc> ps_exposer("poisson_series");
/*
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
		std::make_tuple(polynomial<double,signed char>(),std::string("polynomial_double")),
		std::make_tuple(polynomial<integer,signed char>(),std::string("polynomial_integer")),
		std::make_tuple(polynomial<rational,signed char>(),std::string("polynomial_rational")),
		std::make_tuple(polynomial<real,signed char>(),std::string("polynomial_real"))
	);
	auto ps_interop_types = std::make_tuple(double(),integer(),rational(),real());
	series_exposer<poisson_series,decltype(ps_cf_types),decltype(ps_interop_types)>
		pse("poisson_series",ps_cf_types,ps_interop_types);
*/
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
