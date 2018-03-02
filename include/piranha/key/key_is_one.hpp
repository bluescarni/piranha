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

#ifndef PIRANHA_KEY_KEY_IS_ONE_HPP
#define PIRANHA_KEY_KEY_IS_ONE_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default functor for the implementation of piranha::key_is_one().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
class key_is_one_impl
{
};

inline namespace impl
{

// Candidate type for piranha::key_is_one().
template <typename T>
using key_is_one_t_ = decltype(key_is_one_impl<uncvref_t<T>>{}(std::declval<T>(), std::declval<const symbol_fset &>()));
}

template <typename T>
using is_key_is_one_type = std::is_convertible<detected_t<key_is_one_t_, T>, bool>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool KeyIsOneType = is_key_is_one_type<T>::value;

#endif

// One detection for keys.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <KeyIsOneType T>
#else
template <typename T, enable_if_t<is_key_is_one_type<T>::value, int> = 0>
#endif
inline bool key_is_one(T &&x, const symbol_fset &s)
{
    return key_is_one_impl<uncvref_t<T>>{}(std::forward<T>(x), s);
}
}

#endif
