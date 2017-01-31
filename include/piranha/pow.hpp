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
#include <string>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

inline namespace impl
{

// Enabler for integral power.
template <typename T, typename U>
using integer_pow_enabler
    = enable_if_t<disjunction<conjunction<is_mp_integer<T>, mppp::mppp_impl::is_supported_interop<U>>,
                              conjunction<is_mp_integer<U>, mppp::mppp_impl::is_supported_interop<T>>,
                              conjunction<std::is_integral<T>, std::is_integral<U>>, is_same_mp_integer<T, U>>::value>;

// Enabler for the pow overload for arithmetic and floating-point types.
template <typename T, typename U>
using pow_fp_arith_enabler
    = enable_if_t<conjunction<std::is_arithmetic<T>, std::is_arithmetic<U>,
                              disjunction<std::is_floating_point<T>, std::is_floating_point<U>>>::value>;
}

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
using math_pow_t_ = decltype(math::pow_impl<T,U>{}(std::declval<const T &>(), std::declval<const U &>()));

template <typename T, typename U>
using math_pow_t = enable_if_t<is_returnable<math_pow_t_<T,U>>::value,math_pow_t_<T,U>>;

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
inline math_pow_t<T,U> pow(const T &x, const U &y)
{
    return pow_impl<T, U>{}(x, y);
}

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
 * - if the arguments are both piranha::mp_integer, or a piranha::mp_integer and an integral type, then
 *   piranha::mp_integer::pow_ui() is used to compute the result (after any necessary conversion),
 * - if both arguments are integral types, piranha::mp_integer::pow_ui() is used after the conversion of the base
 *   to piranha::integer,
 * - otherwise, the piranha::mp_integer argument is converted to the floating-point type and piranha::math::pow() is
 *   used to compute the result.
 */
template <typename T, typename U>
struct pow_impl<T, U, integer_pow_enabler<T, U>> {
    /// Call operator, integral--integral overload.
    /**
     * @param b base.
     * @param e exponent.
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
        if (e >= U2(0)) {
            return pow_ui(integer(b), safe_cast<unsigned long>(e));
        }
        if (unlikely(!b)) {
            piranha_throw(zero_division_error, "cannot raise zero to the negative integral power " + std::to_string(e));
        }
        
    }
    /// Call operator, piranha::mp_integer overload.
    /**
     * @param b base.
     * @param e exponent.
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
     * @param b base.
     * @param e exponent.
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
     * @param b base.
     * @param e exponent.
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
     * @param b base.
     * @param e exponent.
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
     * @param b base.
     * @param e exponent.
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
