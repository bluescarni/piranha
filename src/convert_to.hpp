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

#ifndef PIRANHA_CONVERT_TO_HPP
#define PIRANHA_CONVERT_TO_HPP

#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Default functor for the implementation of piranha::convert_to().
template <typename To, typename From, typename = void>
struct convert_to_impl {
private:
    template <typename From2>
    using return_type_ = decltype(static_cast<To>(std::declval<const From2 &>()));
    template <typename From2>
    using return_type = typename std::enable_if<is_returnable<return_type_<From2>>::value, return_type_<From2>>::type;

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
     * @param[in] x conversion argument.
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
using convert_to_enabler =
    typename std::enable_if<std::is_same<decltype(convert_to_impl<typename std::decay<To>::type, From>{}(
                                             std::declval<const From &>())),
                                         typename std::decay<To>::type>::value
                                && is_returnable<typename std::decay<To>::type>::value,
                            int>::type;
}

/// Generic conversion function.
/**
 * \note
 * This function is enabled only if the call operator of piranha::convert_to_impl returns a instance of
 * the decay type of \p To, and if the decay type of \p To satisfies piranha::is_returnable.
 *
 * This function is meant to convert an instance of type \p From to an instance of the decay
 * type of \p To. It is intended to be a user-extensible replacement for \p static_cast
 * limited to value conversions (that is, the decay types of \p To and \p From are considered).
 *
 * The actual implementation of this function is in the piranha::convert_to_impl functor's
 * call operator. The decay type of \p To is passed as first template parameter of
 * piranha::convert_to_impl, whereas \p From is passed as-is. The body of this function
 * is equivalent to:
 * @code
 * return convert_to_impl<typename std::decay<To>::type,From>{}(x);
 * @endcode
 *
 * Any specialisation of piranha::convert_to_impl must have a call operator returning
 * an instance of the decay type of \p To, otherwise this function will be disabled.
 *
 * @param[in] x conversion argument.
 *
 * @returns an instance of the decay type of \p To converted from \p x.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::convert_to_impl functor.
 */
template <typename To, typename From, detail::convert_to_enabler<To, From> = 0>
inline To convert_to(const From &x)
{
    return convert_to_impl<typename std::decay<To>::type, From>{}(x);
}

/// Type trait to detect piranha::convert_to().
/**
 * The type trait will be \p true if piranha::convert_to() can be called with the decay types of \p To and \p From
 * as template arguments, \p false otherwise.
 */
template <typename To, typename From>
class has_convert_to : detail::sfinae_types
{
    using Tod = typename std::decay<To>::type;
    using Fromd = typename std::decay<From>::type;
    template <typename To1, typename From1>
    static auto test(const To1 &, const From1 &x) -> decltype(convert_to<To1>(x), void(), yes());
    static no test(...);
    static const bool implementation_defined
        = std::is_same<decltype(test(std::declval<Tod>(), std::declval<Fromd>())), yes>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename To, typename From>
const bool has_convert_to<To, From>::value;
}

#endif
