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

#ifndef PIRANHA_POW_HPP
#define PIRANHA_POW_HPP

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <piranha/mp_integer.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace math
{

/// Default functor for the implementation of piranha::math::pow().
/**
 * This functor can be specialised via the \p std::enable_if mechanism. The default implementation does not define
 * the call operator, and will thus generate a compile-time error if used.
 */
template <typename T, typename U, typename = void>
struct pow_impl {
};
}

inline namespace impl
{

// Enabler for the pow overload for arithmetic and floating-point types.
template <typename T, typename U>
using pow_fp_arith_enabler
    = enable_if_t<conjunction<std::is_arithmetic<T>, std::is_arithmetic<U>,
                              disjunction<std::is_floating_point<T>, std::is_floating_point<U>>>::value>;
}

namespace math
{

/// Specialisation of the implementation of piranha::math::pow() for arithmetic and floating-point types.
/**
 * This specialisation is activated when both arguments are C++ arithmetic types and at least one argument
 * is a floating-point type.
 */
template <typename T, typename U>
struct pow_impl<T, U, pow_fp_arith_enabler<T, U>> {
    /// Call operator.
    /**
     * This operator will compute the exponentiation via one of the overloads of <tt>std::pow()</tt>.
     *
     * @param x base.
     * @param y exponent.
     *
     * @return <tt>x**y</tt>.
     */
    auto operator()(const T &x, const U &y) const -> decltype(std::pow(x, y))
    {
        return std::pow(x, y);
    }
};
}

inline namespace impl
{

// Enabler for math::pow().
template <typename T, typename U>
using math_pow_t_ = decltype(math::pow_impl<T, U>{}(std::declval<const T &>(), std::declval<const U &>()));

template <typename T, typename U>
using math_pow_t = enable_if_t<is_returnable<math_pow_t_<T, U>>::value, math_pow_t_<T, U>>;
}

namespace math
{

/// Exponentiation.
/**
 * \note
 * This function is enabled only if the expression <tt>pow_impl<T, U>{}(x, y)</tt> is valid, returning
 * a type which satisfies piranha::is_returnable.
 *
 * Return \p x to the power of \p y. The actual implementation of this function is in the piranha::math::pow_impl
 * functor's call operator. The body of this function is equivalent to:
 * @code
 * return pow_impl<T, U>{}(x, y);
 * @endcode
 *
 * @param x base.
 * @param y exponent.
 *
 * @return \p x to the power of \p y.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::math::pow_impl functor.
 */
template <typename T, typename U>
inline math_pow_t<T, U> pow(const T &x, const U &y)
{
    return pow_impl<T, U>{}(x, y);
}
}

inline namespace impl
{

// Enabler for integral power.
template <typename T, typename U>
using integer_pow_enabler
    = enable_if_t<disjunction<conjunction<is_mp_integer<T>, mppp::mppp_impl::is_supported_interop<U>>,
                              conjunction<is_mp_integer<U>, mppp::mppp_impl::is_supported_interop<T>>,
                              conjunction<std::is_integral<T>, std::is_integral<U>>, is_same_mp_integer<T, U>>::value>;

// Wrapper for ADL.
template <typename T, typename U>
auto mp_integer_pow_wrapper(const T &base, const U &exp) -> decltype(pow(base, exp))
{
    return pow(base, exp);
}
}

namespace math
{

// NOTE: this specialisation must be here as in the integral-integral overload we use mp_integer inside,
// so the declaration of mp_integer must be avaiable. On the other hand, we cannot put this in mp_integer.hpp
// as the integral-integral overload is supposed to work without including mp_integer.hpp.
/// Specialisation of the implementation of piranha::math::pow() for piranha::mp_integer and integral types.
/**
 * \note
 * This specialisation is activated when:
 * - both arguments are piranha::mp_integer with the same static size,
 * - one of the arguments is piranha::mp_integer and the other is an interoperable type for piranha::mp_integer,
 * - both arguments are C++ integral types.
 *
 * The implementation follows these rules:
 * - if the arguments are both piranha::mp_integer, or a piranha::mp_integer and an interoperable type for
 *   piranha::mp_integer, then piranha::mp_integer::pow() is used to compute the result,
 * - if both arguments are integral types, piranha::mp_integer::pow() is used after the conversion of the base
 *   to piranha::integer.
 */
template <typename T, typename U>
struct pow_impl<T, U, integer_pow_enabler<T, U>> {
private:
    // C++ integral -- C++ integral.
    template <typename T2, typename U2,
              enable_if_t<conjunction<std::is_integral<T2>, std::is_integral<U2>>::value, int> = 0>
    static integer impl(const T2 &b, const U2 &e)
    {
        return mp_integer_pow_wrapper(integer{b}, e);
    }
    // The other cases.
    template <typename T2, typename U2,
              enable_if_t<negation<conjunction<std::is_integral<T2>, std::is_integral<U2>>>::value, int> = 0>
    static auto impl(const T2 &b, const U2 &e) -> decltype(mp_integer_pow_wrapper(b, e))
    {
        return mp_integer_pow_wrapper(b, e);
    }
    using ret_type = decltype(impl(std::declval<const T &>(), std::declval<const U &>()));

public:
    /// Call operator.
    /**
     * @param b base.
     * @param e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::pow().
     */
    ret_type operator()(const T &b, const U &e) const
    {
        return impl(b, e);
    }
};
}

/// Type trait for exponentiable types.
/**
 * The type trait will be \p true if piranha::math::pow() can be successfully called with base \p T and
 * exponent \p U.
 */
template <typename T, typename U>
class is_exponentiable
{
    template <typename Base, typename Expo>
    using pow_t = decltype(math::pow(std::declval<const Base &>(), std::declval<const Expo &>()));
    static const bool implementation_defined = is_detected<pow_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename T, typename U>
const bool is_exponentiable<T, U>::value;
}

#endif
