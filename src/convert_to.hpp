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

#ifndef PIRANHA_CONVERT_TO_HPP
#define PIRANHA_CONVERT_TO_HPP

#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"

namespace piranha
{

/// Default functor for the implementation of piranha::convert_to().
template <typename To, typename From, typename = void>
struct convert_to_impl
{
	/// Call operator.
	/**
	 * \note
	 * This call operator is enabled only if \p To is constructible from \p From2.
	 *
	 * @param[in] x conversion argument.
	 *
	 * @return an instance of type \p To constructed from \p x.
	 *
	 * @throws unspecified any exception thrown by the constructor of \p To from \p From2.
	 */
	template <typename From2, typename std::enable_if<std::is_constructible<To,const From2 &>::value,int>::type = 0>
	To operator()(const From2 &x) const
	{
		return To(x);
	}
};

/// Generic conversion function.
/**
 * This function is meant to convert an instance of type \p From to an instance of the decayed
 * type of \p To. It is intended to be a user-extensible replacement for \p static_cast.
 *
 * The actual implementation of this function is in the piranha::convert_to_impl functor's
 * call operator. The decayed type of \p To is passed as first template parameter of
 * piranha::convert_to_impl, whereas \p From is passed as-is.
 *
 * @param[in] x conversion argument.
 *
 * @returns an instance of the decayed type of \p To converted from \p x.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::convert_to_impl functor.
 */
template <typename To, typename From>
inline auto convert_to(const From &x) -> decltype(convert_to_impl<typename std::decay<To>::type,From>()(x))
{
	using return_type = typename std::decay<To>::type;
	static_assert(std::is_same<return_type,decltype(convert_to_impl<return_type,From>()(x))>::value,
		"Invalid return type for convert_to().");
	return convert_to_impl<return_type,From>()(x);
}

/// Type trait to detect piranha::convert_to().
/**
 * The type trait will be \p true if piranha::convert_to() can be called with the decayed types of \p To and \p From
 * as template arguments, \p false otherwise.
 */
template <typename To, typename From>
class has_convert_to: detail::sfinae_types
{
		using Tod = typename std::decay<To>::type;
		using Fromd = typename std::decay<From>::type;
		template <typename To1, typename From1>
		// NOTE: here we use the impl functor instead of the free function in order to work around what looks like
		// an Intel bug -- it seems like decltype() is not coping well with a function with explicit template parameter
		// such as convert_to<To1>(x). This should be equivalent.
		static auto test(const To1 &, const From1 &x) -> decltype(convert_to_impl<To1,From1>()(x),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<Tod>(),std::declval<Fromd>())),yes>::value;
};

// Static init.
template <typename To, typename From>
const bool has_convert_to<To,From>::value;

}

#endif
