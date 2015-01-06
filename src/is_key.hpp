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

#ifndef PIRANHA_IS_KEY_HPP
#define PIRANHA_IS_KEY_HPP

#include <iostream>
#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Test the presence of requested key methods.
template <typename T>
struct is_key_impl: sfinae_types
{
	template <typename U>
	static auto test0(const U &u) -> decltype(u.is_compatible(std::declval<symbol_set const &>()));
	static no test0(...);
	template <typename U>
	static auto test1(const U &u) -> decltype(u.is_ignorable(std::declval<symbol_set const &>()));
	static no test1(...);
	template <typename U>
	static auto test2(const U &u) -> decltype(u.merge_args(std::declval<symbol_set const &>(),std::declval<symbol_set const &>()));
	static no test2(...);
	template <typename U>
	static auto test3(const U &u) -> decltype(u.is_unitary(std::declval<symbol_set const &>()));
	static no test3(...);
	template <typename U>
	static auto test4(const U &u) -> decltype(u.print(std::declval<std::ostream &>(),std::declval<symbol_set const &>()),void(),yes());
	static no test4(...);
	template <typename U>
	static auto test5(const U &u) -> decltype(u.print_tex(std::declval<std::ostream &>(),std::declval<symbol_set const &>()),void(),yes());
	static no test5(...);
	static const bool value = std::is_same<bool,decltype(test0(std::declval<T>()))>::value &&
				  std::is_same<bool,decltype(test1(std::declval<T>()))>::value &&
				  std::is_same<T,decltype(test2(std::declval<T>()))>::value &&
				  std::is_same<bool,decltype(test3(std::declval<T>()))>::value &&
				  std::is_same<yes,decltype(test4(std::declval<T>()))>::value &&
				  std::is_same<yes,decltype(test5(std::declval<T>()))>::value;
};

}

/// Type trait to detect key types.
/**
 * This type trait will be \p true if \p T can be used as a key type, \p false otherwise.
 * The requisites for \p T are the following:
 * 
 * - it must satisfy piranha::is_container_element,
 * - it must be constructible from a const piranha::symbol_set reference,
 * - it must satisfy piranha::is_equality_comparable,
 * - it must satisfy piranha::is_hashable,
 * - it must be provided with a const non-throwing \p is_compatible method accepting a const piranha::symbol_set
 *   reference as input and returning a \p bool,
 * - it must be provided with a const non-throwing \p is_ignorable method accepting a const piranha::symbol_set
 *   reference as input and returning \p bool,
 * - it must be provided with a const \p merge_args method accepting two const piranha::symbol_set
 *   references as inputs and returning \p T,
 * - it must be provided with a const \p is_unitary method accepting a const piranha::symbol_set
 *   reference as input and returning \p bool,
 * - it must be provided with const \p print and \p print_tex methods accepting an \p std::ostream reference as first argument
 *   and a const piranha::symbol_set reference as second argument.
 */
/*
 * \todo requirements on vector-of-symbols-constructed key: must it be unitary? (seems like it, look at
 * polynomial ctors from symbol) -> note that both these two checks have to go in the runtime requirements of key
 * when they get documented.
 */
template <typename T, typename = void>
class is_key
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T>
class is_key<T,typename std::enable_if<detail::is_key_impl<T>::value>::type>
{
	public:
		static const bool value = is_container_element<T>::value &&
					  std::is_constructible<T,const symbol_set &>::value &&
					  is_equality_comparable<T>::value &&
					  is_hashable<T>::value &&
					  noexcept(std::declval<T const &>().is_compatible(std::declval<symbol_set const &>())) &&
					  noexcept(std::declval<T const &>().is_ignorable(std::declval<symbol_set const &>()));
};

template <typename T, typename Enable>
const bool is_key<T,Enable>::value;

template <typename T>
const bool is_key<T,typename std::enable_if<detail::is_key_impl<T>::value>::type>::value;

}

#endif
