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

#ifndef PIRANHA_INVERT_HPP
#define PIRANHA_INVERT_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace math
{

/// Default functor for the implementation of piranha::math::invert().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. The call operator
 * will call piranha::math::pow() with an exponent of <tt>-1</tt>, and it is enabled only
 * if integral exponentiation is supported by the argument type.
 */
template <typename T, typename = void>
struct invert_impl {
    /// Call operator.
    /**
     * @param x argument for the inversion,
     *
     * @return piranha::math::pow(x,-1).
     *
     * @throws unspecified any exception thrown by piranha::math::pow().
     */
    template <typename U>
    auto operator()(const U &x) const -> decltype(math::pow(x, -1))
    {
        return math::pow(x, -1);
    }
};

/// Compute the inverse.
/**
 * Return the multiplicative inverse of \p x. The actual implementation of this function is in the
 * piranha::math::invert_impl functor's call operator.
 *
 * @param x argument.
 *
 * @return the multiplicative inverse of \p x.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::math::invert_impl functor.
 */
template <typename T>
inline auto invert(const T &x) -> decltype(invert_impl<T>{}(x))
{
    return invert_impl<T>{}(x);
}
}

/// Type trait for invertible types.
/**
 * The type trait will be \p true if piranha::math::invert() can be successfully called with on type \p T.
 *
 * The call to piranha::math::invert() will be tested with const reference arguments.
 */
template <typename T>
class is_invertible
{
    template <typename T1>
    using invert_t = decltype(math::invert(std::declval<const T1 &>()));
    static const bool implementation_defined = is_detected<invert_t, T>::value;

public:
    /// Value of the type trait.
    static constexpr bool value = implementation_defined;
};

#if PIRANHA_CPLUSPLUS < 201703L

template <typename T>
constexpr bool is_invertible<T>::value;

#endif
}

#endif
