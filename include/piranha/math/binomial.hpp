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

#ifndef PIRANHA_MATH_BINOMIAL_HPP
#define PIRANHA_MATH_BINOMIAL_HPP

#include <type_traits>
#include <utility>

#include <mp++/integer.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/integer.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// The default (empty) implementation.
template <typename T, typename U, typename = void>
class binomial_impl
{
};

inline namespace impl
{

// Enabler for binomial().
template <typename T, typename U>
using binomial_type_ = decltype(binomial_impl<uncvref_t<T>, uncvref_t<U>>{}(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using binomial_type = enable_if_t<is_returnable<binomial_type_<T, U>>::value, binomial_type_<T, U>>;
}

// Generalised binomial coefficient.
template <typename T, typename U>
inline binomial_type<T &&, U &&> binomial(T &&x, U &&y)
{
    return binomial_impl<uncvref_t<T>, uncvref_t<U>>{}(std::forward<T>(x), std::forward<U>(y));
}

// Specialisation for C++ integrals.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppIntegral T, CppIntegral U>
class binomial_impl<T, U>
#else
template <typename T, typename U>
class binomial_impl<T, U, enable_if_t<conjunction<std::is_integral<T>, std::is_integral<U>>::value>>
#endif
{
public:
    integer operator()(const T &x, const U &y) const
    {
        return mppp::binomial(integer{x}, y);
    }
};

// Specialisation for mp++ integers.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, mppp::IntegerIntegralOpTypes<U> T>
class binomial_impl<T, U>
#else
template <typename T, typename U>
class binomial_impl<T, U, enable_if_t<mppp::are_integer_integral_op_types<T, U>::value>>
#endif
{
public:
    template <typename T1, typename U1>
    auto operator()(T1 &&x, U1 &&y) const -> decltype(mppp::binomial(std::forward<T1>(x), std::forward<U1>(y)))
    {
        return mppp::binomial(std::forward<T1>(x), std::forward<U1>(y));
    }
};

// Implementation of the type trait to detect the availability of binomial().
inline namespace impl
{

template <typename T, typename U>
using binomial_t = decltype(piranha::binomial(std::declval<const T &>(), std::declval<const U &>()));
}

template <typename T, typename U>
using are_binomial_types = is_detected<binomial_t, T, U>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool BinomialTypes = are_binomial_types<T, U>::value;

#endif
}

#endif
