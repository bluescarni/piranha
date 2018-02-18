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
    template <typename T1, typename U1, typename V1>
    using p_gcd_t = decltype(std::declval<T1>() = piranha::gcd(std::declval<U1>(), std::declval<V1>()));

public:
    template <typename T1, typename U1, typename V1,
              enable_if_t<is_detected<p_gcd_t, T1 &, U1 &&, V1 &&>::value, int> = 0>
    void operator()(T1 &x, U1 &&y, V1 &&z) const
    {
        x = piranha::gcd(std::forward<U1>(y), std::forward<V1>(z));
    }
};

inline namespace impl
{

// Enabler for gcd3().
template <typename T, typename U, typename V>
using gcd3_type_
    = decltype(gcd3_impl<T, uncvref_t<U>, uncvref_t<V>>{}(std::declval<T &>(), std::declval<U>(), std::declval<V>()));

template <typename T, typename U, typename V>
using gcd3_type = enable_if_t<is_detected<gcd3_type_, T, U, V>::value>;
}

// GCD, ternary form.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <NonConst T, typename U, typename V>
#else
template <typename T, typename U, typename V, enable_if_t<!std::is_const<T>::value, int> = 0>
#endif
inline gcd3_type<T, U &&, V &&> gcd3(T &x, U &&y, V &&z)
{
    gcd3_impl<T, uncvref_t<U>, uncvref_t<V>>{}(x, std::forward<U>(y), std::forward<V>(z));
}

// Implementation of the type trait to detect the availability of gcd3().
inline namespace impl
{

template <typename T, typename U, typename V>
using gcd3_t = decltype(piranha::gcd3(std::declval<T &>(), std::declval<const U &>(), std::declval<const V &>()));
}

// NOTE: the pattern we want to detect here is:
// piranha::gcd3(a, b, c)
// where a is an lvalue reference to non-const, and b and c are references to const.
// We need to do the explicit negation of is_rvalue_reference because otherwise, due to
// the collapsing rules, T & above in gcd3_t becomes an lvalue ref, and we are then
// testing for the wrong thing. We don't need to do further manipulations to U and V
// because in gcd3_t we are using const lvalue refs, and everything can bind to them.
template <typename T, typename U = T, typename V = T>
struct are_gcd3_types : conjunction<negation<std::is_rvalue_reference<T>>, is_detected<gcd3_t, T, U, V>> {
};

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T, typename V = T>
concept bool Gcd3Types = are_gcd3_types<T, U, V>::value;

#endif
}

#endif
