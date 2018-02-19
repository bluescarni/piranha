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

#ifndef PIRANHA_MATH_DEGREE_HPP
#define PIRANHA_MATH_DEGREE_HPP

#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default functor for the implementation of piranha::degree() (both overloads).
template <typename T, typename = void>
class degree_impl
{
};

inline namespace impl
{

// Enabler for degree() (total degree overload).
template <typename T>
using tot_degree_type_ = decltype(degree_impl<uncvref_t<T>>{}(std::declval<T>()));

template <typename T>
using tot_degree_type = enable_if_t<is_returnable<tot_degree_type_<T>>::value, tot_degree_type_<T>>;
}

template <typename T>
inline tot_degree_type<T &&> degree(T &&x)
{
    return degree_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

inline namespace impl
{

// Enabler for degree() (partial degree overload).
template <typename T>
using par_degree_type_ = decltype(degree_impl<uncvref_t<T>>{}(std::declval<T>(), std::declval<const symbol_fset &>()));

template <typename T>
using par_degree_type = enable_if_t<is_returnable<par_degree_type_<T>>::value, par_degree_type_<T>>;
}

template <typename T>
inline par_degree_type_<T &&> degree(T &&x, const symbol_fset &s)
{
    return degree_impl<uncvref_t<T>>{}(std::forward<T>(x), s);
}

inline namespace impl
{

template <typename T>
using tot_degree_t = decltype(piranha::degree(std::declval<const T &>()));

template <typename T>
using par_degree_t = decltype(piranha::degree(std::declval<const T &>(), std::declval<const symbol_fset &>()));
}

// Type trait to detect the presence of the piranha::degree() overloads.
template <typename T>
using is_degree_type = conjunction<is_detected<tot_degree_t, T>, is_detected<par_degree_t, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool IsDegreeType = is_degree_type<T>::value;

#endif
}

#endif
