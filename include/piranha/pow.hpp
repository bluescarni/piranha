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

#ifndef PIRANHA_MATH_POW_HPP
#define PIRANHA_MATH_POW_HPP

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/concepts.hpp>
#include <mp++/integer.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/integer.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace math
{

/** @defgroup math_pow_impl math_pow_impl
 *  @{
 */

/// Default functor for the implementation of piranha::math::pow().
/**
 * This functor can be specialised via the \p std::enable_if mechanism. The default implementation does not define
 * the call operator, and will thus generate a compile-time error if used.
 */
template <typename T, typename U, typename Enable = void>
struct pow_impl {
};

/** @} */
}

inline namespace impl
{

// Enabler for math::pow().
template <typename T, typename U>
using math_pow_t_ = decltype(math::pow_impl<T, U>{}(std::declval<const T &>(), std::declval<const U &>()));

template <typename T, typename U>
using math_pow_t = enable_if_t<is_returnable<math_pow_t_<T, U>>::value, math_pow_t_<T, U>>;
}

template <typename T, typename U>
using is_exponentiable = is_detected<math_pow_t, T, U>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool Exponentiable = is_detected<math_pow_t, T, U>::value;

#endif

namespace math
{

/// Exponentiation.
/**
 * \note
 * This function is enabled only if the expression <tt>pow_impl<T, U>{}(x, y)</tt> is valid,
 * returning a type which satisfies piranha::is_returnable.
 *
 * Return \p x to the power of \p y. The actual implementation of this function is in the piranha::math::pow_impl
 * functor's call operator. The body of this function is equivalent to:
 * @code
 * return pow_impl<T, U>{}(x, y);
 * @endcode
 *
 * @param x the base.
 * @param y the exponent.
 *
 * @return \p x to the power of \p y.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::math::pow_impl functor.
 */
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T>
inline auto pow(const Exponentiable<T> &x, const T &y)
#else
template <typename T, typename U>
inline math_pow_t<T, U> pow(const T &x, const U &y)
#endif
{
    return pow_impl<uncvref_t<decltype(x)>, uncvref_t<decltype(y)>>{}(x, y);
}

inline namespace impl
{

// Enabler for the pow overload for arithmetic and floating-point types.
template <typename T, typename U>
using is_pow_fp_arith = conjunction<std::is_arithmetic<T>, std::is_arithmetic<U>,
                                    disjunction<std::is_floating_point<T>, std::is_floating_point<U>>>;

template <typename T, typename U>
using pow_fp_arith_enabler = enable_if_t<is_pow_fp_arith<T, U>::value>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool PowFpArith = is_pow_fp_arith<T, U>::value;

#endif
}

/// Specialisation of the implementation of piranha::math::pow() for arithmetic and floating-point types.
/**
 * \ingroup math_pow_impl
 * This specialisation is activated when both arguments are C++ arithmetic types and at least one argument
 * is a floating-point type.
 */
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, PowFpArith<U> T>
struct pow_impl<T, U> {
#else
template <typename T, typename U>
struct pow_impl<T, U, pow_fp_arith_enabler<T, U>> {
#endif
    /// Call operator.
    /**
     * This operator will compute the exponentiation via one of the overloads of <tt>std::pow()</tt>.
     *
     * @param x the base.
     * @param y the exponent.
     *
     * @return <tt>x**y</tt>.
     */
    auto operator()(const T &x, const U &y) const -> decltype(std::pow(x, y))
    {
        return std::pow(x, y);
    }
};

inline namespace impl
{

// Enabler for integral power.
template <typename T, typename U>
using integer_pow_enabler = enable_if_t<
    disjunction<mppp::are_integer_op_types<T, U>,
                conjunction<mppp::is_cpp_integral_interoperable<T>, mppp::is_cpp_integral_interoperable<U>>>::value>;
}

// NOTE: this specialisation must be here as in the integral-integral overload we use mppp::integer inside,
// so the declaration of mppp::integer must be avaiable. On the other hand, we cannot put this in integer.hpp
// as the integral-integral overload is supposed to work without including mppp::integer.hpp.
/// Specialisation of the implementation of piranha::math::pow() for mp++'s integers and C++ integral types.
/**
 * \ingroup math_pow_impl
 * \note
 * This specialisation is activated when:
 * - the mp++ integer exponentiation function can be successfully called on
 *   instances of ``T`` and ``U``, or
 * - both arguments are C++ integral types with which mp++'s integers can interoperate.
 *
 * The implementation follows these rules:
 * - if both arguments are C++ integral types, then the mp++ integer exponentiation function is used after the
 *   conversion of the base to piranha::integer; otherwise,
 * - the mp++ integer exponentiation function is used directly.
 */
template <typename T, typename U>
struct pow_impl<T, U, integer_pow_enabler<T, U>> {
private:
    // C++ integral -- C++ integral.
    template <typename T2, typename U2>
    static integer impl(const T2 &b, const U2 &e, const std::true_type &)
    {
        return mppp::pow(integer{b}, e);
    }
    // The other cases.
    template <typename T2, typename U2>
    static auto impl(const T2 &b, const U2 &e, const std::false_type &) -> decltype(mppp::pow(b, e))
    {
        return mppp::pow(b, e);
    }
    using ret_type
        = decltype(impl(std::declval<const T &>(), std::declval<const U &>(),
                        std::integral_constant<bool, conjunction<mppp::is_cpp_integral_interoperable<T>,
                                                                 mppp::is_cpp_integral_interoperable<U>>::value>{}));

public:
    /// Call operator.
    /**
     * @param b the base.
     * @param e the exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by mp++'s integer exponentiation function.
     */
    ret_type operator()(const T &b, const U &e) const
    {
        return impl(b, e,
                    std::integral_constant<bool, conjunction<mppp::is_cpp_integral_interoperable<T>,
                                                             mppp::is_cpp_integral_interoperable<U>>::value>{});
    }
};
}
}

#endif
