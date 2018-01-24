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

#ifndef PIRANHA_PRINT_TEX_COEFFICIENT_HPP
#define PIRANHA_PRINT_TEX_COEFFICIENT_HPP

#include <iostream>
#include <utility>

#include <piranha/detail/init.hpp>
#include <piranha/print_coefficient.hpp>

namespace piranha
{

/// Default functor for piranha::print_tex_coefficient().
/**
 * This functor should be specialised via the \p std::enable_if mechanism.
 */
template <typename T, typename = void>
struct print_tex_coefficient_impl {
    /// Call operator.
    /**
     * The default call operator will call piranha::print_coefficient.
     *
     * @param os target stream.
     * @param cf coefficient to be printed.
     *
     * @return the value returned by piranha::print_coefficient.
     *
     * @throws unspecified any exception thrown by piranha::print_coefficient.
     */
    template <typename U>
    auto operator()(std::ostream &os, const U &cf) const -> decltype(print_coefficient(os, cf))
    {
        return print_coefficient(os, cf);
    }
};

/// Print series coefficient in TeX mode.
/**
 * This function is used in to print coefficients in TeX mode.
 *
 * The implementation uses the call operator of the piranha::print_tex_coefficient_impl functor.
 * Specialisations of piranha::print_tex_coefficient_impl can be defined to customize the behaviour.
 *
 * @param os target stream.
 * @param cf coefficient object to be printed.
 *
 * @return the value returned by the call operator of piranha::print_tex_coefficient_impl.
 *
 * @throws unspecified any exception thrown by the call operator of piranha::print_tex_coefficient_impl.
 */
template <typename T>
inline auto print_tex_coefficient(std::ostream &os, const T &cf) -> decltype(print_tex_coefficient_impl<T>()(os, cf))
{
    return print_tex_coefficient_impl<T>()(os, cf);
}

/// Type trait for classes implementing piranha::print_tex_coefficient.
/**
 * This type trait will be \p true if piranha::print_tex_coefficient can be called on instances of type \p T,
 * \p false otherwise.
 */
template <typename T>
class has_print_tex_coefficient
{
    template <typename T1>
    using print_tex_coefficient_t
        = decltype(piranha::print_tex_coefficient(std::declval<std::ostream &>(), std::declval<const T1 &>()));
    static const bool implementation_defined = is_detected<print_tex_coefficient_t, T>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool has_print_tex_coefficient<T>::value;
}

#endif
