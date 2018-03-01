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

#ifndef PIRANHA_MATH_IS_ZERO_HPP
#define PIRANHA_MATH_IS_ZERO_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default functor for the implementation of piranha::is_zero().
template <typename T, typename = void>
class is_zero_impl
{
public:
    // NOTE: the equality comparable requirement already implies that the return type of
    // the comparison must be convertible to bool.
    template <
        typename U,
        enable_if_t<conjunction<std::is_constructible<U, int>, is_equality_comparable<const U &>>::value, int> = 0>
    bool operator()(const U &x) const
    {
        return x == U(0);
    }
};

inline namespace impl
{

template <typename T>
using is_zero_t_ = decltype(is_zero_impl<uncvref_t<T>>{}(std::declval<T>()));
}

template <typename T>
using is_is_zero_type = std::is_convertible<detected_t<is_zero_t_, T>, bool>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool IsZeroType = is_is_zero_type<T>::value;

#endif

// Zero detection.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <IsZeroType T>
#else
template <typename T, enable_if_t<is_is_zero_type<T>::value, int> = 0>
#endif
inline bool is_zero(T &&x)
{
    return is_zero_impl<uncvref_t<T>>{}(std::forward<T>(x));
}

// Specialisation of the piranha::is_zero() functor for C++ complex floating-point types.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppComplex T>
class is_zero_impl<T>
#else
template <typename T>
class is_zero_impl<T, enable_if_t<is_cpp_complex<T>::value>>
#endif
{
public:
    bool operator()(const T &c) const
    {
        return c.real() == typename T::value_type(0) && c.imag() == typename T::value_type(0);
    }
};
}

#endif
