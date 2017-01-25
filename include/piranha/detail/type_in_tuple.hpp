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

#ifndef PIRANHA_DETAIL_TYPE_IN_TUPLE_HPP
#define PIRANHA_DETAIL_TYPE_IN_TUPLE_HPP

#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>

namespace piranha
{
namespace detail
{

// TMP to check if a type is in the tuple.
template <typename T, typename Tuple, std::size_t I = 0u, typename Enable = void>
struct type_in_tuple {
    static_assert(I < std::numeric_limits<std::size_t>::max(), "Overflow error.");
    static const bool value
        = std::is_same<T, typename std::tuple_element<I, Tuple>::type>::value || type_in_tuple<T, Tuple, I + 1u>::value;
};
template <typename T, typename Tuple, std::size_t I>
struct type_in_tuple<T, Tuple, I, typename std::enable_if<I == std::tuple_size<Tuple>::value>::type> {
    static const bool value = false;
};
}
}

#endif
