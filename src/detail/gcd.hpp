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

#include "../mp_integer.hpp"

namespace piranha
{

namespace detail
{

template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
inline T gcd(const T &a, const T &b)
{
	return gcd_euclidean(a,b);
}

template <typename T, typename std::enable_if<is_mp_integer<T>::value,int>::type = 0>
inline T gcd(const T &a, const T &b)
{
	return T::gcd(a,b);
}

}

}

#endif
