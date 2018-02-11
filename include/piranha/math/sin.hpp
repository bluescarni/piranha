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

#ifndef PIRANHA_MATH_SIN_HPP
#define PIRANHA_MATH_SIN_HPP

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default (empty) implementation.
template <typename T, typename Enable = void>
class sin_impl
{
};

inline namespace impl
{

// Enabler+result type for piranha::sin().
template <typename T>
using sin_type_ = decltype(sin_impl<uncvref_t<T>>{}(std::declval<T>()));

template <typename T>
using sin_type = enable_if_t<is_returnable<sin_type_<T>>::value, sin_type_<T>>;
}

// Sine.
template <typename T>
inline sin_type<T &&> sin(T &&x)
{
    return sin_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

// Specialisation of the implementation of piranha::sin() for C++ arithmetic types.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppArithmetic T>
class sin_impl<T>
#else
template <typename T>
class sin_impl<T, enable_if_t<std::is_arithmetic<T>::value>>
#endif
{
    // Floating-point overload.
    template <typename T1>
    static T1 impl(const T1 &x, const std::true_type &)
    {
        return std::sin(x);
    }
    // Integral overload.
    template <typename T1>
    static T1 impl(const T1 &x, const std::false_type &)
    {
        if (unlikely(x != T1(0))) {
            piranha_throw(std::domain_error,
                          "cannot compute the sine of the non-zero C++ integral " + std::to_string(x));
        }
        return T1(0);
    }

public:
    T operator()(const T &x) const
    {
        return impl(x, std::is_floating_point<T>{});
    }
};

// Implementation of the type trait to detect the availability of sin().
inline namespace impl
{

template <typename T>
using sin_t = decltype(piranha::sin(std::declval<const T &>()));
}

template <typename T>
using is_sine_type = is_detected<sin_t, T>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool SineType = is_sine_type<T>::value;

#endif
}

#endif
