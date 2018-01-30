/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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
#include <boost/python/stl_iterator.hpp>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <mp++/config.hpp>

#include <piranha/config.hpp>
#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/integer.hpp>
#include <piranha/invert.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/math.hpp>
#include <piranha/math/binomial.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/monomial.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/thread_pool.hpp>
#include <piranha/type_traits.hpp>

#include "exceptions.hpp"
#include "expose_divisor_series.hpp"
#include "expose_poisson_series.hpp"
#include "expose_polynomials.hpp"
#include "expose_utils.hpp"
#include "python_converters.hpp"
#include "type_system.hpp"
#include "utils.hpp"

namespace bp = boost::python;

static std::mutex global_mutex;
static bool inited = false;

namespace pyranha
{

PYRANHA_DECLARE_T_NAME(piranha::monomial)
PYRANHA_DECLARE_T_NAME(piranha::divisor)
}

// A couple of utils to test exception translation.
template <typename Exc, piranha::enable_if_t<std::is_constructible<Exc, std::string>::value, int> = 0>
static inline void test_exception()
{
    piranha_throw(Exc, "hello world");
}

template <typename Exc, piranha::enable_if_t<!std::is_constructible<Exc, std::string>::value, int> = 0>
static inline void test_exception()
{
    piranha_throw(Exc, );
}

// Small helper to retrieve the argument error exception from python.
static inline void generate_argument_error(int) {}

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
    // Docstring options setup.
    bp::docstring_options doc_options(false, false, false);
    // Type generator class.
    bp::class_<pyranha::type_generator> tg_class("_type_generator", bp::no_init);
    tg_class.def("__call__", &pyranha::type_generator::operator());
    tg_class.def("__repr__", &pyranha::type_generator::repr);
    // Type generator template class.
    bp::class_<pyranha::type_generator_template> tgt_class("_type_generator_template", bp::no_init);
    tgt_class.def("__getitem__", &pyranha::type_generator_template::getitem_o);
    tgt_class.def("__getitem__", &pyranha::type_generator_template::getitem_t);
    tgt_class.def("__repr__", &pyranha::type_generator_template::repr);
    // Create the types submodule.
    std::string types_module_name = bp::extract<std::string>(bp::scope().attr("__name__") + ".types");
    // NOTE: the nested namespace is created if not there, otherwise it will be returned.
    ::PyObject *types_module_ptr = ::PyImport_AddModule(types_module_name.c_str());
    if (!types_module_ptr) {
        ::PyErr_SetString(PyExc_RuntimeError, "error while creating the 'types' submodule");
        bp::throw_error_already_set();
    }
    // NOTE: I think at one point in the past there was a typo in the Python3 C API documentation
    // which hinted at a difference in behaviour for PyImport_AddModule between 2 and 3 (new reference
    // vs borrowed reference). Current documentation states that the reference is always
    // borrowed, both in Python 2 and 3.
    auto types_module = bp::object(bp::handle<>(bp::borrowed(types_module_ptr)));
    bp::scope().attr("types") = types_module;
    // Signal the presence of MPFR.
    bp::scope().attr("_with_mpfr") =
#if defined(MPPP_WITH_MPFR)
        true
#else
        false
#endif
        ;
    // Expose concrete instances of type generators.
    pyranha::instantiate_type_generator<std::int_least16_t>("int16", types_module);
    pyranha::instantiate_type_generator<double>("double", types_module);
    pyranha::instantiate_type_generator<piranha::integer>("integer", types_module);
    pyranha::instantiate_type_generator<piranha::rational>("rational", types_module);
#if defined(MPPP_WITH_MPFR)
    pyranha::instantiate_type_generator<piranha::real>("real", types_module);
#endif
    pyranha::instantiate_type_generator<piranha::k_monomial>("k_monomial", types_module);
    // Register template instances of monomial, and instantiate the type generator template.
    pyranha::instantiate_type_generator_template<piranha::monomial>("monomial", types_module);
    pyranha::register_template_instance<piranha::monomial, piranha::rational>();
    pyranha::register_template_instance<piranha::monomial, std::int_least16_t>();
    // Same for divisor.
    pyranha::instantiate_type_generator_template<piranha::divisor>("divisor", types_module);
    pyranha::register_template_instance<piranha::divisor, std::int_least16_t>();
    // Arithmetic converters.
    pyranha::integer_converter i_c;
    pyranha::rational_converter ra_c;
#if defined(MPPP_WITH_MPFR)
    pyranha::real_converter re_c;
#endif
    // Exceptions translation.
    // NOTE: the order matters here, as translators registered later are tried first.
    // Since some of our exceptions derive from std exceptions, we want to make sure
    // our more specific exceptions have the priority when translating.
    pyranha::generic_translate<&PyExc_ValueError, std::domain_error>();
    pyranha::generic_translate<&PyExc_OverflowError, std::overflow_error>();
    pyranha::generic_translate<&PyExc_ZeroDivisionError, mppp::zero_division_error>();
    pyranha::generic_translate<&PyExc_NotImplementedError, piranha::not_implemented_error>();
    pyranha::generic_translate<&PyExc_OverflowError, boost::numeric::positive_overflow>();
    pyranha::generic_translate<&PyExc_OverflowError, boost::numeric::negative_overflow>();
    pyranha::generic_translate<&PyExc_OverflowError, boost::numeric::bad_numeric_cast>();
    pyranha::generic_translate<&PyExc_ValueError, piranha::safe_cast_failure>();
#if defined(PIRANHA_WITH_MSGPACK)
    pyranha::generic_translate<&PyExc_TypeError, msgpack::type_error>();
#endif
    // Exposed types list.
    bp::def("_get_exposed_types_list", pyranha::get_exposed_types_list);
    // The s11n enums.
    bp::enum_<piranha::data_format>("data_format")
        .value("boost_binary", piranha::data_format::boost_binary)
        .value("boost_portable", piranha::data_format::boost_portable)
        .value("msgpack_binary", piranha::data_format::msgpack_binary)
        .value("msgpack_portable", piranha::data_format::msgpack_portable);
    bp::enum_<piranha::compression>("compression")
        .value("none", piranha::compression::none)
        .value("zlib", piranha::compression::zlib)
        .value("gzip", piranha::compression::gzip)
        .value("bzip2", piranha::compression::bzip2);
    // Expose polynomials.
    pyranha::instantiate_type_generator_template<piranha::polynomial>("polynomial", types_module);
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
    // Expose Poisson series.
    pyranha::instantiate_type_generator_template<piranha::poisson_series>("poisson_series", types_module);
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
    // Expose divisor series.
    pyranha::instantiate_type_generator_template<piranha::divisor_series>("divisor_series", types_module);
    pyranha::expose_divisor_series_0();
    pyranha::expose_divisor_series_1();
    pyranha::expose_divisor_series_2();
    pyranha::expose_divisor_series_3();
    pyranha::expose_divisor_series_4();
    pyranha::expose_divisor_series_5();
    // Expose the settings class.
    bp::class_<piranha::settings> settings_class("_settings", bp::init<>());
    settings_class.def("_get_max_term_output", piranha::settings::get_max_term_output)
        .staticmethod("_get_max_term_output");
    settings_class.def("_set_max_term_output", piranha::settings::set_max_term_output)
        .staticmethod("_set_max_term_output");
    settings_class.def("_reset_max_term_output", piranha::settings::reset_max_term_output)
        .staticmethod("_reset_max_term_output");
    settings_class.def("_set_n_threads", piranha::settings::set_n_threads).staticmethod("_set_n_threads");
    settings_class.def("_get_n_threads", piranha::settings::get_n_threads).staticmethod("_get_n_threads");
    settings_class.def("_reset_n_threads", piranha::settings::reset_n_threads).staticmethod("_reset_n_threads");
    settings_class.def("_set_min_work_per_thread", piranha::settings::set_min_work_per_thread)
        .staticmethod("_set_min_work_per_thread");
    settings_class.def("_get_min_work_per_thread", piranha::settings::get_min_work_per_thread)
        .staticmethod("_get_min_work_per_thread");
    settings_class.def("_reset_min_work_per_thread", piranha::settings::reset_min_work_per_thread)
        .staticmethod("_reset_min_work_per_thread");
    settings_class.def("_set_thread_binding", piranha::settings::set_thread_binding)
        .staticmethod("_set_thread_binding");
    settings_class.def("_get_thread_binding", piranha::settings::get_thread_binding)
        .staticmethod("_get_thread_binding");
    // Factorial.
    bp::def("_factorial", &piranha::math::factorial<1>);
// Binomial coefficient.
#define PYRANHA_EXPOSE_BINOMIAL(top, bot) bp::def("_binomial", &piranha::math::binomial<const top &, const bot &>)
    PYRANHA_EXPOSE_BINOMIAL(piranha::integer, piranha::integer);
    PYRANHA_EXPOSE_BINOMIAL(piranha::rational, piranha::integer);
#undef PYRANHA_EXPOSE_BINOMIAL
// Sine and cosine.
#define PYRANHA_EXPOSE_SIN_COS(arg)                                                                                    \
    bp::def("_sin", &piranha::math::sin<const arg &>);                                                                 \
    bp::def("_cos", &piranha::math::cos<const arg &>)
    PYRANHA_EXPOSE_SIN_COS(double);
    PYRANHA_EXPOSE_SIN_COS(piranha::integer);
    PYRANHA_EXPOSE_SIN_COS(piranha::rational);
#if defined(MPPP_WITH_MPFR)
    PYRANHA_EXPOSE_SIN_COS(piranha::real);
#endif
#undef PYRANHA_EXPOSE_SIN_COS
#define PYRANHA_EXPOSE_INVERT(arg) bp::def("_invert", &piranha::math::invert<arg>)
    PYRANHA_EXPOSE_INVERT(double);
    PYRANHA_EXPOSE_INVERT(piranha::integer);
    PYRANHA_EXPOSE_INVERT(piranha::rational);
#if defined(MPPP_WITH_MPFR)
    PYRANHA_EXPOSE_INVERT(piranha::real);
#endif
#undef PYRANHA_EXPOSE_INVERT
    // GCD.
    bp::def("_gcd", &piranha::math::gcd<piranha::integer, piranha::integer>);
    // Tests for exception translation.
    bp::def("_test_safe_cast_failure", &test_exception<piranha::safe_cast_failure>);
    bp::def("_test_zero_division_error", &test_exception<mppp::zero_division_error>);
    bp::def("_test_not_implemented_error", &test_exception<piranha::not_implemented_error>);
    bp::def("_test_overflow_error", &test_exception<std::overflow_error>);
    bp::def("_test_bn_poverflow_error", &test_exception<boost::numeric::positive_overflow>);
    bp::def("_test_bn_noverflow_error", &test_exception<boost::numeric::negative_overflow>);
    bp::def("_test_bn_bnc", &test_exception<boost::numeric::bad_numeric_cast>);
    // Helper to generate an argument error.
    bp::def("_generate_argument_error", &generate_argument_error);

    // Define a cleanup functor to be run when the module is unloaded.
    struct cleanup_functor {
        void operator()() const
        {
            // First we clean up the custom derivatives.
            auto e_types = pyranha::get_exposed_types_list();
            bp::stl_input_iterator<bp::object> end_it;
            for (bp::stl_input_iterator<bp::object> it(e_types); it != end_it; ++it) {
                if (pyranha::hasattr(*it, "unregister_all_custom_derivatives")) {
                    it->attr("unregister_all_custom_derivatives")();
                }
            }
            pyranha::builtin().attr("print")("Custom derivatives cleanup completed.");
            // Next we clean up the pow caches.
            for (bp::stl_input_iterator<bp::object> it(e_types); it != end_it; ++it) {
                if (pyranha::hasattr(*it, "clear_pow_cache")) {
                    it->attr("clear_pow_cache")();
                }
            }
            pyranha::builtin().attr("print")("Pow caches cleanup completed.");
            // Clean up the pyranha type system.
            pyranha::et_map.clear();
            pyranha::builtin().attr("print")("Pyranha's type system cleanup completed.");
            // Finally, shut down the thread pool.
            // NOTE: this is necessary in Windows/MinGW currently, otherwise the python
            // interpreter hangs on exit (possibly due to either some implementation-defined
            // static order destruction fiasco, or maybe a threading bug in MinGW).
            std::cout << "Shutting down the thread pool.\n";
            piranha::thread_pool_shutdown<void>();
        }
    };
    // Expose it.
    bp::class_<cleanup_functor> cl_c("_cleanup_functor", bp::init<>());
    cl_c.def("__call__", &cleanup_functor::operator());
    // Register it.
    bp::object atexit_mod = bp::import("atexit");
    atexit_mod.attr("register")(cleanup_functor{});
}
