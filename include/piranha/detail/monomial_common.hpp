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

#ifndef PIRANHA_DETAIL_MONOMIAL_COMMON_HPP
#define PIRANHA_DETAIL_MONOMIAL_COMMON_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/integer.hpp>
#include <piranha/math.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

// Code shared in the implementation of monomials.

namespace piranha
{

inline namespace impl
{

// Dispatcher for the monomial exponentiation algorithm.
// Here T is the monomial's expo type, U the type of the power
// to which the monomial will be raised.
template <typename T, typename U>
using monomial_pow_dispatcher = disjunction_idx<
    // Case 0: T and U are both integrals.
    conjunction<std::is_integral<T>, std::is_integral<U>>,
    // Case 1: T and U are the same type, and they support math::mul3.
    conjunction<std::is_same<T, U>, has_mul3<U>>,
    // Case 2: T * U is well-defined, yielding T.
    std::is_same<detected_t<mul_t, T, U>, T>,
    // Case 3: T * U is well-defined, yielding a type that
    // can be safely cast back to T.
    has_safe_cast<T, detected_t<mul_t, T, U>>>;

// Implementation of case 0.
template <typename T, typename U>
inline void monomial_pow_mult_exp(T &ret, const T &exp, const U &x, const std::integral_constant<std::size_t, 0u> &)
{
    // NOTE: we could actually use the GCC intrinsics here to detect
    // overflow on integral multiply.
    PIRANHA_MAYBE_TLS integer tmp1, tmp2, tmp3;
    tmp1 = exp;
    tmp2 = x;
    mul(tmp3, tmp1, tmp2);
    // NOTE: we are assuming that T is move-assignable here.
    // This will result in std::overflow_error in case the result is too
    // large or small.
    ret = static_cast<T>(tmp3);
}

// Implementation of case 1.
template <typename T, typename U>
inline void monomial_pow_mult_exp(T &ret, const T &exp, const U &x, const std::integral_constant<std::size_t, 1u> &)
{
    math::mul3(ret, exp, x);
}

// Implementation of case 2.
template <typename T, typename U>
inline void monomial_pow_mult_exp(T &ret, const T &exp, const U &x, const std::integral_constant<std::size_t, 2u> &)
{
    ret = exp * x;
}

// Implementation of case 3.
template <typename T, typename U>
inline void monomial_pow_mult_exp(T &ret, const T &exp, const U &x, const std::integral_constant<std::size_t, 3u> &)
{
// NOTE: gcc here complains when x is a float and exp a large integer type,
// on the basis that the conversion from integral to fp might not preserve
// the value exactly (which is of course true). We don't care about this
// corner case, let's just disable "-Wconversion" temporarily.
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    ret = safe_cast<T>(exp * x);
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic pop
#endif
}

// The enabler.
template <typename T, typename U>
using monomial_pow_enabler = enable_if_t<(monomial_pow_dispatcher<T, U>::value < 4u), int>;
}
}

#endif
