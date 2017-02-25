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

#ifndef PIRANHA_KEY_IS_CONVERTIBLE_HPP
#define PIRANHA_KEY_IS_CONVERTIBLE_HPP

#include <type_traits>

#include <piranha/is_key.hpp>
#include <piranha/symbol_set.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Detect if a key type is convertible to another key type.
/**
 * This type trait will be \p true if the key type \p To can be constructed
 * from a const reference to the key type \p From and a const reference to
 * piranha::symbol_set, \p false otherwise.
 *
 * The decay types of \p To and \p From are considered by this type trait. \p To and \p From must satisfy
 * piranha::is_key, otherwise a compile-time error will be generated.
 */
template <typename To, typename From>
class key_is_convertible
{
    using Tod = typename std::decay<To>::type;
    using Fromd = typename std::decay<From>::type;
    PIRANHA_TT_CHECK(is_key, Tod);
    PIRANHA_TT_CHECK(is_key, Fromd);

public:
    /// Value of the type trait.
    static const bool value = std::is_constructible<Tod, const Fromd &, const symbol_set &>::value;
};

template <typename To, typename From>
const bool key_is_convertible<To, From>::value;
}

#endif
