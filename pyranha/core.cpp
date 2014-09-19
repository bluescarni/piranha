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

#include "python_includes.hpp"

#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/init.hpp>
#include <boost/python/module.hpp>
#include <boost/python/object.hpp>
#include <boost/python/scope.hpp>
#include <mutex>
#include <stdexcept>
#include <string>

#include "../src/exceptions.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/exceptions.hpp"
#include "../src/math.hpp"
#include "exceptions.hpp"
#include "expose_poisson_series.hpp"
#include "expose_polynomials.hpp"
#include "expose_utils.hpp"
#include "python_converters.hpp"
#include "type_system.hpp"

namespace bp = boost::python;

static std::mutex global_mutex;
static bool inited = false;

// Used for debugging on Python side.
inline piranha::integer get_big_int()
{
	return piranha::integer(boost::integer_traits<int>::const_max) + 1;
}

static inline auto binomial_integer(const piranha::integer &n, const piranha::integer &k) -> decltype(piranha::math::binomial(n,k))
{
	if (piranha::math::abs(n) > 10000 || piranha::math::abs(k) > 10000) {
		piranha_throw(std::invalid_argument,"input value is too large");
	}
	return piranha::math::binomial(n,k);
}

static inline auto binomial_rational(const piranha::rational &q, const piranha::integer &k) -> decltype(piranha::math::binomial(q,k))
{
	return piranha::math::binomial(q,k);
}

// Cleanup function to be called on module unload.
static inline void cleanup_type_system()
{
	pyranha::et_map.clear();
}

namespace pyranha
{

// Name the template template classes of interest.
DECLARE_TT_NAMER(piranha::kronecker_monomial,"kronecker_monomial")

}

BOOST_PYTHON_MODULE(_core)
{
	// NOTE: this is a single big lock to avoid registering types/conversions multiple times and prevent contention
	// if the module is loaded from multiple threads.
	std::lock_guard<std::mutex> lock(global_mutex);
	if (inited) {
		return;
	}
	// Set the inited flag.
	// NOTE: we do it here because if something goes wrong in the rest of the init, we do not have a way
	// to roll back the initialisation, and if the user tries again the init probably a lot of things would
	// go haywire. Like this, we will not re-run any init code in a successive attempt at loading the module.
	inited = true;
	// Piranha environment setup.
	piranha::environment env;
	// Docstring options setup.
	bp::docstring_options doc_options(true,true,false);
	// Type generator class.
	bp::class_<pyranha::type_generator> tg_class("_type_generator",bp::no_init);
	tg_class.def("__call__",&pyranha::type_generator::operator());
	tg_class.def("__repr__",&pyranha::type_generator::repr);
	// Generic type generator class.
	bp::class_<pyranha::generic_type_generator> gtg_class("_generic_type_generator",bp::no_init);
	gtg_class.def("__call__",&pyranha::generic_type_generator::operator());
	gtg_class.def("__repr__",&pyranha::generic_type_generator::repr);
	// Create the types submodule.
	std::string types_module_name = bp::extract<std::string>(bp::scope().attr("__name__") + ".types");
	// NOTE: the nested namespace is created if not there, otherwise it will be returned.
	::PyObject *types_module_ptr = ::PyImport_AddModule(types_module_name.c_str());
	if (!types_module_ptr) {
		::PyErr_Clear();
		::PyErr_SetString(PyExc_RuntimeError,"error while creating the 'types' submodule");
		bp::throw_error_already_set();
	}
	// NOTE: apparently there is a difference in behaviour here: Python2 returns a borrowed reference from PyImport_AddModule,
	// Python3 a new one. Check:
	// https://docs.python.org/3/c-api/import.html
	// versus
	// https://docs.python.org/2/c-api/import.html
#if PY_MAJOR_VERSION < 3
	auto types_module = bp::object(bp::handle<>(bp::borrowed(types_module_ptr)));
#else
	auto types_module = bp::object(bp::handle<>(types_module_ptr));
#endif
	bp::scope().attr("types") = types_module;
	// Expose concrete instances of the type generator.
	pyranha::expose_type_generator<signed char>("signed_char");
	pyranha::expose_type_generator<short>("short");
	pyranha::expose_type_generator<float>("float");
	pyranha::expose_type_generator<double>("double");
	pyranha::expose_type_generator<long double>("long_double");
	pyranha::expose_type_generator<piranha::integer>("integer");
	pyranha::expose_type_generator<piranha::rational>("rational");
	pyranha::expose_type_generator<piranha::real>("real");
	pyranha::expose_generic_type_generator<piranha::kronecker_monomial>();
	// Arithmetic converters.
	pyranha::integer_converter i_c;
	pyranha::rational_converter ra_c;
	pyranha::real_converter re_c;
	// Exceptions translation.
	pyranha::generic_translate<&PyExc_ZeroDivisionError,piranha::zero_division_error>();
	pyranha::generic_translate<&PyExc_NotImplementedError,piranha::not_implemented_error>();
	pyranha::generic_translate<&PyExc_OverflowError,std::overflow_error>();
	pyranha::generic_translate<&PyExc_OverflowError,boost::numeric::positive_overflow>();
	pyranha::generic_translate<&PyExc_OverflowError,boost::numeric::negative_overflow>();
	pyranha::generic_translate<&PyExc_OverflowError,boost::numeric::bad_numeric_cast>();
	// Debug functions.
	bp::def("_get_big_int",&get_big_int);
	// Series list.
	bp::def("_get_series_list",pyranha::get_series_list);
	// Expose polynomials.
	pyranha::expose_polynomials();
	// Expose Poisson series.
	pyranha::expose_poisson_series();
	// Expose the settings class.
	bp::class_<piranha::settings> settings_class("_settings",bp::init<>());
	settings_class.def("_get_max_term_output",piranha::settings::get_max_term_output).staticmethod("_get_max_term_output");
	settings_class.def("_set_max_term_output",piranha::settings::set_max_term_output).staticmethod("_set_max_term_output");
	settings_class.def("_set_n_threads",piranha::settings::set_n_threads).staticmethod("_set_n_threads");
	settings_class.def("_get_n_threads",piranha::settings::get_n_threads).staticmethod("_get_n_threads");
	settings_class.def("_reset_n_threads",piranha::settings::reset_n_threads).staticmethod("_reset_n_threads");
	// Factorial.
	bp::def("_factorial",&piranha::math::factorial<0>);
	// Binomial coefficient.
	bp::def("_binomial",&binomial_integer);
	bp::def("_binomial",&binomial_rational);
	// Cleanup function.
	bp::def("_cleanup_type_system",&cleanup_type_system);
}
