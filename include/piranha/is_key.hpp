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

#ifndef PIRANHA_IS_KEY_HPP
#define PIRANHA_IS_KEY_HPP

#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Key type concept check.
/**
 * This type trait will be \p true if \p T satisfies all the requirements of a key type, \p false otherwise.
 *
 * Key types must implement the following methods:
 * @code{.unparsed}
 * bool is_compatible(const symbol_fset &) const noexcept;
 * bool is_zero(const symbol_fset &) const noexcept;
 * bool is_unitary(const symbol_fset &) const;
 * T merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
 * void print(std::ostream &, const symbol_fset &) const;
 * void print_tex(std::ostream &, const symbol_fset &) const;
 * void trim_identify(std::vector<char> &, const symbol_fset &) const;
 * T trim(const symbol_idx_fset &, const symbol_fset &) const;
 * @endcode
 * Additionally, \p T must also be constructible from a const piranha::symbol_fset reference and satisfy the following
 * type traits: piranha::is_container_element, piranha::is_equality_comparable and piranha::is_hashable.
 */
// \todo requirements on vector-of-symbols-constructed key: must it be unitary? (seems like it, look at
// polynomial ctors from symbol) -> note that both these two checks have to go in the runtime requirements of key
// when they get documented.
template <typename T>
class is_key
{
    template <typename U>
    using is_compatible_t = decltype(std::declval<const U &>().is_compatible(std::declval<symbol_fset const &>()));
    template <typename U>
    using is_zero_t = decltype(std::declval<const U &>().is_zero(std::declval<symbol_fset const &>()));
    template <typename U>
    using merge_symbols_t = decltype(std::declval<const U &>().merge_symbols(
        std::declval<const symbol_idx_fmap<symbol_fset> &>(), std::declval<symbol_fset const &>()));
    template <typename U>
    using is_unitary_t = decltype(std::declval<const U &>().is_unitary(std::declval<symbol_fset const &>()));
    template <typename U>
    using print_t = decltype(
        std::declval<const U &>().print(std::declval<std::ostream &>(), std::declval<symbol_fset const &>()));
    template <typename U>
    using print_tex_t = decltype(
        std::declval<const U &>().print_tex(std::declval<std::ostream &>(), std::declval<symbol_fset const &>()));
    template <typename U>
    using trim_identify_t = decltype(std::declval<const U &>().trim_identify(std::declval<std::vector<char> &>(),
                                                                             std::declval<symbol_fset const &>()));
    template <typename U>
    using trim_t = decltype(
        std::declval<const U &>().trim(std::declval<const symbol_idx_fset &>(), std::declval<symbol_fset const &>()));
    template <typename U>
    using check_methods_t = std::integral_constant<bool, conjunction<std::is_same<detected_t<is_compatible_t, U>, bool>,
                                                                     std::is_same<detected_t<is_zero_t, U>, bool>,
                                                                     std::is_same<detected_t<merge_symbols_t, U>, U>,
                                                                     std::is_same<detected_t<is_unitary_t, U>, bool>,
                                                                     std::is_same<detected_t<print_t, U>, void>,
                                                                     std::is_same<detected_t<print_tex_t, U>, void>,
                                                                     std::is_same<detected_t<trim_identify_t, U>, void>,
                                                                     std::is_same<detected_t<trim_t, U>, U>>::value>;
    template <typename U, typename = void>
    struct is_key_impl {
        static const bool value = false;
    };
    template <typename U>
    struct is_key_impl<U, enable_if_t<check_methods_t<U>::value>> {
        static const bool value
            = conjunction<is_container_element<U>, std::is_constructible<U, const symbol_fset &>,
                          is_equality_comparable<U>, is_hashable<U>>::value
              && noexcept(std::declval<const U &>().is_compatible(std::declval<const symbol_fset &>()))
              && noexcept(std::declval<const U &>().is_zero(std::declval<const symbol_fset &>()));
    };
    static const bool implementation_defined = is_key_impl<T>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_key<T>::value;
}

#endif
