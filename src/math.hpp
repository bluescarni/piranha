/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PIRANHA_MATH_HPP
#define PIRANHA_MATH_HPP

#include <boost/type_traits/is_complex.hpp>

#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Default implementation of math::is_zero.
// NOTE: the technique and its motivations are well-described here:
// http://www.gotw.ca/publications/mill17.htm
template <typename T, typename Enable = void>
struct math_is_zero_impl
{
	static bool run(const T &x)
	{
		return x == 0;
	}
};

// Handle std::complex types.
template <typename T>
struct math_is_zero_impl<T,typename std::enable_if<boost::is_complex<T>::value>::type>
{
	static bool run(const T &c)
	{
		return math_is_zero_impl<typename T::value_type>::run(c.real()) &&
			math_is_zero_impl<typename T::value_type>::run(c.imag());
	}
};

// Default implementation of math::negate.
template <typename T, typename Enable = void>
struct math_negate_impl
{
	static void run(T &x)
	{
		x = -x;
	}
};

}

/// Math namespace.
/**
 * Namespace for general-purpose mathematical functions.
 */
namespace math
{

/// Zero test.
/**
 * Test if value is zero. This function works with all C++ arithmetic types
 * and with piranha's numerical types.
 * 
 * @param[in] x value to be tested.
 * 
 * @return \p true if value is zero, \p false otherwise.
 */
template <typename T>
inline bool is_zero(const T &x)
{
	return piranha::detail::math_is_zero_impl<T>::run(x);
}

/// In-place negation.
/**
 * Negate value in-place. This function works with all C++ arithmetic types
 * and with piranha's numerical types.
 * 
 * @param[in,out] x value to be negated.
 */
template <typename T>
inline void negate(T &x)
{
	piranha::detail::math_negate_impl<typename strip_cv_ref<T>::type>::run(x);
}

}

}

#endif
