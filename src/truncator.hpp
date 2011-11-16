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

#ifndef PIRANHA_TRUNCATOR_HPP
#define PIRANHA_TRUNCATOR_HPP

#include <boost/concept/assert.hpp>
#include <iostream>

#include "concepts/series.hpp"
#include "config.hpp"
#include "detail/sfinae_types.hpp"

namespace piranha
{

/// Default truncator class.
/**
 * A truncator is an object that is intended to establish an order over the terms of a series and truncate the result of series multiplication.
 * The minimal interface that must be provided by a truncator class is the following:
 * 
 * - a constructor from a \p Series object (the series on which the truncator will operate),
 * - an is_active() method, signalling whether the truncator is active
 *   or not (this method is provided for optimisation purposes: by knowing if the truncator is active or not
 *   it is possible to avoid querying the truncator repeatedly during certain time-critical operations).
 * 
 * Additionally, the following optional methods can be implemented by truncator classes:
 * 
 * - a <tt>bool compare_terms(const term_type &t1, const term_type &t2) const</tt> method, which must be a strict
 *   weak ordering comparison function on the term type of \p Series
 *   that returns \p true if \p t1 comes before \p t2, \p false otherwise. This method
 *   is used to rank the terms of the series used for construction. A truncator implementing
 *   this method is a <em>sorting</em> truncator.
 * 
 * The presence of the optional methods can be queried at compile time using the piranha::truncator_traits class. The default
 * implementation of piranha::truncator does not implement any of the optional methods, and its is_active() method will always
 * return \p false.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Series must be a model of the piranha::concept::Series concept.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * This class is stateless and hence provides trivial move semantics.
 * 
 * \todo truncator concept?
 * \todo require copy-constructability?
 * \todo mention this must be specialised using enable_if, and do the same in series multiplier.
 */
template <typename Series, typename Enable = void>
class truncator
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series>));
	public:
		/// Constructor from series.
		explicit truncator(const Series &) {}
		/// Query the status of the truncator.
		/**
		 * @return \p false.
		 */
		bool is_active() const
		{
			return false;
		}
		/// Stream operator overload for the default implementatin of piranha::truncator.
		/**
		 * Will send to stream \p os a human-readable representation of the truncator.
		 * 
		 * @param[in] os target stream.
		 * @param[in] t piranha::truncator argument.
		 * 
		 * @return reference to \p os.
		 */
		friend std::ostream &operator<<(std::ostream &os, const truncator &t)
		{
			os <<	"Null truncator\n";
			return os;
		}
};

/// Truncator traits.
/**
 * This traits class is used to query which optional methods are implemented in piranha::truncator of \p Series.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Series must be a model of the piranha::concept::Series concept.
 * 
 * @see piranha::truncator for the description of the optional interface.
 */
template <typename Series>
class truncator_traits: detail::sfinae_types
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series>));
		typedef truncator<Series> truncator_type;
		typedef typename Series::term_type term_type;
		template <typename T>
		static auto test_sorting(const T *t) -> decltype(t->compare_terms(std::declval<term_type>(),std::declval<term_type>()),yes());
		static no test_sorting(...);
	public:
		/// Sorting flag.
		/**
		 * Will be \p true if the truncator type of \p Series is a sorting truncator, \p false otherwise.
		 */
		static const bool is_sorting = (sizeof(test_sorting((const truncator_type *)piranha_nullptr)) == sizeof(yes));
};

template <typename Series>
const bool truncator_traits<Series>::is_sorting;

}

#endif
