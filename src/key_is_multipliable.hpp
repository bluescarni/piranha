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

#ifndef PIRANHA_KEY_IS_MULTIPLIABLE_HPP
#define PIRANHA_KEY_IS_MULTIPLIABLE_HPP

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"
#include "is_cf.hpp"
#include "is_key.hpp"
#include "symbol_set.hpp"
#include "term.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

template <typename Cf, typename Key, typename = void>
struct key_is_multipliable_impl
{
	static const bool value = false;
};

template <typename Cf, typename Key>
struct key_is_multipliable_impl<Cf,Key,typename std::enable_if<std::is_same<const std::size_t,decltype(Key::multiply_arity)>::value &&
	(Key::multiply_arity > 0u)>::type>: detail::sfinae_types
{
	template <typename Cf1, typename Key1>
	static auto test(const Cf1 &, const Key1 &) -> decltype(Key1::multiply(std::declval<std::array<term<Cf1,Key1>,Key1::multiply_arity> &>(),
		std::declval<const term<Cf1,Key1> &>(),std::declval<const term<Cf1,Key1> &>(),std::declval<const symbol_set &>()),void(),yes());
	static no test(...);
	static const bool value = std::is_same<yes,decltype(test(std::declval<Cf>(),std::declval<Key>()))>::value;
};

}

/// Type trait for multipliable key.
/**
 * A multipliable key satisfies the following requirements:
 * - it has a static member of type \p std::size_t called \p multiply_arity with a value greater than zero,
 * - it has a static function called \p multiply() accepting the following arguments:
 *   - a reference to an \p std::array of piranha::term of \p Cf and \p Key of size \p multiply_arity,
 *   - two const references to piranha::terms of \p Cf and \p Key,
 *   - a const reference to piranha::symbol_set.
 *
 * The decay types of \p Cf and \p Key are considered by this type trait. \p Cf and \p Key must satisfy
 * piranha::is_cf and piranha::is_key, otherwise a compile-time error will be generated.
 */
template <typename Cf, typename Key>
class key_is_multipliable
{
		using Cfd = typename std::decay<Cf>::type;
		using Keyd = typename std::decay<Key>::type;
		PIRANHA_TT_CHECK(is_cf,Cfd);
		PIRANHA_TT_CHECK(is_key,Keyd);
	public:
		/// Value of the type trait.
		static const bool value = detail::key_is_multipliable_impl<Cfd,Keyd>::value;
};

template <typename Cf, typename Key>
const bool key_is_multipliable<Cf,Key>::value;

}

#endif
