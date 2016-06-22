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

#ifndef PIRANHA_DETAIL_ULSHIFT_HPP
#define PIRANHA_DETAIL_ULSHIFT_HPP

#include <limits>
#include <type_traits>

#include "../config.hpp"

namespace piranha
{
namespace detail
{

// Wrapper for left shifting of unsigned integrals.
// This method exists to prevent potential UB when left shifting short unsigned integral type (e.g., unsigned short,
// unsigned char). In this situation,
// the left operand might be promoted to int because of integral promotions. Then we are left shifting on a signed type,
// and we could run in undefined
// behaviour because the excessive shifting for signed types is UB. The idea is then to cast n to unsigned
// if the range of Uint is not greater than the range of unsigned (which means that Uint *might* be a short integral
// type), perform the shift
// which is now guaranteed to involve only unsigned types, and finally cast back the result to Uint (which is also
// always a well-defined operation
// on unsigned types). All this should be well optimised by the compiler.
template <typename Uint, typename Uint2,
          typename std::enable_if<std::numeric_limits<Uint>::max() <= std::numeric_limits<unsigned>::max(), int>::type
          = 0>
inline Uint ulshift(Uint n, Uint2 s)
{
    static_assert(std::is_integral<Uint>::value && std::is_unsigned<Uint>::value, "Invalid type.");
    static_assert(std::is_integral<Uint2>::value && std::is_unsigned<Uint2>::value, "Invalid type.");
    piranha_assert(s < unsigned(std::numeric_limits<Uint>::digits));
    return static_cast<Uint>(static_cast<unsigned>(n) << s);
}

// If the range of Uint is greater than the range of unsigned, it cannot possibly be a short integral type.
template <typename Uint, typename Uint2,
          typename std::enable_if<(std::numeric_limits<Uint>::max() > std::numeric_limits<unsigned>::max()), int>::type
          = 0>
inline Uint ulshift(Uint n, Uint2 s)
{
    static_assert(std::is_integral<Uint>::value && std::is_unsigned<Uint>::value, "Invalid type.");
    static_assert(std::is_integral<Uint2>::value && std::is_unsigned<Uint2>::value, "Invalid type.");
    piranha_assert(s < unsigned(std::numeric_limits<Uint>::digits));
    return n << s;
}
}
}

#endif
