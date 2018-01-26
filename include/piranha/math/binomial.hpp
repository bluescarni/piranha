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

#ifndef PIRANHA_MATH_BINOMIAL_HPP
#define PIRANHA_MATH_BINOMIAL_HPP

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

/// Default functor for the implementation of piranha::math::binomial().
/**
 * This functor can be specialised via the \p std::enable_if mechanism. The default implementation does not define
 * the call operator, and will thus generate a compile-time error if used.
 */
template <typename T, typename U, typename = void>
struct binomial_impl {
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
 * This function will return the generalised binomial coefficient:
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
 * @param x the top argument.
 * @param y the bottom argument.
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

inline namespace impl
{

// Enabler for the binomial overload involving integral types.
template <typename T, typename U>
using integer_binomial_enabler = enable_if_t<
    disjunction<mppp::are_integer_integral_op_types<T, U>,
                conjunction<mppp::is_cpp_integral_interoperable<T>, mppp::is_cpp_integral_interoperable<U>>>::value>;
}

/// Specialisation of the implementation of piranha::math::binomial() for C++ integrals and mp++'s integers.
/**
 * This specialisation is activated when either:
 * - one of the arguments is an mp++ integer and the other one is either an mp++ integer with the same static size or
 *   an integral interoperable type for mp++'s integers, or
 * - both arguments are C++ integral types with which mp++'s integers can interoperate.
 */
template <typename T, typename U>
struct binomial_impl<T, U, integer_binomial_enabler<T, U>> {
private:
    // integral--integral overload.
    template <typename T2, typename U2>
    static integer impl(const T2 &x, const U2 &y, const std::true_type &)
    {
        return mppp::binomial(integer{x}, y);
    }
    // Overload when one of the arguments is an mp++ integer..
    template <typename T2, typename U2>
    static auto impl(const T2 &x, const U2 &y, const std::false_type &) -> decltype(mppp::binomial(x, y))
    {
        return mppp::binomial(x, y);
    }
    // Return type.
    using ret_type
        = decltype(impl(std::declval<const T &>(), std::declval<const U &>(),
                        std::integral_constant<bool, conjunction<mppp::is_cpp_integral_interoperable<T>,
                                                                 mppp::is_cpp_integral_interoperable<U>>::value>{}));

public:
    /// Call operator.
    /**
     * @param x the top argument.
     * @param y the bottom argument.
     *
     * @returns \f$ x \choose y \f$.
     *
     * @throws unspecified any exception thrown by mp++'s binomial function for integral arguments.
     */
    ret_type operator()(const T &x, const U &y) const
    {
        return impl(x, y,
                    std::integral_constant<bool, conjunction<mppp::is_cpp_integral_interoperable<T>,
                                                             mppp::is_cpp_integral_interoperable<U>>::value>{});
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
    static constexpr bool value = implementation_defined;
};

#if PIRANHA_CPLUSPLUS < 201703L

template <typename T, typename U>
constexpr bool has_binomial<T, U>::value;

#endif
}

#endif
