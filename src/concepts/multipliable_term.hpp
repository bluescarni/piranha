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

#ifndef PIRANHA_CONCEPT_MULTIPLIABLE_TERM_HPP
#define PIRANHA_CONCEPT_MULTIPLIABLE_TERM_HPP

#include <boost/concept_check.hpp>
#include <type_traits>
#include <tuple>
#include <utility>

#include "../symbol_set.hpp"
#include "../type_traits.hpp"
#include "term.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series terms multipliable by the default piranha::series_multiplier.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::Term,
 * - must be provided with a typedef \p multiplication_result_type which is either \p T itself or a tuple of types \p T of nonzero size,
 *   used to represent the result of the multiplication with another term,
 * - must be provided with a const member function <tt>multiply()</tt> accepting as first parameter a mutable reference
 *   to an instance of type \p multiplication_result_type, as second parameter a const reference to another term instance and
 *   as third parameter a const reference to a piranha::symbol_set, and returning \p void.
 * 
 * \todo implement the check on the tuple types.
 */
template <typename T>
class MultipliableTerm:
	Term<T>
{
	private:
		PIRANHA_DECLARE_HAS_TYPEDEF(multiplication_result_type);
		static void check_tuple(const std::true_type &)
		{
			static_assert(std::tuple_size<typename T::multiplication_result_type>::value > 0u,
				"Invalid tuple length for term multiplication result type.");
		}
		static void check_tuple(const std::false_type &) {}
	public:
		/// Concept usage pattern.
		BOOST_CONCEPT_USAGE(MultipliableTerm)
		{
			static_assert(has_typedef_multiplication_result_type<T>::value,"Missing multiplication result type typedef.");
			static_assert(std::is_same<typename T::multiplication_result_type,T>::value ||
				is_tuple<typename T::multiplication_result_type>::value,"Invalid multiplication result type typedef.");
			check_tuple(std::integral_constant<bool,is_tuple<typename T::multiplication_result_type>::value>());
			typename T::multiplication_result_type retval;
			typedef decltype(std::declval<const T &>().multiply(retval,std::declval<const T &>(),std::declval<const symbol_set &>())) ret_type;
			static_assert(std::is_same<void,ret_type>::value,"Invalid return value type for multiply()");
		}
};

}
}

#endif
