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
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default (empty) implementation.
template <typename T, typename Enable = void>
class cos_impl
{
};

inline namespace impl
{

// Candidate result type for piranha::cos().
template <typename T>
using cos_t_ = decltype(cos_impl<uncvref_t<T>>{}(std::declval<T>()));
}

// NOTE: when we use this TT/concept in conjunction with the perfect
// forwarding cos() function below, there are two possibilities:
// - cos() is called on an lvalue of type A, in which case T resolves to A &
//   and, in cos_t_(), we end up calling the call operator of cos_impl
//   with type A & && -> A & thanks to the collapsing rules;
// - cos() is called on an rvalue of type A, in which case T resolves to
//   A and, in cos_t_(), we end up calling the call operator of cos_impl
//   with type A &&.
template <typename T>
using is_cosine_type = is_returnable<detected_t<cos_t_, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool CosineType = is_cosine_type<T>::value;

#endif

inline namespace impl
{

// NOTE: this is needed for the non-concept implementation, and
// useful as a shortcut to the type of piranha::cos() in various
// internal implementation details.
template <typename T>
using cos_t = enable_if_t<is_cosine_type<T>::value, cos_t_<T>>;
}

// Cosine.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CosineType T>
inline auto
#else
template <typename T>
inline cos_t<T>
#endif
cos(T &&x)
{
    return cos_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

// Specialisation of the implementation of piranha::cos() for C++ arithmetic types.
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

#endif
