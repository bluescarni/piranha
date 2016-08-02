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

#ifndef PIRANHA_IS_KEY_HPP
#define PIRANHA_IS_KEY_HPP

#include <iostream>
#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"
// NOTE: include directly symbol_set.hpp once its dependency from serialization.hpp is removed.
#include "detail/symbol_set_fwd.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Test the presence of requested key methods.
template <typename T>
struct is_key_impl : sfinae_types {
    template <typename U>
    static auto test0(const U &u) -> decltype(u.is_compatible(std::declval<symbol_set const &>()));
    static no test0(...);
    template <typename U>
    static auto test1(const U &u) -> decltype(u.is_ignorable(std::declval<symbol_set const &>()));
    static no test1(...);
    template <typename U>
    static auto test2(const U &u)
        -> decltype(u.merge_args(std::declval<symbol_set const &>(), std::declval<symbol_set const &>()));
    static no test2(...);
    template <typename U>
    static auto test3(const U &u) -> decltype(u.is_unitary(std::declval<symbol_set const &>()));
    static no test3(...);
    template <typename U>
    static auto test4(const U &u)
        -> decltype(u.print(std::declval<std::ostream &>(), std::declval<symbol_set const &>()), void(), yes());
    static no test4(...);
    template <typename U>
    static auto test5(const U &u)
        -> decltype(u.print_tex(std::declval<std::ostream &>(), std::declval<symbol_set const &>()), void(), yes());
    static no test5(...);
    template <typename U>
    static auto test6(const U &u)
        -> decltype(u.trim_identify(std::declval<symbol_set &>(), std::declval<symbol_set const &>()), void(), yes());
    static no test6(...);
    template <typename U>
    static auto test7(const U &u)
        -> decltype(u.trim(std::declval<symbol_set const &>(), std::declval<symbol_set const &>()));
    static no test7(...);
    static const bool value = std::is_same<bool, decltype(test0(std::declval<T>()))>::value
                              && std::is_same<bool, decltype(test1(std::declval<T>()))>::value
                              && std::is_same<T, decltype(test2(std::declval<T>()))>::value
                              && std::is_same<bool, decltype(test3(std::declval<T>()))>::value
                              && std::is_same<yes, decltype(test4(std::declval<T>()))>::value
                              && std::is_same<yes, decltype(test5(std::declval<T>()))>::value
                              && std::is_same<yes, decltype(test6(std::declval<T>()))>::value
                              && std::is_same<T, decltype(test7(std::declval<T>()))>::value;
};
}

/// Type trait to detect key types.
/**
 * This type trait will be \p true if \p T can be used as a key type, \p false otherwise.
 * The requisites for \p T are the following:
 *
 * - it must satisfy piranha::is_container_element,
 * - it must be constructible from a const piranha::symbol_set reference,
 * - it must satisfy piranha::is_equality_comparable,
 * - it must satisfy piranha::is_hashable,
 * - it must be provided with a const non-throwing \p is_compatible method accepting a const piranha::symbol_set
 *   reference as input and returning a \p bool,
 * - it must be provided with a const non-throwing \p is_ignorable method accepting a const piranha::symbol_set
 *   reference as input and returning \p bool,
 * - it must be provided with a const \p merge_args method accepting two const piranha::symbol_set
 *   references as inputs and returning \p T,
 * - it must be provided with a const \p is_unitary method accepting a const piranha::symbol_set
 *   reference as input and returning \p bool,
 * - it must be provided with const \p print and \p print_tex methods accepting an \p std::ostream reference as first
 * argument
 *   and a const piranha::symbol_set reference as second argument,
 * - it must be provided with a const \p trim_identify method accepting a reference to piranha::symbol_set and a const
 * reference
 *   to piranha::symbol_set,
 * - it must be provided with a const \p trim method accepting a const reference to piranha::symbol_set and a const
 * reference
 *   to piranha::symbol_set, and returning \p T.
 */
// \todo requirements on vector-of-symbols-constructed key: must it be unitary? (seems like it, look at
// polynomial ctors from symbol) -> note that both these two checks have to go in the runtime requirements of key
// when they get documented.
template <typename T, typename = void>
class is_key
{
    static const bool implementation_defined = false;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
class is_key<T, typename std::enable_if<detail::is_key_impl<T>::value>::type>
{
    static const bool implementation_defined
        = is_container_element<T>::value && std::is_constructible<T, const symbol_set &>::value
          && is_equality_comparable<T>::value && is_hashable<T>::value
          && noexcept(std::declval<T const &>().is_compatible(std::declval<symbol_set const &>()))
          && noexcept(std::declval<T const &>().is_ignorable(std::declval<symbol_set const &>()));

public:
    static const bool value = implementation_defined;
};

template <typename T, typename Enable>
const bool is_key<T, Enable>::value;

template <typename T>
const bool is_key<T, typename std::enable_if<detail::is_key_impl<T>::value>::type>::value;
}

#endif
