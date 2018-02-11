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

#ifndef PIRANHA_MATH_POW_HPP
#define PIRANHA_MATH_POW_HPP

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/concepts.hpp>
#include <mp++/integer.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/integer.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// The default (empty) implementation.
template <typename T, typename U, typename = void>
class pow_impl
{
};

inline namespace impl
{

// Enabler for pow().
template <typename T, typename U>
using pow_type_ = decltype(pow_impl<uncvref_t<T>, uncvref_t<U>>{}(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using pow_type = enable_if_t<is_returnable<pow_type_<T, U>>::value, pow_type_<T, U>>;
}

// The exponentiation function.
template <typename T, typename U>
inline pow_type<T &&, U &&> pow(T &&x, U &&y)
{
    return pow_impl<uncvref_t<T>, uncvref_t<U>>{}(std::forward<T>(x), std::forward<U>(y));
}

// Specialisation of the implementation of piranha::pow() for C++ arithmetic types.
// It will use std::pow() if at least one of the types is an FP, and mp++ integral exponentiation
// otherwise.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppArithmetic T, CppArithmetic U>
class pow_impl<T, U>
#else
template <typename T, typename U>
class pow_impl<T, U, enable_if_t<conjunction<std::is_arithmetic<T>, std::is_arithmetic<U>>::value>>
#endif
{
    using use_std_pow = disjunction<std::is_floating_point<T>, std::is_floating_point<U>>;
    template <typename T1, typename U1>
    static auto impl(const T1 &x, const U1 &y, const std::true_type &) -> decltype(std::pow(x, y))
    {
        return std::pow(x, y);
    }
    template <typename T1, typename U1>
    static integer impl(const T1 &x, const U1 &y, const std::false_type &)
    {
        return mppp::pow(integer{x}, y);
    }

public:
    auto operator()(const T &x, const U &y) const -> decltype(impl(x, y, use_std_pow{}))
    {
        return impl(x, y, use_std_pow{});
    }
};

// Specialisation of the implementation of piranha::pow() for mp++'s integers.
// NOTE: this specialisation must be here (rather than integer.hpp) as in the integral-integral overload above we use
// mppp::integer inside, so the declaration of mppp::integer must be avaiable. On the other hand, we cannot put this in
// integer.hpp as the integral-integral overload is supposed to work without including mppp::integer.hpp.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, mppp::IntegerOpTypes<U> T>
class pow_impl<T, U>
#else
template <typename T, typename U>
class pow_impl<T, U, enable_if_t<mppp::are_integer_op_types<T, U>::value>>
#endif
{
public:
    template <typename T1, typename U1>
    auto operator()(T1 &&b, U1 &&e) const -> decltype(mppp::pow(std::forward<T1>(b), std::forward<U1>(e)))
    {
        return mppp::pow(std::forward<T1>(b), std::forward<U1>(e));
    }
};

// Implementation of the type trait to detect exponentiability.
inline namespace impl
{

template <typename T, typename U>
using pow_t = decltype(piranha::pow(std::declval<const T &>(), std::declval<const U &>()));
}

template <typename T, typename U>
using is_exponentiable = is_detected<pow_t, T, U>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool Exponentiable = is_exponentiable<T, U>::value;

#endif
}

#endif
