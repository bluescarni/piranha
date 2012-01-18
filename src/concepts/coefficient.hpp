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

#ifndef PIRANHA_CONCEPT_COEFFICIENT_HPP
#define PIRANHA_CONCEPT_COEFFICIENT_HPP

#include <boost/concept_check.hpp>
#include <iostream>
#include <type_traits>

#include "../config.hpp"
#include "../math.hpp"
#include "../print_coefficient.hpp"
#include "container_element.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series coefficients.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::ContainerElement,
 * - must not be a pointer,
 * - must be suitable as argument type of piranha::print_coefficient(),
 * - must be suitable as argument type of piranha::math::is_zero() and piranha::math::negate(),
 * - must be equality comparable,
 * - must be addable and subtractable.
 */
template <typename T>
struct Coefficient:
	ContainerElement<T>,
	boost::EqualityComparable<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(Coefficient)
	{
		static_assert(!std::is_pointer<T>::value,"Coefficient type cannot be a pointer.");
		const T inst = T();
		print_coefficient(std::cout,inst);
		math::is_zero(inst);
		T inst_mut = T();
		math::negate(inst_mut);
		(void)(inst_mut + inst_mut);
		inst_mut += inst_mut;
		(void)(inst_mut - inst_mut);
		inst_mut -= inst_mut;
	}
};

}
}

#endif
