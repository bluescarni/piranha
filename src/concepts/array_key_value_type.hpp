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

#ifndef PIRANHA_CONCEPT_ARRAY_KEY_VALUE_TYPE_HPP
#define PIRANHA_CONCEPT_ARRAY_KEY_VALUE_TYPE_HPP

#include <boost/concept_check.hpp>
#include <iostream>

#include "../math.hpp"
#include "container_element.hpp"

namespace piranha
{

namespace concept
{

/// Concept for types suitable for use as value types in piranha::array_key.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::ContainerElement,
 * - must be constructible from the integral constant 0,
 * - must be assignable,
 * - must be equality-comparable,
 * - must be addable (both in binary and unary form),
 * - must be streamable,
 * - must be a valid argument type for piranha::math::is_zero().
 * 
 * \todo add std::is_assignable check when implemented in GCC.
 */
template <typename T>
struct ArrayKeyValueType:
	ContainerElement<T>,
	boost::EqualityComparable<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(ArrayKeyValueType)
	{
		T inst = T(0);
		// Shut off compiler warning.
		(void)&inst;
		T inst2(inst);
		inst += inst2;
		inst = inst + inst2;
		std::cout << inst2;
		piranha::math::is_zero(inst);
	}
};

}
}

#endif
