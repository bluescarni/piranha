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

#ifndef PIRANHA_DETAIL_GCD_HPP
#define PIRANHA_DETAIL_GCD_HPP

#include <type_traits>

#include "../math.hpp"
#include "../mp_integer.hpp"

namespace piranha
{

namespace detail
{

template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
inline void gcd_mod(T &a, const T &b)
{
	a %= b;
}

template <typename T, typename std::enable_if<!detail::is_mp_integer<T>::value,int>::type = 0>
inline void gcd_mod(T &a, const T &b)
{
	a = static_cast<T>(a % b);
}

// Greatest common divisor using the euclidean algorithm.
// NOTE: this can yield negative values, depending on the signs
// of a and b. Supports C++ integrals and mp_integer.
// NOTE: using this with C++ integrals unchecked on ranges can result in undefined
// behaviour.
template <typename T>
inline T gcd(T a, T b)
{
	while (true) {
		if (math::is_zero(a)) {
			return b;
		}
		// NOTE: the difference in implementation here is because
		// we want to prevent compiler warnings when T is a short int,
		// hence the static cast. For mp_integer, the in-place version
		// might be faster.
		gcd_mod(b,a);
		if (math::is_zero(b)) {
			return a;
		}
		gcd_mod(a,b);
	}
}

}

}

#endif
