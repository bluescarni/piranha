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

#ifndef PIRANHA_CONVERT_TO_HPP
#define PIRANHA_CONVERT_TO_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Default functor for the implementation of piranha::convert_to().
template <typename To, typename From, typename = void>
struct convert_to_impl {
private:
    template <typename From2>
    using return_type_ = decltype(static_cast<To>(std::declval<const From2 &>()));
    template <typename From2>
    using return_type = enable_if_t<is_returnable<return_type_<From2>>::value, return_type_<From2>>;

public:
    /// Call operator.
    /**
     * \note
     * This call operator is enabled only if <tt>static_cast<To>(x)</tt> is a well-formed
     * expression, returning a type which satisfies piranha::is_returnable.
     *
     * The body of this operator is equivalent to:
     * @code
     * return static_cast<To>(x);
     * @endcode
     *
     * @param x conversion argument.
     *
     * @return an instance of type \p To cast from \p x.
     *
     * @throws unspecified any exception thrown by casting \p x to \p To.
     */
    template <typename From2>
    return_type<From2> operator()(const From2 &x) const
    {
        return static_cast<To>(x);
    }
};

namespace detail
{

// Enabler for the generic conversion function. We need to check that the impl functor is callable
// and that it returns the proper type.
template <typename To, typename From>
using convert_to_enabler = enable_if_t<
    conjunction<
        std::is_same<decltype(convert_to_impl<uncvref_t<To>, From>{}(std::declval<const From &>())), uncvref_t<To>>,
        is_returnable<uncvref_t<To>>>::value,
    int>;
}

/// Generic conversion function.
/**
 * \note
 * This function is enabled only if the call operator of piranha::convert_to_impl returns a instance of
 * ``To``, after the removal of reference and const qualifiers.
 *
 * This function is meant to convert an instance of type \p From to an instance of ``To``, after the removal of
 * reference and const qualifiers. It is intended to be a user-extensible replacement for \p static_cast limited to
 * value conversions.
 *
 * The actual implementation of this function is in the piranha::convert_to_impl functor's
 * call operator. ``To`` is passed as first template parameter of
 * piranha::convert_to_impl, after the removal of reference and const qualifiers, whereas \p From is passed as-is. The
 * body of this function is equivalent to:
 * @code
 * return convert_to_impl<uncvref_t<To>,From>{}(x);
 * @endcode
 *
 * @param x conversion argument.
 *
 * @returns ``x`` converted to ``To``.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::convert_to_impl functor.
 */
template <typename To, typename From, detail::convert_to_enabler<To, From> = 0>
inline To convert_to(const From &x)
{
    return convert_to_impl<uncvref_t<To>, From>{}(x);
}

/// Type trait to detect piranha::convert_to().
/**
 * The type trait will be \p true if piranha::convert_to() can be called with \p To and \p From
 * as template arguments, \p false otherwise.
 */
template <typename To, typename From>
class has_convert_to
{
    template <typename To1, typename From1>
    using convert_to_t = decltype(convert_to<To1>(std::declval<const From1 &>()));
    static const bool implementation_defined = is_detected<convert_to_t, uncvref_t<To>, uncvref_t<From>>::value;

public:
    /// Value of the type trait.
    static constexpr bool value = implementation_defined;
};

#if PIRANHA_CPLUSPLUS < 201703L

// Static init.
template <typename To, typename From>
constexpr bool has_convert_to<To, From>::value;

#endif
}

#endif
