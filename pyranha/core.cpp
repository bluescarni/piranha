/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "python_includes.hpp"

#include <boost/numeric/conversion/cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/enum.hpp>
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

#include "../src/binomial.hpp"
#include "../src/divisor.hpp"
#include "../src/exceptions.hpp"
#include "../src/invert.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/series.hpp"
#include "exceptions.hpp"
#include "expose_divisor_series.hpp"
#include "expose_poisson_series.hpp"
#include "expose_polynomials.hpp"
#include "expose_rational_functions.hpp"
#include "expose_utils.hpp"
#include "python_converters.hpp"
#include "type_system.hpp"

namespace bp = boost::python;

static std::mutex global_mutex;
static bool inited = false;

// Cleanup function to be called on module unload.
static inline void cleanup_type_system()
{
	pyranha::et_map.clear();
}

namespace pyranha
{

DECLARE_TT_NAMER(piranha::monomial,"monomial")
DECLARE_TT_NAMER(piranha::divisor,"divisor")

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
		::PyErr_SetString(PyExc_RuntimeError,"error while creating the 'types' submodule");
		bp::throw_error_already_set();
	}
	// NOTE: I think at one point in the past there was a typo in the Python3 C API documentation
	// which hinted at a difference in behaviour for PyImport_AddModule between 2 and 3 (new reference
	// vs borrowed reference). Current documentation states that the reference is always
	// borrowed, both in Python 2 and 3.
	auto types_module = bp::object(bp::handle<>(bp::borrowed(types_module_ptr)));
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
	pyranha::expose_type_generator<piranha::k_monomial>("k_monomial");
	// The generic type generator for monomial instances.
	pyranha::expose_generic_type_generator<piranha::monomial,piranha::rational>();
	pyranha::expose_generic_type_generator<piranha::monomial,short>();
	// The generic type generator for divisor instances.
	pyranha::expose_generic_type_generator<piranha::divisor,short>();
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
	pyranha::generic_translate<&PyExc_ArithmeticError,piranha::math::inexact_division>();
	// Series list.
	bp::def("_get_series_list",pyranha::get_series_list);
	// The enums for save/load.
	bp::enum_<piranha::file_format>("file_format")
		.value("text",piranha::file_format::text)
		.value("binary",piranha::file_format::binary);
	bp::enum_<piranha::file_compression>("file_compression")
		.value("disabled",piranha::file_compression::disabled)
		.value("bzip2",piranha::file_compression::bzip2);
	// Expose polynomials.
	pyranha::expose_polynomials_0();
	pyranha::expose_polynomials_1();
	pyranha::expose_polynomials_2();
	pyranha::expose_polynomials_3();
	pyranha::expose_polynomials_4();
	pyranha::expose_polynomials_5();
	pyranha::expose_polynomials_6();
	pyranha::expose_polynomials_7();
	pyranha::expose_polynomials_8();
	pyranha::expose_polynomials_9();
	pyranha::expose_polynomials_10();
	pyranha::expose_polynomials_11();
	pyranha::expose_polynomials_12();
	pyranha::expose_polynomials_13();
	// Expose Poisson series.
	pyranha::expose_poisson_series_0();
	pyranha::expose_poisson_series_1();
	pyranha::expose_poisson_series_2();
	pyranha::expose_poisson_series_3();
	pyranha::expose_poisson_series_4();
	pyranha::expose_poisson_series_5();
	pyranha::expose_poisson_series_6();
	pyranha::expose_poisson_series_7();
	pyranha::expose_poisson_series_8();
	pyranha::expose_poisson_series_9();
	pyranha::expose_poisson_series_10();
	pyranha::expose_poisson_series_11();
	pyranha::expose_poisson_series_12();
	pyranha::expose_poisson_series_13();
	pyranha::expose_poisson_series_14();
	// Expose divisor series.
	pyranha::expose_divisor_series_0();
	pyranha::expose_divisor_series_1();
	pyranha::expose_divisor_series_2();
	pyranha::expose_divisor_series_3();
	pyranha::expose_divisor_series_4();
	pyranha::expose_divisor_series_5();
	pyranha::expose_divisor_series_6();
	pyranha::expose_divisor_series_7();
	pyranha::expose_divisor_series_8();
	// Expose rational function.
	pyranha::expose_rational_functions_0();
	pyranha::expose_rational_functions_1();
	// Expose the settings class.
	bp::class_<piranha::settings> settings_class("_settings",bp::init<>());
	settings_class.def("_get_max_term_output",piranha::settings::get_max_term_output).staticmethod("_get_max_term_output");
	settings_class.def("_set_max_term_output",piranha::settings::set_max_term_output).staticmethod("_set_max_term_output");
	settings_class.def("_reset_max_term_output",piranha::settings::reset_max_term_output).staticmethod("_reset_max_term_output");
	settings_class.def("_set_n_threads",piranha::settings::set_n_threads).staticmethod("_set_n_threads");
	settings_class.def("_get_n_threads",piranha::settings::get_n_threads).staticmethod("_get_n_threads");
	settings_class.def("_reset_n_threads",piranha::settings::reset_n_threads).staticmethod("_reset_n_threads");
	settings_class.def("_set_min_work_per_thread",piranha::settings::set_min_work_per_thread).staticmethod("_set_min_work_per_thread");
	settings_class.def("_get_min_work_per_thread",piranha::settings::get_min_work_per_thread).staticmethod("_get_min_work_per_thread");
	settings_class.def("_reset_min_work_per_thread",piranha::settings::reset_min_work_per_thread).staticmethod("_reset_min_work_per_thread");
	// Factorial.
	bp::def("_factorial",&piranha::math::factorial<0>);
	// Binomial coefficient.
#define PYRANHA_EXPOSE_BINOMIAL(top,bot) \
bp::def("_binomial",&piranha::math::binomial<top,bot>)
	PYRANHA_EXPOSE_BINOMIAL(double,double);
	PYRANHA_EXPOSE_BINOMIAL(double,piranha::integer);
	PYRANHA_EXPOSE_BINOMIAL(double,piranha::rational);
	PYRANHA_EXPOSE_BINOMIAL(double,piranha::real);
	PYRANHA_EXPOSE_BINOMIAL(piranha::integer,double);
	PYRANHA_EXPOSE_BINOMIAL(piranha::integer,piranha::integer);
	PYRANHA_EXPOSE_BINOMIAL(piranha::integer,piranha::rational);
	PYRANHA_EXPOSE_BINOMIAL(piranha::integer,piranha::real);
	PYRANHA_EXPOSE_BINOMIAL(piranha::rational,double);
	PYRANHA_EXPOSE_BINOMIAL(piranha::rational,piranha::integer);
	PYRANHA_EXPOSE_BINOMIAL(piranha::rational,piranha::rational);
	PYRANHA_EXPOSE_BINOMIAL(piranha::rational,piranha::real);
	PYRANHA_EXPOSE_BINOMIAL(piranha::real,double);
	PYRANHA_EXPOSE_BINOMIAL(piranha::real,piranha::integer);
	PYRANHA_EXPOSE_BINOMIAL(piranha::real,piranha::rational);
	PYRANHA_EXPOSE_BINOMIAL(piranha::real,piranha::real);
#undef PYRANHA_EXPOSE_BINOMIAL
	// Sine and cosine.
#define PYRANHA_EXPOSE_SIN_COS(arg) \
bp::def("_sin",&piranha::math::sin<arg>); \
bp::def("_cos",&piranha::math::cos<arg>)
	PYRANHA_EXPOSE_SIN_COS(double);
	PYRANHA_EXPOSE_SIN_COS(piranha::integer);
	PYRANHA_EXPOSE_SIN_COS(piranha::rational);
	PYRANHA_EXPOSE_SIN_COS(piranha::real);
#undef PYRANHA_EXPOSE_SIN_COS
#define PYRANHA_EXPOSE_INVERT(arg) \
bp::def("_invert",&piranha::math::invert<arg>)
	PYRANHA_EXPOSE_INVERT(double);
	PYRANHA_EXPOSE_INVERT(piranha::integer);
	PYRANHA_EXPOSE_INVERT(piranha::rational);
	PYRANHA_EXPOSE_INVERT(piranha::real);
#undef PYRANHA_EXPOSE_INVERT
	// GCD.
	bp::def("_gcd",&piranha::math::gcd<piranha::integer,piranha::integer>);
	// Cleanup function.
	bp::def("_cleanup_type_system",&cleanup_type_system);
}
