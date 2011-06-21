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

#ifndef PIRANHA_CONCEPT_MULTIPLIABLE_COEFFICIENT_HPP
#define PIRANHA_CONCEPT_MULTIPLIABLE_COEFFICIENT_HPP

#include <boost/concept_check.hpp>
#include <type_traits>

#include "../base_term.hpp"
#include "../echelon_descriptor.hpp"
#include "coefficient.hpp"

namespace piranha
{

namespace concept
{

/// Concept for multipliable coefficients.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::Coefficient,
 * - must be provided with a const \p multiply method accepting an instance of \p T and a generic piranha::echelon_descriptor
 *   as input and returning an instance of \p T.
 */
template <typename T>
struct MultipliableCoefficient:
	Coefficient<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(MultipliableCoefficient)
	{
		typedef echelon_descriptor<base_term<int,int,void>> ed_type;
		static_assert(std::is_same<decltype(std::declval<T>().multiply(std::declval<T>(),std::declval<ed_type>())),T>::value,"Invalid multiply() method signature for coefficient type.");
	}
};

}

}

#endif
