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

#ifndef PIRANHA_DETAIL_SAFE_INTEGRAL_ARITH_HPP
#define PIRANHA_DETAIL_SAFE_INTEGRAL_ARITH_HPP

#include <stdexcept>
#include <string>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/exceptions.hpp>

// We have to do a bit of preprocessor woodwork to determine
// if we have the integer overflow builtins.

#if defined(PIRANHA_COMPILER_IS_GCC) && __GNUC__ >= 5

// GCC >= 5 is good to go:
// https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// https://software.intel.com/en-us/forums/intel-c-compiler/topic/720757
#define PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS

#elif defined(PIRANHA_COMPILER_IS_CLANG)

// Clang has them since version 3.4:
// http://releases.llvm.org/3.4/tools/clang/docs/LanguageExtensions.html
// However we need to take care of AppleClang's hijacking of clang's version macros:
// https://stackoverflow.com/questions/19387043/how-can-i-reliably-detect-the-version-of-clang-at-preprocessing-time
// It seems like on OSX clang's version macros refer to the Xcode version. According to this table, clang 3.4 first
// appears in Xcode 5.1:
// https://en.wikipedia.org/wiki/Xcode#8.x_series

#if defined(__apple_build_version__)

// Xcode >= 5.1.
#if __clang_major__ > 5 || (__clang_major__ == 5 && __clang_minor__ >= 1)

#define PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS

#endif

#else

// Vanilla clang >= 3.4.
#if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 4)

#define PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS

#endif

#endif

#endif

#if !defined(PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS)

#include <limits>

#endif

namespace piranha
{

inline namespace impl
{

// Little wrapper for creating the error message for the safe
// add/sub functions.
template <typename T>
inline std::string safe_int_arith_err(const char *op, T op1, T op2)
{
    return std::string("overflow error in an integral ") + op + ": the operands' type is '" + demangle<T>()
           + "', and the operands' values are " + std::to_string(op1) + " and " + std::to_string(op2);
}

#if defined(PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS)

template <typename T>
inline T safe_int_add(T a, T b)
{
    // A couple of compile-time checks.
    // First, these functions are supposed to be called only with integral types in input.
    static_assert(std::is_integral<T>::value, "This function needs integeral types in input.");
    // Second, the overflow builtins do not work on bools (we have an explicit specialisation
    // for bools later).
    static_assert(!std::is_same<T, bool>::value, "This function should not be invoked with a bool argument.");
    T retval;
    if (unlikely(__builtin_add_overflow(a, b, &retval))) {
        piranha_throw(std::overflow_error, safe_int_arith_err("addition", a, b));
    }
    return retval;
}

template <typename T>
inline T safe_int_sub(T a, T b)
{
    static_assert(std::is_integral<T>::value, "This function needs integeral types in input.");
    static_assert(!std::is_same<T, bool>::value, "This function should not be invoked with a bool argument.");
    T retval;
    if (unlikely(__builtin_sub_overflow(a, b, &retval))) {
        piranha_throw(std::overflow_error, safe_int_arith_err("subtraction", a, b));
    }
    return retval;
}

#else

// The signed add implementation.
template <typename T>
inline T safe_int_add_impl(T a, T b, const std::true_type &)
{
    if (b >= T(0)) {
        if (unlikely(a > std::numeric_limits<T>::max() - b)) {
            piranha_throw(std::overflow_error, safe_int_arith_err("addition", a, b));
        }
    } else {
        if (unlikely(a < std::numeric_limits<T>::min() - b)) {
            piranha_throw(std::overflow_error, safe_int_arith_err("addition", a, b));
        }
    }
    return static_cast<T>(a + b);
}

// The unsigned add implementation.
template <typename T>
inline T safe_int_add_impl(T a, T b, const std::false_type &)
{
    if (unlikely(a > std::numeric_limits<T>::max() - b)) {
        piranha_throw(std::overflow_error, safe_int_arith_err("addition", a, b));
    }
    return static_cast<T>(a + b);
}

// The signed sub implementation.
template <typename T>
inline T safe_int_sub_impl(T a, T b, const std::true_type &)
{
    if (b <= T(0)) {
        if (unlikely(a > std::numeric_limits<T>::max() + b)) {
            piranha_throw(std::overflow_error, safe_int_arith_err("subtraction", a, b));
        }
    } else {
        if (unlikely(a < std::numeric_limits<T>::min() + b)) {
            piranha_throw(std::overflow_error, safe_int_arith_err("subtraction", a, b));
        }
    }
    return static_cast<T>(a - b);
}

// The unsigned sub implementation.
template <typename T>
inline T safe_sub_add_impl(T a, T b, const std::false_type &)
{
    if (unlikely(a < b)) {
        piranha_throw(std::overflow_error, safe_int_arith_err("subtraction", a, b));
    }
    return static_cast<T>(a - b);
}

template <typename T>
inline T safe_int_add(T a, T b)
{
    static_assert(std::is_integral<T>::value, "This function needs integeral types in input.");
    static_assert(!std::is_same<T, bool>::value, "This function should not be invoked with a bool argument.");
    return safe_int_add_impl(a, b, std::is_signed<T>{});
}

template <typename T>
inline T safe_int_sub(T a, T b)
{
    static_assert(std::is_integral<T>::value, "This function needs integeral types in input.");
    static_assert(!std::is_same<T, bool>::value, "This function should not be invoked with a bool argument.");
    return safe_int_sub_impl(a, b, std::is_signed<T>{});
}

#endif

// Let's special case bools, as the integer overflow builtins will not work with them.
template <>
inline bool safe_int_add(bool a, bool b)
{
    if (unlikely(a && b)) {
        piranha_throw(std::overflow_error, safe_int_arith_err("addition", a, b));
    }
    return (a + b) != 0;
}

template <>
inline bool safe_int_sub(bool a, bool b)
{
    if (unlikely(!a && b)) {
        piranha_throw(std::overflow_error, safe_int_arith_err("subtraction", a, b));
    }
    return (a - b) != 0;
}
}
}

// Undefine the macro, if necessary, as we don't need it outside this file.
#if defined(PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS)

#undef PIRANHA_HAVE_INTEGER_OVERFLOW_BUILTINS

#endif

#endif
