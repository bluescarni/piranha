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

#ifndef PIRANHA_KEY_KEY_DEGREE_HPP
#define PIRANHA_KEY_KEY_DEGREE_HPP

#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default functor for the implementation of piranha::key_degree().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
class key_degree_impl
{
};

inline namespace impl
{

// Enabler for key_degree() (total degree overload).
template <typename T>
using tot_key_degree_type_
    = decltype(key_degree_impl<uncvref_t<T>>{}(std::declval<T>(), std::declval<const symbol_fset &>()));

template <typename T>
using tot_key_degree_type = enable_if_t<is_returnable<tot_key_degree_type_<T>>::value, tot_key_degree_type_<T>>;
}

// Total degree of a key.
template <typename T>
inline tot_key_degree_type<T &&> key_degree(T &&x, const symbol_fset &s)
{
    return key_degree_impl<uncvref_t<T>>{}(std::forward<T>(x), s);
}

inline namespace impl
{

// Enabler for key_degree() (partial degree overload).
template <typename T>
using par_key_degree_type_ = decltype(key_degree_impl<uncvref_t<T>>{}(
    std::declval<T>(), std::declval<const symbol_idx_fset &>(), std::declval<const symbol_fset &>()));

template <typename T>
using par_key_degree_type = enable_if_t<is_returnable<par_key_degree_type_<T>>::value, par_key_degree_type_<T>>;
}

// Partial degree of a key.
template <typename T>
inline par_key_degree_type<T &&> key_degree(T &&x, const symbol_idx_fset &p, const symbol_fset &s)
{
    return key_degree_impl<uncvref_t<T>>{}(std::forward<T>(x), p, s);
}

inline namespace impl
{

template <typename T>
using tot_key_degree_t = decltype(piranha::key_degree(std::declval<const T &>(), std::declval<const symbol_fset &>()));

template <typename T>
using par_key_degree_t = decltype(piranha::key_degree(
    std::declval<const T &>(), std::declval<const symbol_idx_fset &>(), std::declval<const symbol_fset &>()));
}

// Type trait to detect the presence of the piranha::key_degree() overloads.
template <typename T>
using is_key_degree_type = conjunction<is_detected<tot_key_degree_t, T>, is_detected<par_key_degree_t, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool IsKeyDegreeType = is_key_degree_type<T>::value;

#endif
}

#endif
