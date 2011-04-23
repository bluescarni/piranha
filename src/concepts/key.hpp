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

#ifndef PIRANHA_CONCEPT_KEY_HPP
#define PIRANHA_CONCEPT_KEY_HPP

#include <boost/concept_check.hpp>
#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "../config.hpp"
#include "../symbol.hpp"
#include "container_element.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series keys.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::ContainerElement,
 * - must not be a pointer,
 * - must be constructible from a \p std::vector of piranha::symbol,
 * - must be directable to output stream,
 * - must be equality-comparable,
 * - must be provided with a \p std::hash specialisation,
 * - must be provided with a const \p is_compatible method accepting a vector of piranha::symbol
 *   as input and returning a boolean value,
 * - must be provided with a const \p is_ignorable method accepting a vector of piranha::symbol
 *   as input and returning a boolean value,
 * - must be provided with a const \p merge_args method accepting two \p std::vector of piranha::symbol
 *   as input and returning an instance of \p T,
 * - must be provided with a const \p is_unitary method accepting a \p std::vector of piranha::symbol
 *   as input and returning bool.
 * 
 * \todo assert that key's hasher satisfy the Hashable requirements.
 */
template <typename T>
struct Key:
	ContainerElement<T>,
	boost::EqualityComparable<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(Key)
	{
		static_assert(!std::is_pointer<T>::value,"Key type cannot be a pointer.");
		T tmp = T(std::vector<symbol>());
		std::cout << *(static_cast<T *>(piranha_nullptr));
		const T inst = T();
		auto tmp1 = inst.is_compatible(std::vector<symbol>{});
		static_assert(std::is_same<decltype(tmp1),bool>::value,"Invalid is_compatible() method signature for key type.");
		auto tmp2 = inst.is_ignorable(std::vector<symbol>{});
		static_assert(std::is_same<decltype(tmp2),bool>::value,"Invalid is_ignorable() method signature for key type.");
		auto merge_out = inst.merge_args(std::vector<symbol>{},std::vector<symbol>{});
		static_assert(std::is_same<decltype(merge_out),T>::value,"Invalid merge_args() method signature for key type.");
		auto tmp3 = inst.is_unitary(std::vector<piranha::symbol>{});
		static_assert(std::is_same<decltype(tmp3),bool>::value,"Invalid is_unitary() method signature for key type.");
		// TODO: assert here that hasher satisfy the Hashable requirements.
		std::hash<T> hasher;
		(void)hasher;
	}
};

}
}

#endif
