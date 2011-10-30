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

#ifndef PIRANHA_CONCEPT_DEGREE_KEY_HPP
#define PIRANHA_CONCEPT_DEGREE_KEY_HPP

#include <boost/concept_check.hpp>

#include "../config.hpp"
#include "../detail/sfinae_types.hpp"
#include "../symbol_set.hpp"
#include "key.hpp"

namespace piranha
{

namespace detail
{

template <typename Key>
struct key_has_degree: sfinae_types
{
	template <typename T>
	static auto test1(const T *t) -> decltype(t->degree(std::declval<symbol_set>()),yes());
	static no test1(...);
	template <typename T>
	static auto test2(const T *t) -> decltype(t->degree(std::declval<symbol_set>(),std::declval<symbol_set>()),yes());
	static no test2(...);
	template <typename T>
	static auto test3(const T *t) -> decltype(t->ldegree(std::declval<symbol_set>()),yes());
	static no test3(...);
	template <typename T>
	static auto test4(const T *t) -> decltype(t->ldegree(std::declval<symbol_set>(),std::declval<symbol_set>()),yes());
	static no test4(...);
	static const bool value = (sizeof(test1((Key *)piranha_nullptr)) == sizeof(yes)) &&
		(sizeof(test2((Key *)piranha_nullptr)) == sizeof(yes)) && (sizeof(test3((Key *)piranha_nullptr)) == sizeof(yes)) &&
		(sizeof(test4((Key *)piranha_nullptr)) == sizeof(yes));
};

template <typename Key>
const bool key_has_degree<Key>::value;

}

namespace concept
{

/// Concept for key with degree.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::Key,
 * - must be provided with the following methods to query the degree of the key:
 *   - <tt>degree(const piranha::symbol_set &args) const</tt>, to query the total degree,
 *   - <tt>degree(const piranha::symbol_set &active_args, const piranha::symbol_set &args) const</tt>, to query the partial degree,
 *   - <tt>ldegree(const piranha::symbol_set &args) const</tt>, to query the total low degree,
 *   - <tt>ldegree(const piranha::symbol_set &active_args, const piranha::symbol_set &args) const</tt>, to query the partial low degree.
 */
template <typename T>
struct DegreeKey:
	Key<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(DegreeKey)
	{
		static_assert(detail::key_has_degree<T>::value,"The key does not implement all the required methods to query the degree.");
	}
};

}
}

#endif
