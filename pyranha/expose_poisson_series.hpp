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

#ifndef PYRANHA_EXPOSE_POISSON_SERIES_HPP
#define PYRANHA_EXPOSE_POISSON_SERIES_HPP

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/detail/sfinae_types.hpp"
#include "../src/poisson_series.hpp"
#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

PYRANHA_DECLARE_T_NAME(piranha::poisson_series)

// Custom hook for Poisson series.
struct ps_custom_hook {
    // Detect and enable t_integrate() conditionally.
    template <typename T>
    struct has_t_integrate : piranha::detail::sfinae_types {
        template <typename T1>
        static auto test(const T1 &x) -> decltype(x.t_integrate(), void(), yes());
        static no test(...);
        static const bool value = std::is_same<decltype(test(std::declval<T>())), yes>::value;
    };
    template <typename S>
    static auto t_integrate_wrapper(const S &s) -> decltype(s.t_integrate())
    {
        return s.t_integrate();
    }
    // NOTE: here the return type is the same as returned by the other overload of t_integrate().
    template <typename S>
    static auto t_integrate_names_wrapper(const S &s, bp::list l) -> decltype(s.t_integrate())
    {
        bp::stl_input_iterator<std::string> begin_p(l), end_p;
        return s.t_integrate(std::vector<std::string>(begin_p, end_p));
    }
    template <typename S, typename std::enable_if<has_t_integrate<S>::value, int>::type = 0>
    static void expose_t_integrate(bp::class_<S> &series_class)
    {
        series_class.def("t_integrate", t_integrate_wrapper<S>);
        series_class.def("t_integrate", t_integrate_names_wrapper<S>);
    }
    template <typename S, typename std::enable_if<!has_t_integrate<S>::value, int>::type = 0>
    static void expose_t_integrate(bp::class_<S> &)
    {
    }
    template <typename T>
    void operator()(bp::class_<T> &series_class) const
    {
        expose_t_integrate(series_class);
    }
};

void expose_poisson_series_0();
void expose_poisson_series_1();
void expose_poisson_series_2();
void expose_poisson_series_3();
void expose_poisson_series_4();
void expose_poisson_series_5();
void expose_poisson_series_6();
void expose_poisson_series_7();
void expose_poisson_series_8();
void expose_poisson_series_9();
void expose_poisson_series_10();
void expose_poisson_series_11();
}

#endif
