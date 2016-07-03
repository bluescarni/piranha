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

#ifndef PIRANHA_DETAIL_PREPARE_FOR_PRINT_HPP
#define PIRANHA_DETAIL_PREPARE_FOR_PRINT_HPP

#include <type_traits>

namespace piranha {

namespace detail {

// Helper to print char types without displaying garbage.
template <typename T> struct pfp_special {
  static const bool value = std::is_same<T, char>::value ||
                            std::is_same<T, unsigned char>::value ||
                            std::is_same<T, signed char>::value;
};

template <typename T>
inline const T &prepare_for_print(
    const T &x,
    typename std::enable_if<!pfp_special<T>::value>::type * = nullptr) {
  return x;
}

template <typename T>
inline int prepare_for_print(
    const T &n,
    typename std::enable_if<pfp_special<T>::value &&
                            std::is_signed<T>::value>::type * = nullptr) {
  return static_cast<int>(n);
}

template <typename T>
inline unsigned prepare_for_print(
    const T &n,
    typename std::enable_if<pfp_special<T>::value &&
                            std::is_unsigned<T>::value>::type * = nullptr) {
  return static_cast<unsigned>(n);
}
}
}

#endif
