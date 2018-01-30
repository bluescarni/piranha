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

#ifndef PIRANHA_MATH_COS_HPP
#define PIRANHA_MATH_COS_HPP

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

namespace math
{

// Default (empty) implementation.
template <typename T, typename Enable = void>
class cos_impl
{
};

inline namespace impl
{

// Enabler+result type for math::cos().
template <typename T>
using math_cos_t_ = decltype(cos_impl<uncvref_t<T>>{}(std::declval<T>()));

template <typename T>
using math_cos_t = enable_if_t<is_returnable<math_cos_t_<T>>::value, math_cos_t_<T>>;
}

// Cosine.
template <typename T>
inline math_cos_t<T &&> cos(T &&x)
{
    return cos_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

// Specialisation of the implementation of piranha::math::cos() for C++ arithmetic types.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppArithmetic T>
class cos_impl<T>
#else
template <typename T>
class cos_impl<T, enable_if_t<std::is_arithmetic<T>::value>>
#endif
{
    // Floating-point overload.
    template <typename T1>
    static T1 impl(const T1 &x, const std::true_type &)
    {
        return std::cos(x);
    }
    // Integral overload.
    template <typename T1>
    static T1 impl(const T1 &x, const std::false_type &)
    {
        if (unlikely(x != T1(0))) {
            piranha_throw(std::domain_error,
                          "cannot compute the cosine of the non-zero C++ integral " + std::to_string(x));
        }
        return T1(1);
    }

public:
    T operator()(const T &x) const
    {
        return impl(x, std::is_floating_point<T>{});
    }
};
}

// Implementation of the type trait to detect the availability of cos().
inline namespace impl
{

template <typename T>
using cos_t = decltype(math::cos(std::declval<const T &>()));
}

template <typename T>
using is_cosine_type = is_detected<cos_t, T>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool CosineType = is_cosine_type<T>::value;

#endif
}

#endif
