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
	 * This call operator is enabled only if <tt>static_cast<To>(x)</tt> is a well-formed
	 * expression.
	 *
	 * @param[in] x conversion argument.
	 *
	 * @return an instance of type \p To cast from \p x.
	 *
	 * @throws unspecified any exception thrown by casting \p x to \p To.
	 */
	template <typename From2>
	auto operator()(const From2 &x) const -> decltype(static_cast<To>(x))
	{
		return static_cast<To>(x);
	}
};

namespace detail
{

// Enabler for the generic conversion function. We need to check that the impl functor is callable
// and that it returns the proper type.
template <typename To, typename From>
using convert_enabler = typename std::enable_if<
	std::is_same<decltype(convert_to_impl<typename std::decay<To>::type,From>()(std::declval<const From &>())),typename std::decay<To>::type>::value,
	int>::type;

}

/// Generic conversion function.
/**
 * This function is meant to convert an instance of type \p From to an instance of the decayed
 * type of \p To. It is intended to be a user-extensible replacement for \p static_cast.
 *
 * The actual implementation of this function is in the piranha::convert_to_impl functor's
 * call operator. The decayed type of \p To is passed as first template parameter of
 * piranha::convert_to_impl, whereas \p From is passed as-is.
 *
 * Any specialisation of piranha::convert_to_impl must have a call operator returning
 * an instance of the decayed type of \p To, otherwise this function will be disabled.
 *
 * @param[in] x conversion argument.
 *
 * @returns an instance of the decayed type of \p To converted from \p x.
 *
 * @throws unspecified any exception thrown by the call operator of the piranha::convert_to_impl functor.
 */
template <typename To, typename From, detail::convert_enabler<To,From> = 0>
inline To convert_to(const From &x)
{
	return convert_to_impl<typename std::decay<To>::type,From>()(x);
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
		// NOTE: here we can just reuse the logic from the detail enabler. Note that using
		// directly the convert_to() function here, as we do elsewhere, will not work on Intel,
		// due to what looks like a bug when using decltype with an explicit template parameter
		// in the function.
		template <typename To1, typename From1, detail::convert_enabler<To1,From1> = 0>
		static yes test(const To1 &, const From1 &);
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
