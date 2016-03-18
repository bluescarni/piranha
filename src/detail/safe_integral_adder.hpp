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

#ifndef PIRANHA_DETAIL_SAFE_INTEGRAL_ADDER_HPP
#define PIRANHA_DETAIL_SAFE_INTEGRAL_ADDER_HPP

#include <limits>
#include <stdexcept>
#include <type_traits>

#include "../config.hpp"
#include "../exceptions.hpp"

namespace piranha
{

namespace detail
{

// An overloaded helper function to perform safely the addition in-place of two integral values. It will throw
// std::overflow_error in case of out-of-range conditions.
template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value,int>::type = 0>
inline void safe_integral_adder(T &a, const T &b)
{
	if (b >= T(0)) {
		if (unlikely(a > std::numeric_limits<T>::max() - b)) {
			piranha_throw(std::overflow_error,"overflow in the addition of two signed integrals");
		}
	} else {
		if (unlikely(a < std::numeric_limits<T>::min() - b)) {
			piranha_throw(std::overflow_error,"overflow in the addition of two signed integrals");
		}
	}
	a = static_cast<T>(a + b);
}

template <typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_signed<T>::value,int>::type = 0>
inline void safe_integral_adder(T &a, const T &b)
{
	if (unlikely(a > std::numeric_limits<T>::max() - b)) {
		piranha_throw(std::overflow_error,"overflow in the addition of two unsigned integrals");
	}
	a = static_cast<T>(a + b);
}

template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value,int>::type = 0>
inline void safe_integral_subber(T &a, const T &b)
{
	if (b <= T(0)) {
		if (unlikely(a > std::numeric_limits<T>::max() + b)) {
			piranha_throw(std::overflow_error,"overflow in the subtraction of two signed integrals");
		}
	} else {
		if (unlikely(a < std::numeric_limits<T>::min() + b)) {
			piranha_throw(std::overflow_error,"overflow in the subtraction of two signed integrals");
		}
	}
	a = static_cast<T>(a - b);
}

template <typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_signed<T>::value,int>::type = 0>
inline void safe_integral_subber(T &a, const T &b)
{
	if (unlikely(a < b)) {
		piranha_throw(std::overflow_error,"overflow in the subtraction of two unsigned integrals");
	}
	a = static_cast<T>(a - b);
}

}

}

#endif
