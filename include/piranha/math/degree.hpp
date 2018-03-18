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

// Candidate result type for piranha::degree() (both total and partial degree overloads).
template <typename T>
using total_degree_t_ = decltype(degree_impl<uncvref_t<T>>{}(std::declval<T>()));

template <typename T>
using partial_degree_t_ = decltype(degree_impl<uncvref_t<T>>{}(std::declval<T>(), std::declval<const symbol_fset &>()));

} // namespace impl

template <typename T>
using is_degree_type
    = conjunction<is_returnable<detected_t<total_degree_t_, T>>, is_returnable<detected_t<partial_degree_t_, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool DegreeType = is_degree_type<T>::value;

#endif

inline namespace impl
{

template <typename T>
using total_degree_t = enable_if_t<is_degree_type<T>::value, total_degree_t_<T>>;

template <typename T>
using partial_degree_t = enable_if_t<is_degree_type<T>::value, partial_degree_t_<T>>;
} // namespace impl

// Total degree.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <DegreeType T>
inline auto
#else
template <typename T>
inline total_degree_t<T>
#endif
degree(T &&x)
{
    return degree_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

// Partial degree.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <DegreeType T>
inline auto
#else
template <typename T>
inline partial_degree_t<T>
#endif
degree(T &&x, const symbol_fset &s)
{
    return degree_impl<uncvref_t<T>>{}(std::forward<T>(x), s);
}
} // namespace piranha

#endif
