/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

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
#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"
#include "mp_integer.hpp"

namespace piranha
{

namespace detail
{

template <typename T, typename U>
using integer_pow_enabler =
    typename std::enable_if<(is_mp_integer<T>::value && is_mp_integer_interoperable_type<U>::value)
                            || (is_mp_integer<U>::value && is_mp_integer_interoperable_type<T>::value) ||
                            // NOTE: here we are catching two arguments with potentially different
                            // bits. BUT this case is not caught in the pow_impl, so we should be ok as long
                            // as we don't allow interoperablity with different bits.
                            (is_mp_integer<T>::value &&is_mp_integer<U>::value)
                            || (std::is_integral<T>::value && std::is_integral<U>::value)>::type;

// Enabler for the pow overload for arithmetic and floating-point types.
template <typename T, typename U>
using pow_fp_arith_enabler =
    typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<U>::value
                            && (std::is_floating_point<T>::value || std::is_floating_point<U>::value)>::type;
}

namespace math
{

/// Default functor for the implementation of piranha::math::pow().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename U, typename Enable = void>
struct pow_impl {
};

/// Specialisation of the piranha::math::pow() functor for arithmetic and floating-point types.
/**
 * This specialisation is activated when both arguments are C++ arithmetic types and at least one argument
 * is a floating-point type.
 */
template <typename T, typename U>
struct pow_impl<T, U, detail::pow_fp_arith_enabler<T, U>> {
    /// Call operator.
    /**
     * This operator will compute the exponentiation via one of the overloads of <tt>std::pow()</tt>.
     *
     * @param[in] x base.
     * @param[out] y exponent.
     *
     * @return <tt>x**y</tt>.
     */
    template <typename T2, typename U2>
    auto operator()(const T2 &x, const U2 &y) const -> decltype(std::pow(x, y))
    {
        return std::pow(x, y);
    }
};

/// Exponentiation.
/**
 * Return \p x to the power of \p y. The actual implementation of this function is in the piranha::math::pow_impl
 * functor's
 * call operator.
 *
 * @param[in] x base.
 * @param[in] y exponent.
 *
 * @return \p x to the power of \p y.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::math::pow_impl functor.
 */
template <typename T, typename U>
inline auto pow(const T &x, const U &y) -> decltype(pow_impl<T, U>()(x, y))
{
    return pow_impl<T, U>()(x, y);
}

// NOTE: this specialisation must be here as in the integral-integral overload we use mp_integer inside,
// so the declaration of mp_integer must be avaiable. On the other hand, we cannot put this in mp_integer.hpp
// as the integral-integral overload is supposed to work without including mp_integer.hpp.
/// Specialisation of the piranha::math::pow() functor for piranha::mp_integer and integral types.
/**
 * This specialisation is activated when:
 * - one of the arguments is piranha::mp_integer and the other is either
 *   piranha::mp_integer or an interoperable type for piranha::mp_integer,
 * - both arguments are integral types.
 *
 * The implementation follows these rules:
 * - if the arguments are both piranha::mp_integer, or a piranha::mp_integer and an integral type, then
 * piranha::mp_integer::pow() is used
 *   to compute the result (after any necessary conversion),
 * - if both arguments are integral types, piranha::mp_integer::pow() is used after the conversion of the base
 *   to piranha::mp_integer,
 * - otherwise, the piranha::mp_integer argument is converted to the floating-point type and \p piranha::math::pow() is
 *   used to compute the result.
 */
template <typename T, typename U>
struct pow_impl<T, U, detail::integer_pow_enabler<T, U>> {
    /// Call operator, integral--integral overload.
    /**
     * @param[in] b base.
     * @param[in] e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::pow()
     * or by the constructor of piranha::mp_integer from integral type.
     */
    template <typename T2, typename U2,
              typename std::enable_if<std::is_integral<T2>::value && std::is_integral<U2>::value, int>::type = 0>
    integer operator()(const T2 &b, const U2 &e) const
    {
        return integer(b).pow(e);
    }
    /// Call operator, piranha::mp_integer overload.
    /**
     * @param[in] b base.
     * @param[in] e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::pow()
     * or by the constructor of piranha::mp_integer from integral type.
     */
    template <int NBits>
    mp_integer<NBits> operator()(const mp_integer<NBits> &b, const mp_integer<NBits> &e) const
    {
        return b.pow(e);
    }
    /// Call operator, integer--integral overload.
    /**
     * @param[in] b base.
     * @param[in] e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::pow().
     */
    template <int NBits, typename T2, typename std::enable_if<std::is_integral<T2>::value, int>::type = 0>
    mp_integer<NBits> operator()(const mp_integer<NBits> &b, const T2 &e) const
    {
        return b.pow(e);
    }
    /// Call operator, integer--floating-point overload.
    /**
     * @param[in] b base.
     * @param[in] e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by converting piranha::mp_integer to a floating-point type.
     */
    template <int NBits, typename T2, typename std::enable_if<std::is_floating_point<T2>::value, int>::type = 0>
    T2 operator()(const mp_integer<NBits> &b, const T2 &e) const
    {
        return math::pow(static_cast<T2>(b), e);
    }
    /// Call operator, integral--integer overload.
    /**
     * @param[in] b base.
     * @param[in] e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::pow().
     */
    template <int NBits, typename T2, typename std::enable_if<std::is_integral<T2>::value, int>::type = 0>
    mp_integer<NBits> operator()(const T2 &b, const mp_integer<NBits> &e) const
    {
        return mp_integer<NBits>(b).pow(e);
    }
    /// Call operator, floating-point--integer overload.
    /**
     * @param[in] b base.
     * @param[in] e exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by converting piranha::mp_integer to a floating-point type.
     */
    template <int NBits, typename T2, typename std::enable_if<std::is_floating_point<T2>::value, int>::type = 0>
    T2 operator()(const T2 &b, const mp_integer<NBits> &e) const
    {
        return math::pow(b, static_cast<T2>(e));
    }
};
}

/// Type trait for exponentiable types.
/**
 * The type trait will be \p true if piranha::math::pow() can be successfully called with base \p T and
 * exponent \p U.
 *
 * The call to piranha::math::pow() will be tested with const reference arguments.
 */
template <typename T, typename U>
class is_exponentiable : detail::sfinae_types
{
    template <typename Base, typename Expo>
    static auto test(const Base &b, const Expo &e) -> decltype(math::pow(b, e), void(), yes());
    static no test(...);

public:
    /// Value of the type trait.
    static const bool value = std::is_same<decltype(test(std::declval<T>(), std::declval<U>())), yes>::value;
};

// Static init.
template <typename T, typename U>
const bool is_exponentiable<T, U>::value;
}

#endif
