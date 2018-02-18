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

#ifndef PIRANHA_MATH_GCD_HPP
#define PIRANHA_MATH_GCD_HPP

#include <piranha/config.hpp>

#if PIRANHA_CPLUSPLUS >= 201703L
#include <numeric>
#endif
#include <type_traits>
#include <utility>

#include <piranha/detail/init.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// The default (empty) implementation.
template <typename T, typename U
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
class gcd_impl
{
};

inline namespace impl
{

// Enabler for gcd().
template <typename T, typename U>
using gcd_type_ = decltype(gcd_impl<uncvref_t<T>, uncvref_t<U>>{}(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using gcd_type = enable_if_t<is_returnable<gcd_type_<T, U>>::value, gcd_type_<T, U>>;
}

// GCD.
template <typename T, typename U>
inline gcd_type<T &&, U &&> gcd(T &&x, U &&y)
{
    return gcd_impl<uncvref_t<T>, uncvref_t<U>>{}(std::forward<T>(x), std::forward<U>(y));
}

// Specialisation for C++ integrals.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppIntegral T, CppIntegral U>
class gcd_impl<T, U>
#else
template <typename T, typename U>
class gcd_impl<T, U, enable_if_t<conjunction<std::is_integral<T>, std::is_integral<U>>::value>>
#endif
{
    // NOTE: here we use the same return type as specified by std::gcd().
    // This forces us to do some casting around to avoid compiler warnings
    // when using short ints.
    using ret_t = typename std::common_type<T, U>::type;
    // NOTE: we have to mess around a bit here as std::gcd() does not accept bools:
    // http://en.cppreference.com/w/cpp/numeric/gcd
    // Just write a couple of ad-hoc implementations if bools are involved.
    static bool impl(bool b1, bool b2)
    {
        // NOTE: only way this returns zero is if b1 and b2 are both false.
        return b1 || b2;
    }
    // A couple of implementations if only 1 argument is bool.
    // Promote to the other type and invoke the general impl.
    template <typename T1>
    static ret_t impl(bool b, T1 x)
    {
        return impl(T1(b), x);
    }
    template <typename T1>
    static ret_t impl(T1 x, bool b)
    {
        return impl(b, x);
    }
    // General implementation.
    template <typename T1, typename U1>
    static ret_t impl(T1 x, U1 y)
    {
#if PIRANHA_CPLUSPLUS >= 201703L
        static_assert(!std::is_same<T1, bool>::value && !std::is_same<U1, bool>::value);
        return std::gcd(x, y);
#else
        // NOTE: like std::gcd(), let's abs() both input arguments. This ensures
        // in the algorithm below everything stays positive.
        auto make_abs = [](ret_t x) { return x >= ret_t(0) ? static_cast<ret_t>(x) : static_cast<ret_t>(-x); };
        ret_t a(make_abs(x)), b(make_abs(y));
        while (true) {
            if (a == ret_t(0)) {
                return b;
            }
            b = static_cast<ret_t>(b % a);
            if (b == ret_t(0)) {
                return a;
            }
            a = static_cast<ret_t>(a % b);
        }
#endif
    }

public:
    ret_t operator()(T x, U y) const
    {
        return impl(x, y);
    }
};

// Implementation of the type trait to detect the availability of gcd().
inline namespace impl
{

template <typename T, typename U>
using gcd_t = decltype(piranha::gcd(std::declval<const T &>(), std::declval<const U &>()));
}

template <typename T, typename U = T>
struct are_gcd_types : is_detected<gcd_t, T, U> {
};

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T>
concept bool GcdTypes = are_gcd_types<T, U>::value;

#endif
}

#endif
