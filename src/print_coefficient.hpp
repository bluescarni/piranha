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

#ifndef PIRANHA_PRINT_COEFFICIENT_HPP
#define PIRANHA_PRINT_COEFFICIENT_HPP

#include <iostream>
#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"

namespace piranha {

/// Default functor for piranha::print_coefficient().
/**
 * This functor should be specialised via the \p std::enable_if mechanism.
 */
template <typename T, typename = void> struct print_coefficient_impl {
  /// Call operator.
  /**
   * \note
   * This operator is enabled only if the expression <tt>os << cf</tt> is
   * well-formed.
   *
   * The default call operator will print to stream the object.
   *
   * @param[in] os target stream.
   * @param[in] cf coefficient to be printed.
   *
   * @return the value returned by the stream insertion operator of \p U.
   *
   * @throws unspecified any exception thrown by printing \p cf to stream \p os.
   */
  template <typename U>
  auto operator()(std::ostream &os, const U &cf) const -> decltype(os << cf) {
    return os << cf;
  }
};

/// Print series coefficient.
/**
 * This function is used in the stream operator overload for piranha::series
 * when
 * printing coefficients.
 *
 * The implementation uses the call operator of the
 * piranha::print_coefficient_impl functor.
 * Specialisations of piranha::print_coefficient_impl can be defined to
 * customize the behaviour.
 *
 * @param[in] os target stream.
 * @param[in] cf coefficient object to be printed.
 *
 * @return the value returned by the call operator of
 * piranha::print_coefficient_impl.
 *
 * @throws unspecified any exception thrown by the call operator of
 * piranha::print_coefficient_impl.
 */
template <typename T>
inline auto print_coefficient(std::ostream &os, const T &cf)
    -> decltype(print_coefficient_impl<T>()(os, cf)) {
  return print_coefficient_impl<T>()(os, cf);
}

/// Type trait for classes implementing piranha::print_coefficient.
/**
 * This type trait will be \p true if piranha::print_coefficient can be called
 * on instances of type \p T,
 * \p false otherwise.
 */
template <typename T> class has_print_coefficient : detail::sfinae_types {
  template <typename T1>
  static auto test(std::ostream &os, const T1 &t)
      -> decltype(piranha::print_coefficient(os, t), void(), yes());
  static no test(...);

public:
  /// Value of the type trait.
  static const bool value =
      std::is_same<decltype(test(*(std::ostream *)nullptr, std::declval<T>())),
                   yes>::value;
};

template <typename T> const bool has_print_coefficient<T>::value;
}

#endif
