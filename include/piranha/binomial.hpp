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

#ifndef PIRANHA_BINOMIAL_HPP
#define PIRANHA_BINOMIAL_HPP

#include <boost/math/constants/constants.hpp>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/pow.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

inline namespace impl
{

// TODO move this stuff in rational, the only place where it is used.
// Generic binomial implementation.
// template <typename T, enable_if_t<std::is_unsigned<T>::value, int> = 0>
// inline bool generic_binomial_check_k(const T &, const T &)
// {
//     return false;
// }
//
// template <typename T, enable_if_t<!std::is_unsigned<T>::value, int> = 0>
// inline bool generic_binomial_check_k(const T &k, const T &zero)
// {
//     return k < zero;
// }

// Generic binomial implementation using the falling factorial. U must be an integer
// type, T can be anything that supports basic arithmetics. k must be non-negative.
// template <typename T, typename U>
// inline T generic_binomial(const T &x, const U &k)
// {
//     const U zero(0), one(1);
//     if (generic_binomial_check_k(k, zero)) {
//         piranha_throw(std::invalid_argument, "negative k value in binomial coefficient");
//     }
//     // Zero at bottom results always in 1.
//     if (k == zero) {
//         return T(1);
//     }
//     T tmp(x), retval = x / T(k);
//     --tmp;
//     for (auto i = static_cast<U>(k - one); i >= one; --i, --tmp) {
//         retval *= tmp;
//         retval /= T(i);
//     }
//     return retval;
// }

// Compute gamma(a)/(gamma(b) * gamma(c)), assuming a, b and c are not negative ints.
// This is a helper function for the implementation of binomial() for fp types.
template <typename T>
inline T compute_3_gamma(const T &a, const T &b, const T &c)
{
    // Here we should never enter with negative ints.
    piranha_assert(a >= T(0) || std::trunc(a) != a);
    piranha_assert(b >= T(0) || std::trunc(b) != b);
    piranha_assert(c >= T(0) || std::trunc(c) != c);
    const T pi = boost::math::constants::pi<T>();
    T tmp0(0), tmp1(1);
    if (a < T(0)) {
        tmp0 -= std::lgamma(T(1) - a);
        tmp1 *= pi / std::sin(a * pi);
    } else {
        tmp0 += std::lgamma(a);
    }
    if (b < T(0)) {
        tmp0 += std::lgamma(T(1) - b);
        tmp1 *= std::sin(b * pi) / pi;
    } else {
        tmp0 -= std::lgamma(b);
    }
    if (c < T(0)) {
        tmp0 += std::lgamma(T(1) - c);
        tmp1 *= std::sin(c * pi) / pi;
    } else {
        tmp0 -= std::lgamma(c);
    }
    return std::exp(tmp0) * tmp1;
}

// Implementation of the generalised binomial coefficient for floating-point types.
template <typename T>
inline T math_fp_binomial(const T &x, const T &y)
{
    static_assert(std::is_floating_point<T>::value, "Invalid type for fp_binomial.");
    if (unlikely(!std::isfinite(x) || !std::isfinite(y))) {
        piranha_throw(std::invalid_argument,
                      "cannot compute binomial coefficient with non-finite floating-point argument(s)");
    }
    const bool neg_int_x = std::trunc(x + T(1)) == (x + T(1)) && (x + T(1)) <= T(0),
               neg_int_y = std::trunc(y + T(1)) == (y + T(1)) && (y + T(1)) <= T(0),
               neg_int_x_y = std::trunc(x - y + T(1)) == (x - y + T(1)) && (x - y + T(1)) <= T(0);
    const unsigned mask = unsigned(neg_int_x) + (unsigned(neg_int_y) << 1u) + (unsigned(neg_int_x_y) << 2u);
    switch (mask) {
        case 0u:
            // Case 0 is the non-special one, use the default implementation.
            return compute_3_gamma(x + T(1), y + T(1), x - y + T(1));
        // NOTE: case 1 is not possible: x < 0, y > 0 implies x - y < 0 always.
        case 2u:
        case 4u:
            // These are finite numerators with infinite denominators.
            return T(0.);
        // NOTE: case 6 is not possible: x > 0, y < 0 implies x - y > 0 always.
        case 3u: {
            // 3 and 5 are the cases with 1 inf in num and 1 inf in den. Use the transformation
            // formula to make them finite.
            // NOTE: the phase here is really just a sign, but it seems tricky to compute this exactly
            // due to potential rounding errors. We are attempting to err on the safe side by using pow()
            // here.
            const auto phase = std::pow(T(-1), x + T(1)) / std::pow(T(-1), y + T(1));
            return compute_3_gamma(-y, -x, x - y + T(1)) * phase;
        }
        case 5u: {
            const auto phase = std::pow(T(-1), x - y + T(1)) / std::pow(T(-1), x + T(1));
            return compute_3_gamma(-(x - y), y + T(1), -x) * phase;
        }
    }
    // Case 7 returns zero -> from inf / (inf * inf) it becomes a / (b * inf) after the transform.
    // NOTE: put it here so the compiler does not complain about missing return statement in the switch block.
    return T(0);
}

// Enabler for the specialisation of binomial for floating-point and arithmetic arguments.
template <typename T, typename U>
using binomial_fp_arith_enabler
    = enable_if_t<conjunction<std::is_arithmetic<T>, std::is_arithmetic<U>,
                              disjunction<std::is_floating_point<T>, std::is_floating_point<U>>>::value>;
}

namespace math
{

/// Default functor for the implementation of piranha::math::binomial().
/**
 * This functor can be specialised via the \p std::enable_if mechanism. The default implementation does not define
 * the call operator, and will thus generate a compile-time error if used.
 */
template <typename T, typename U, typename = void>
struct binomial_impl {
};

/// Specialisation of the implementation of piranha::math::binomial() for floating-point and arithmetic arguments.
/**
 * This specialisation is activated when both arguments are C++ arithmetic types and at least one argument
 * is a floating-point type.
 */
template <typename T, typename U>
struct binomial_impl<T, U, binomial_fp_arith_enabler<T, U>> {
    /// Result type for the call operator.
    /**
     * The result type is the widest floating-point type among \p T and \p U.
     */
    using result_type = typename std::common_type<T, U>::type;
    /// Call operator.
    /**
     * The implementation, accepting any real finite value for \p x and \p y, is described in
     * http://arxiv.org/abs/1105.3689/. Note that, since the implementation uses floating-point
     * arithmetics, the result will - in general - be inexact, even if both \p x and \p y represent
     * integral values.
     *
     * @param x top argument.
     * @param y bottom argument.
     *
     * @return \p x choose \p y.
     *
     * @throws std::invalid_argument if at least one argument is not finite.
     */
    result_type operator()(const T &x, const U &y) const
    {
        return math_fp_binomial(static_cast<result_type>(x), static_cast<result_type>(y));
    }
};
}

inline namespace impl
{

// Determination and enabling of the return type for math::binomial().
template <typename T, typename U>
using math_binomial_type_ = decltype(math::binomial_impl<T, U>{}(std::declval<const T &>(), std::declval<const U &>()));

template <typename T, typename U>
using math_binomial_type = enable_if_t<is_returnable<math_binomial_type_<T, U>>::value, math_binomial_type_<T, U>>;
}

namespace math
{

/// Generalised binomial coefficient.
/**
 * \note
 * This function is enabled only if <tt>binomial_impl<T,U>{}(x,y)</tt> is a valid expression,
 * returning a type which satisfies piranha::is_returnable.
 *
 * Will return the generalised binomial coefficient:
 * \f[
 * {x \choose y}.
 * \f]
 *
 * The actual implementation of this function is in the piranha::math::binomial_impl functor. The body of this
 * function is equivalent to:
 * @code
 * return binomial_impl<T,U>{}(x,y);
 * @endcode
 *
 * @param x top number.
 * @param y bottom number.
 *
 * @return \p x choose \p y.
 *
 * @throws unspecified any exception thrown by the call operator of piranha::math::binomial_impl.
 */
template <typename T, typename U>
inline math_binomial_type<T, U> binomial(const T &x, const U &y)
{
    return binomial_impl<T, U>{}(x, y);
}
}

inline namespace impl
{

// Enabler for the binomial overload involving integral types, the same as pow.
template <typename T, typename U>
using integer_binomial_enabler = integer_pow_enabler<T, U>;

// Wrapper for ADL.
template <typename T, typename U>
auto mp_integer_binomial_wrapper(const T &base, const U &exp) -> decltype(binomial(base, exp))
{
    return binomial(base, exp);
}
}

namespace math
{

/// Specialisation of the implementation of piranha::math::binomial() for piranha::mp_integer.
/**
 * This specialisation is activated when:
 * - one of the arguments is a piranha::mp_integer and the other is either
 *   a piranha::mp_integer with the same static size or an interoperable type for piranha::mp_integer,
 * - both arguments are integral types.
 *
 * The implementation follows these rules:
 * - if the arguments are both piranha::mp_integer with the same static size, or a piranha::mp_integer and an integral
 *   type, then piranha::mp_integer::binomial() is used to compute the result (after any necessary conversion),
 * - if both arguments are integral types, piranha::mp_integer::binomial() is used after the conversion of the top
 *   argument to piranha::integer,
 * - otherwise, the piranha::mp_integer argument is converted to the floating-point type and
 *   piranha::math::binomial() is used to compute the result.
 */
template <typename T, typename U>
struct binomial_impl<T, U, integer_binomial_enabler<T, U>> {
private:
    // integral--integral overload.
    template <typename T2, typename U2,
              enable_if_t<conjunction<std::is_integral<T2>, std::is_integral<U2>>::value, int> = 0>
    static integer impl(const T2 &x, const U2 &y)
    {
        return mp_integer_binomial_wrapper(integer{x}, y);
    }
    // mp_integer--integral, integral--mp_integer, mp_integer--mp_integer overload.
    template <typename T2, typename U2, enable_if_t<disjunction<conjunction<is_mp_integer<T2>, std::is_integral<U2>>,
                                                                conjunction<is_mp_integer<U2>, std::is_integral<T2>>,
                                                                is_same_mp_integer<T2, U2>>::value,
                                                    int> = 0>
    static auto impl(const T2 &x, const U2 &y) -> decltype(mp_integer_binomial_wrapper(x, y))
    {
        return mp_integer_binomial_wrapper(x, y);
    }
    // fp--mp_integer overload.
    template <typename T2, typename U2,
              enable_if_t<conjunction<std::is_floating_point<T2>, is_mp_integer<U2>>::value, int> = 0>
    static T2 impl(const T2 &x, const U2 &y)
    {
        return math::binomial(x, static_cast<T2>(y));
    }
    // mp_integer--fp overload.
    template <typename T2, typename U2,
              enable_if_t<conjunction<std::is_floating_point<U2>, is_mp_integer<T2>>::value, int> = 0>
    static U2 impl(const T2 &x, const U2 &y)
    {
        return math::binomial(static_cast<U2>(x), y);
    }
    // Return type.
    using ret_type = decltype(impl(std::declval<const T &>(), std::declval<const U &>()));

public:
    /// Call operator.
    /**
     * @param x top argument.
     * @param y bottom argument.
     *
     * @returns \f$ x \choose y \f$.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::binomial() or piranha::math::binomial().
     */
    ret_type operator()(const T &x, const U &y) const
    {
        return impl(x, y);
    }
};
}

/// Type trait to detect the presence of the piranha::math::binomial() function.
/**
 * The type trait will be \p true if piranha::math::binomial() can be successfully called on instances of \p T and \p U
 * respectively, \p false otherwise.
 */
template <typename T, typename U>
class has_binomial
{
    template <typename T2, typename U2>
    using binomial_t = decltype(math::binomial(std::declval<const T2 &>(), std::declval<const U2 &>()));
    static const bool implementation_defined = is_detected<binomial_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename T, typename U>
const bool has_binomial<T, U>::value;
}

#endif
