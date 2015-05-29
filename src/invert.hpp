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

#ifndef PIRANHA_INVERT_HPP
#define PIRANHA_INVERT_HPP

#include <type_traits>

#include "detail/sfinae_types.hpp"
#include "pow.hpp"

namespace piranha
{

namespace math
{

template <typename T, typename = void>
struct invert_impl
{
	template <typename U>
	auto operator()(const U &x) const -> decltype(math::pow(x,-1))
	{
		return math::pow(x,-1);
	}
};

/// Compute the inverse.
/**
 * Return the multiplicative inverse of \p x. The actual implementation of this function is in the piranha::math::invert_impl functor's
 * call operator.
 *
 * @param[in] x argument.
 *
 * @return the multiplicative inverse of \p x.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::math::invert_impl functor.
 */
template <typename T>
inline auto invert(const T &x) -> decltype(invert_impl<T>()(x))
{
	return invert_impl<T>()(x);
}

}

/// Type trait for invertible types.
/**
 * The type trait will be \p true if piranha::math::invert() can be successfully called with on type \p T.
 *
 * The call to piranha::math::invert() will be tested with const reference arguments.
 */
template <typename T>
class is_invertible: detail::sfinae_types
{
		template <typename T1>
		static auto test(const T1 &x) -> decltype(math::invert(x),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool is_invertible<T>::value;

}

#endif
