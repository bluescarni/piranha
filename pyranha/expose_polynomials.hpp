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

#ifndef PYRANHA_EXPOSE_POLYNOMIALS_HPP
#define PYRANHA_EXPOSE_POLYNOMIALS_HPP

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/tuple.hpp>
#include <string>
#include <tuple>
#include <type_traits>

#include <piranha/polynomial.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

PYRANHA_DECLARE_T_NAME(piranha::polynomial)

// Custom hook for polynomials.
template <typename Descriptor>
struct poly_custom_hook {
    PIRANHA_DECLARE_HAS_TYPEDEF(degree_truncation_types);
    template <typename S>
    static void unset_auto_truncate_degree_wrapper()
    {
        S::unset_auto_truncate_degree();
    }
    template <typename S>
    static bp::tuple get_auto_truncate_degree_wrapper()
    {
        auto retval = S::get_auto_truncate_degree();
        bp::list l;
        for (const auto &s : std::get<2u>(retval)) {
            l.append(s);
        }
        return bp::make_tuple(std::get<0u>(retval), std::get<1u>(retval), l);
    }
    template <typename S,
              typename std::enable_if<piranha::detail::has_get_auto_truncate_degree<S>::value, int>::type = 0>
    static void expose_degree_auto_truncation_get_unset(bp::class_<S> &series_class)
    {
        series_class.def("unset_auto_truncate_degree", unset_auto_truncate_degree_wrapper<S>)
            .staticmethod("unset_auto_truncate_degree");
        series_class.def("get_auto_truncate_degree", get_auto_truncate_degree_wrapper<S>)
            .staticmethod("get_auto_truncate_degree");
    }
    template <typename S,
              typename std::enable_if<!piranha::detail::has_get_auto_truncate_degree<S>::value, int>::type = 0>
    static void expose_degree_auto_truncation_get_unset(bp::class_<S> &)
    {
    }
    template <typename S>
    struct auto_truncate_set_exposer {
        auto_truncate_set_exposer(bp::class_<S> &series_class) : m_series_class(series_class) {}
        bp::class_<S> &m_series_class;
        template <typename T>
        static void set_auto_truncate_degree_wrapper(const T &max_degree)
        {
            S::set_auto_truncate_degree(max_degree);
        }
        template <typename T>
        static void set_auto_truncate_pdegree_wrapper(const T &max_degree, bp::list l)
        {
            bp::stl_input_iterator<std::string> begin(l), end;
            S::set_auto_truncate_degree(max_degree, piranha::symbol_fset(begin, end));
        }
        template <typename T>
        void expose_set_auto_truncate_degree(
            const T &,
            typename std::enable_if<piranha::detail::has_set_auto_truncate_degree<S, T>::value>::type * = nullptr) const
        {
            m_series_class.def("set_auto_truncate_degree", set_auto_truncate_degree_wrapper<T>);
            m_series_class.def("set_auto_truncate_degree", set_auto_truncate_pdegree_wrapper<T>);
            // NOTE: let's expose here the truncated mult static methods, as they share the same requirements
            // (more or less).
            m_series_class.def("truncated_multiplication", +[](const S &p1, const S &p2, const T &max_degree) {
                return S::truncated_multiplication(p1, p2, max_degree);
            });
            m_series_class.def(
                "truncated_multiplication", +[](const S &p1, const S &p2, const T &max_degree, bp::list l) -> S {
                    bp::stl_input_iterator<std::string> begin(l), end;
                    return S::truncated_multiplication(p1, p2, max_degree, piranha::symbol_fset(begin, end));
                });
            set_auto_truncate_degree_exposed() = true;
        }
        template <typename T>
        void expose_set_auto_truncate_degree(
            const T &,
            typename std::enable_if<!piranha::detail::has_set_auto_truncate_degree<S, T>::value>::type * = nullptr)
            const
        {
        }
        static bool &set_auto_truncate_degree_exposed()
        {
            static bool retval = false;
            return retval;
        }
        template <typename T>
        void operator()(const T &x) const
        {
            expose_set_auto_truncate_degree(x);
        }
    };
    template <typename S, typename T = Descriptor,
              typename std::enable_if<has_typedef_degree_truncation_types<T>::value, int>::type = 0>
    static void expose_degree_auto_truncation_set(bp::class_<S> &series_class)
    {
        using dt_types = typename Descriptor::degree_truncation_types;
        dt_types dt;
        tuple_for_each(dt, auto_truncate_set_exposer<S>(series_class));
        if (auto_truncate_set_exposer<S>::set_auto_truncate_degree_exposed()) {
            series_class.staticmethod("set_auto_truncate_degree");
            series_class.staticmethod("truncated_multiplication");
        }
    }
    template <typename S, typename T = Descriptor,
              typename std::enable_if<!has_typedef_degree_truncation_types<T>::value, int>::type = 0>
    static void expose_degree_auto_truncation_set(bp::class_<S> &)
    {
    }
    // find_cf() exposition.
    template <typename S>
    static typename S::term_type::cf_type find_cf_wrapper(const S &s, bp::list l)
    {
        using expo_type = typename S::term_type::key_type::value_type;
        bp::stl_input_iterator<expo_type> begin(l), end;
        return s.find_cf(std::vector<expo_type>(begin, end));
    }
    // The call operator.
    template <typename T>
    void operator()(bp::class_<T> &series_class) const
    {
        // We need to separate get and set, as they have different
        // type requirements.
        expose_degree_auto_truncation_get_unset(series_class);
        expose_degree_auto_truncation_set(series_class);
        // This is always available.
        series_class
            .def("untruncated_multiplication",
                 +[](const T &p1, const T &p2) { return T::untruncated_multiplication(p1, p2); })
            .staticmethod("untruncated_multiplication");
        // find_cf().
        series_class.def("find_cf", find_cf_wrapper<T>);
    }
};

void expose_polynomials_0();
void expose_polynomials_1();
void expose_polynomials_2();
void expose_polynomials_3();
void expose_polynomials_4();
void expose_polynomials_5();
void expose_polynomials_6();
void expose_polynomials_7();
void expose_polynomials_8();
void expose_polynomials_9();
void expose_polynomials_10();
}

#endif
