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

#ifndef PIRANHA_MULTIPLIABLE_TERM_HPP
#define PIRANHA_MULTIPLIABLE_TERM_HPP

#include <boost/concept_check.hpp>
#include <type_traits>

#include "../echelon_descriptor.hpp"
#include "../type_traits.hpp"
#include "term.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series terms multipliable by the non-specialised piranha::series_multiplier.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::Term,
 * - must be provided with a const member function <tt>multiply()</tt> accepting as const parameters
 *   another term instance and a generic piranha::echelon_descriptor, and returning either a single term of type \p T or an \p std::tuple
 *   of terms of type \p T (of nonzero size) as the result of the multiplication between \p this and the first argument of the method.
 * 
 * \todo implement the check on the tuple types.
 */
template <typename T>
struct MultipliableTerm:
	Term<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(MultipliableTerm)
	{
		typedef decltype(std::declval<T>().multiply(std::declval<T>(),std::declval<echelon_descriptor<T>>())) ret_type;
		static_assert(is_tuple<ret_type>::value || std::is_same<T,ret_type>::value,"Invalid return value type.");
	}
};

}
}

#endif
