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

#ifndef PIRANHA_MATH_LDEGREE_HPP
#define PIRANHA_MATH_LDEGREE_HPP

#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default functor for the implementation of piranha::ldegree() (both overloads).
template <typename T, typename = void>
class ldegree_impl
{
};

inline namespace impl
{

// Candidate result type for piranha::ldegree() (both total and partial degree overloads).
template <typename T>
using total_ldegree_t_ = decltype(ldegree_impl<uncvref_t<T>>{}(std::declval<T>()));

template <typename T>
using partial_ldegree_t_
    = decltype(ldegree_impl<uncvref_t<T>>{}(std::declval<T>(), std::declval<const symbol_fset &>()));

} // namespace impl

template <typename T>
using is_ldegree_type
    = conjunction<is_returnable<detected_t<total_ldegree_t_, T>>, is_returnable<detected_t<partial_ldegree_t_, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool LdegreeType = is_ldegree_type<T>::value;

#endif

inline namespace impl
{

template <typename T>
using total_ldegree_t = enable_if_t<is_ldegree_type<T>::value, total_ldegree_t_<T>>;

template <typename T>
using partial_ldegree_t = enable_if_t<is_ldegree_type<T>::value, partial_ldegree_t_<T>>;
} // namespace impl

// Total low degree.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <LdegreeType T>
inline auto
#else
template <typename T>
inline total_ldegree_t<T>
#endif
ldegree(T &&x)
{
    return ldegree_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

// Partial low degree.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <LdegreeType T>
inline auto
#else
template <typename T>
inline partial_ldegree_t<T>
#endif
ldegree(T &&x, const symbol_fset &s)
{
    return ldegree_impl<uncvref_t<T>>{}(std::forward<T>(x), s);
}
} // namespace piranha

#endif
