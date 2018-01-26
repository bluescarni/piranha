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

namespace math
{

// The default (empty) implementation.
template <typename T, typename U, typename Enable = void>
class pow_impl
{
};

inline namespace impl
{

// Enabler for math::pow().
template <typename T, typename U>
using math_pow_t_ = decltype(pow_impl<uncvref_t<T>, uncvref_t<U>>{}(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using math_pow_t = enable_if_t<is_returnable<math_pow_t_<T, U>>::value, math_pow_t_<T, U>>;
}

// The exponentiation function.
template <typename T, typename U>
inline math_pow_t<T &&, U &&> pow(T &&x, U &&y)
{
    return pow_impl<uncvref_t<T>, uncvref_t<U>>{}(std::forward<T>(x), std::forward<U>(y));
}

// Machinery for the pow overload for arithmetic and floating-point types.
template <typename T, typename U>
using are_std_pow_types = conjunction<std::is_arithmetic<T>, std::is_arithmetic<U>,
                                      disjunction<std::is_floating_point<T>, std::is_floating_point<U>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool StdPowTypes = are_std_pow_types<T, U>::value;

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, StdPowTypes<U> T>
class pow_impl<T, U>
#else
template <typename T, typename U>
class pow_impl<T, U, enable_if_t<are_std_pow_types<T, U>::value>>
#endif
{
public:
    auto operator()(const T &x, const U &y) const -> decltype(std::pow(x, y))
    {
        return std::pow(x, y);
    }
};

// Machinery for the pow overload for integral types.
template <typename T, typename U>
using are_mppp_pow_types
    = disjunction<mppp::are_integer_op_types<T, U>,
                  conjunction<mppp::is_cpp_integral_interoperable<T>, mppp::is_cpp_integral_interoperable<U>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool MpppPowTypes = are_mppp_pow_types<T, U>::value;

#endif

// NOTE: this specialisation must be here as in the integral-integral overload we use mppp::integer inside,
// so the declaration of mppp::integer must be avaiable. On the other hand, we cannot put this in integer.hpp
// as the integral-integral overload is supposed to work without including mppp::integer.hpp.
// Specialisation for mp++'s integers and C++ integral types.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, MpppPowTypes<U> T>
class pow_impl<T, U>
#else
template <typename T, typename U>
class pow_impl<T, U, enable_if_t<are_mppp_pow_types<T, U>::value>>
#endif
{
    // C++ integral -- C++ integral.
    template <typename T2, typename U2>
    static integer impl(const T2 &b, const U2 &e, const std::true_type &)
    {
        return mppp::pow(integer{b}, e);
    }
    // The other cases.
    template <typename T2, typename U2>
    static auto impl(const T2 &b, const U2 &e, const std::false_type &) -> decltype(mppp::pow(b, e))
    {
        return mppp::pow(b, e);
    }
    using ret_type
        = decltype(impl(std::declval<const T &>(), std::declval<const U &>(),
                        std::integral_constant<bool, conjunction<mppp::is_cpp_integral_interoperable<T>,
                                                                 mppp::is_cpp_integral_interoperable<U>>::value>{}));

public:
    ret_type operator()(const T &b, const U &e) const
    {
        return impl(b, e,
                    std::integral_constant<bool, conjunction<mppp::is_cpp_integral_interoperable<T>,
                                                             mppp::is_cpp_integral_interoperable<U>>::value>{});
    }
};
}

// Implementation of the type trait to detect exponentiability.
inline namespace impl
{

template <typename T, typename U>
using pow_t = decltype(math::pow(std::declval<const T &>(), std::declval<const U &>()));
}

template <typename T, typename U>
using is_exponentiable = is_detected<pow_t, T, U>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool Exponentiable = is_exponentiable<T, U>::value;

#endif
}

#endif
