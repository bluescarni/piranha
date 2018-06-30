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

#ifndef PIRANHA_SAFE_CONVERT_HPP
#define PIRANHA_SAFE_CONVERT_HPP

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

#include <boost/numeric/conversion/converter.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Default implementation of piranha::safe_convert().
template <typename To, typename From
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
class safe_convert_impl
{
public:
    // NOTE: the default implementation is enabled if T and U are the same type
    // (after uncvref) and we can assign from to out.
    template <
        typename T, typename U,
        enable_if_t<conjunction<std::is_same<uncvref_t<T>, uncvref_t<U>>, std::is_assignable<T, U>>::value, int> = 0>
    bool operator()(T &&out, U &&from) const
    {
        std::forward<T>(out) = std::forward<U>(from);
        return true;
    }
};

inline namespace impl
{

// Candidate return type for piranha::safe_convert().
template <typename To, typename From>
using safe_convert_t_
    = decltype(safe_convert_impl<uncvref_t<To>, uncvref_t<From>>{}(std::declval<To>(), std::declval<From>()));
} // namespace impl

template <typename From, typename To>
using is_safely_convertible = std::is_convertible<detected_t<safe_convert_t_, To, From>, bool>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename From, typename To>
concept bool SafelyConvertible = is_safely_convertible<From, To>::value;

#endif

// Safe conversion.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename To, SafelyConvertible<To> From>
#else
template <typename To, typename From, enable_if_t<is_safely_convertible<From, To>::value, int> = 0>
#endif
inline bool safe_convert(To &&x, From &&y)
{
    return safe_convert_impl<uncvref_t<To>, uncvref_t<From>>{}(std::forward<To>(x), std::forward<From>(y));
}

// Specialisation for integral to integral conversions.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppIntegral To, CppIntegral From>
class safe_convert_impl<To, From>
#else
template <typename To, typename From>
class safe_convert_impl<To, From, enable_if_t<conjunction<std::is_integral<To>, std::is_integral<From>>::value>>
#endif
{
    // Unsigned-unsigned overload.
    template <typename T1, typename U1>
    static bool impl(T1 &out, U1 x, const std::false_type &, const std::false_type &)
    {
        if (x > std::numeric_limits<T1>::max()) {
            return false;
        }
        out = static_cast<T1>(x);
        return true;
    }
    // Signed-signed overload.
    template <typename T1, typename U1>
    static bool impl(T1 &out, U1 x, const std::true_type &, const std::true_type &)
    {
        if (x > std::numeric_limits<T1>::max() || x < std::numeric_limits<T1>::min()) {
            return false;
        }
        out = static_cast<T1>(x);
        return true;
    }
    // Signed-unsigned overload.
    template <typename T1, typename U1>
    static bool impl(T1 &out, U1 x, const std::true_type &, const std::false_type &)
    {
        if (x > static_cast<typename std::make_unsigned<T1>::type>(std::numeric_limits<T1>::max())) {
            return false;
        }
        out = static_cast<T1>(x);
        return true;
    }
    // Unsigned-signed overload.
    template <typename T1, typename U1>
    static bool impl(T1 &out, U1 x, const std::false_type &, const std::true_type &)
    {
        if (x < U1(0) || static_cast<typename std::make_unsigned<U1>::type>(x) > std::numeric_limits<T1>::max()) {
            return false;
        }
        out = static_cast<T1>(x);
        return true;
    }

public:
    bool operator()(To &out, From x) const
    {
        return impl(out, x, std::is_signed<To>{}, std::is_signed<From>{});
    }
};

// Specialisation for fp to integral conversions.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CppIntegral To, CppFloatingPoint From>
class safe_convert_impl<To, From>
#else
template <typename To, typename From>
class safe_convert_impl<To, From, enable_if_t<conjunction<std::is_integral<To>, std::is_floating_point<From>>::value>>
#endif
{
public:
    bool operator()(To &out, From x) const
    {
        // NOTE: the conversion fails if either:
        // - x is not finite (NaN, inf), or
        // - x is not an integral value, or,
        // - x does not fit in the range of T (checked via Boost's conversion
        //   utilities).
        if (!std::isfinite(x) || std::trunc(x) != x
            || boost::numeric::converter<To, From>::out_of_range(x) != boost::numeric::range_check_result::cInRange) {
            return false;
        }
        out = static_cast<To>(x);
        return true;
    }
};
} // namespace piranha

#endif
