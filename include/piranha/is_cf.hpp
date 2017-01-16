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

#ifndef PIRANHA_IS_CF_HPP
#define PIRANHA_IS_CF_HPP

#include <type_traits>

#include <piranha/math.hpp>
#include <piranha/print_coefficient.hpp>
#include <piranha/print_tex_coefficient.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Type trait to detect coefficient types.
/**
 * This type trait will be \p true if \p T can be used as a coefficient type, \p false otherwise.
 * The requisites for \p T are the following:
 *
 * - it must satisfy piranha::is_container_element,
 * - it must satisfy piranha::has_print_coefficient and piranha::has_print_tex_coefficient,
 * - it must satisfy piranha::has_is_zero and piranha::has_negate,
 * - it must be equality comparable,
 * - it must be addable and subtractable (both binary and in-place forms),
 * - it must be constructible from integer numerals.
 */
template <typename T>
class is_cf
{
    static const bool implementation_defined
        = is_container_element<T>::value && has_print_coefficient<T>::value && has_print_tex_coefficient<T>::value
          && has_is_zero<T>::value && has_negate<T>::value && is_equality_comparable<T>::value && is_addable<T>::value
          && is_addable_in_place<T>::value && is_subtractable_in_place<T>::value && is_subtractable<T>::value
          && std::is_constructible<T, int>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_cf<T>::value;
}

#endif
