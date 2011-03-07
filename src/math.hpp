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

#include <type_traits>

#include "config.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Math namespace.
/**
 * Namespace for general-purpose mathematical functions.
 */
namespace math
{

/// Zero test.
/**
 * This template function is activated iff \p T is an arithmetic type.
 * 
 * @param[in] x value to be tested.
 * 
 * @return <tt>x == static_cast<T>(0)</tt>.
 */
template <typename T>
inline bool is_zero(const T &x, typename std::enable_if<
	std::is_arithmetic<typename strip_cv_ref<T>::type>::value
	>::type * = piranha_nullptr)
{
	return x == static_cast<T>(0);
}

/// In-place negation.
/**
 * This template function is activated iff \p T is an arithmetic type.
 * Equivalent to <tt>x = -x</tt>.
 * 
 * @param[in,out] x value to be negated.
 */
template <typename T>
inline void negate(T &x, typename std::enable_if<
	std::is_arithmetic<typename strip_cv_ref<T>::type>::value
	>::type * = piranha_nullptr)
{
	x = -x;
}

}

}

#endif
