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

#ifndef PIRANHA_MATH_GCD3_HPP
#define PIRANHA_MATH_GCD3_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/math/gcd.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// The default implementation.
template <typename T, typename U, typename V
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
class gcd3_impl
{
public:
    template <typename T1, typename U1, typename V1>
    auto operator()(T1 &&x, U1 &&y, V1 &&z) const
        -> decltype(void(std::forward<T1>(x) = piranha::gcd(std::forward<U1>(y), std::forward<V1>(z))))
    {
        std::forward<T1>(x) = piranha::gcd(std::forward<U1>(y), std::forward<V1>(z));
    }
};

inline namespace impl
{

// Candidate type for piranha::gcd3().
template <typename T, typename U, typename V>
using gcd3_t_ = decltype(
    gcd3_impl<uncvref_t<T>, uncvref_t<U>, uncvref_t<V>>{}(std::declval<T>(), std::declval<U>(), std::declval<V>()));
}

template <typename T, typename U, typename V = U>
struct are_gcd3_types : is_detected<gcd3_t_, T, U, V> {
};

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U, typename V = U>
concept bool Gcd3Types = are_gcd3_types<T, U, V>::value;

#endif

// GCD, ternary form.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, typename U>
inline void gcd3(Gcd3Types<T, U> &&x, T &&y, U &&z)
#else
template <typename T, typename U, typename V, enable_if_t<are_gcd3_types<T, U, V>::value, int> = 0>
inline void gcd3(T &&x, U &&y, V &&z)
#endif
{
    gcd3_impl<uncvref_t<decltype(x)>, uncvref_t<decltype(y)>, uncvref_t<decltype(z)>>{}(
        std::forward<decltype(x)>(x), std::forward<decltype(y)>(y), std::forward<decltype(z)>(z));
}
}

#endif
