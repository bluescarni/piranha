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

#ifndef PIRANHA_KEY_IS_CONVERTIBLE_HPP
#define PIRANHA_KEY_IS_CONVERTIBLE_HPP

#include <type_traits>

#include "is_key.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Detect if a key type is convertible to another key type.
/**
 * This type trait will be \p true if the key type \p To can be constructed
 * from a const reference to the key type \p From and a const reference to
 * piranha::symbol_set, \p false otherwise.
 *
 * The decay types of \p To and \p From are considered by this type trait. \p To and \p From must satisfy
 * piranha::is_key, otherwise a compile-time error will be generated.
 */
template <typename To, typename From>
class key_is_convertible
{
		using Tod = typename std::decay<To>::type;
		using Fromd = typename std::decay<From>::type;
		PIRANHA_TT_CHECK(is_key,Tod);
		PIRANHA_TT_CHECK(is_key,Fromd);
	public:
		/// Value of the type trait.
		static const bool value = std::is_constructible<Tod,const Fromd &, const symbol_set &>::value;
};

template <typename To, typename From>
const bool key_is_convertible<To,From>::value;

}

#endif
