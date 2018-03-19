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

#ifndef PIRANHA_KEY_KEY_LDEGREE_HPP
#define PIRANHA_KEY_KEY_LDEGREE_HPP

#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default functor for the implementation of piranha::key_ldegree().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
class key_ldegree_impl
{
};

inline namespace impl
{

// Candidate result type for piranha::key_ldegree() (both total and partial degree overloads).
template <typename T>
using total_key_ldegree_t_
    = decltype(key_ldegree_impl<uncvref_t<T>>{}(std::declval<T>(), std::declval<const symbol_fset &>()));

template <typename T>
using partial_key_ldegree_t_ = decltype(key_ldegree_impl<uncvref_t<T>>{}(
    std::declval<T>(), std::declval<const symbol_idx_fset &>(), std::declval<const symbol_fset &>()));
}

template <typename T>
using is_key_ldegree_type = conjunction<is_returnable<detected_t<total_key_ldegree_t_, T>>,
                                        is_returnable<detected_t<partial_key_ldegree_t_, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool KeyLdegreeType = is_key_ldegree_type<T>::value;

#endif

inline namespace impl
{

template <typename T>
using total_key_ldegree_t = enable_if_t<is_key_ldegree_type<T>::value, total_key_ldegree_t_<T>>;

template <typename T>
using partial_key_ldegree_t = enable_if_t<is_key_ldegree_type<T>::value, partial_key_ldegree_t_<T>>;
} // namespace impl

// Total low degree of a key.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <KeyLdegreeType T>
inline auto
#else
template <typename T>
inline total_key_ldegree_t<T>
#endif
key_ldegree(T &&x, const symbol_fset &s)
{
    return key_ldegree_impl<uncvref_t<T>>{}(std::forward<T>(x), s);
}

// Partial low degree of a key.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <KeyLdegreeType T>
inline auto
#else
template <typename T>
inline partial_key_ldegree_t<T>
#endif
key_ldegree(T &&x, const symbol_idx_fset &idx, const symbol_fset &s)
{
    return key_ldegree_impl<uncvref_t<T>>{}(std::forward<T>(x), idx, s);
}
}

#endif
