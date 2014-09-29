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

#ifndef PIRANHA_SAFE_CAST_HPP
#define PIRANHA_SAFE_CAST_HPP

#include <boost/numeric/conversion/cast.hpp>
#include <stdexcept>
#include <type_traits>

#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"

namespace piranha
{

template <typename To, typename From, typename = void>
struct safe_cast_impl
{
	template <typename To2 = To, typename std::enable_if<std::is_same<To2,From>::value &&
		std::is_copy_constructible<From>::value,int>::type = 0>
	To2 operator()(const From &f) const
	{
		return f;
	}
};

template <typename To, typename From>
struct safe_cast_impl<To,From,typename std::enable_if<
	std::is_integral<To>::value && std::is_integral<From>::value
>::type>
{
	To operator()(const From &f) const
	{
		try {
			return boost::numeric_cast<To>(f);
		} catch (...) {
			piranha_throw(std::invalid_argument,"invalid safe cast between integral types");
		}
	}
};

template <typename To, typename From>
inline auto safe_cast(const From &x) -> decltype(safe_cast_impl<typename std::decay<To>::type,From>()(x))
{
	using return_type = typename std::decay<To>::type;
	static_assert(std::is_same<return_type,decltype(safe_cast_impl<return_type,From>()(x))>::value,
		"Invalid return type for safe_cast().");
	return safe_cast_impl<return_type,From>()(x);
}

template <typename To, typename From>
class has_safe_cast: detail::sfinae_types
{
		using Tod = typename std::decay<To>::type;
		using Fromd = typename std::decay<From>::type;
		template <typename To1, typename From1>
		static auto test(const To1 &, const From1 &x) -> decltype(safe_cast_impl<To1,From1>()(x),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<Tod>(),std::declval<Fromd>())),yes>::value;
};

// Static init.
template <typename To, typename From>
const bool has_safe_cast<To,From>::value;

}

#endif
