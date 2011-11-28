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

#ifndef PIRANHA_CONCEPT_TRUNCATOR_HPP
#define PIRANHA_CONCEPT_TRUNCATOR_HPP

#include <boost/concept_check.hpp>
#include <iostream>
#include <type_traits>

#include "../detail/truncator_fwd.hpp"
#include "series.hpp"

namespace piranha
{

namespace concept
{

/// Truncator concept.
/**
 * Will check that piranha::truncator of <tt>Series...</tt> satisfies the truncator requirements.
 */
template <typename... Series>
class Truncator
{
		static_assert(sizeof...(Series) == 1u || sizeof...(Series) == 2u,"Invalid number of template parameters.");
		template <typename T>
		static truncator<T> check_ctor()
		{
			BOOST_CONCEPT_ASSERT((concept::Series<T>));
			const T s;
			const truncator<T> retval(s);
			return retval;
		}
		template <typename T, typename U>
		static truncator<T,U> check_ctor()
		{
			BOOST_CONCEPT_ASSERT((concept::Series<T>));
			BOOST_CONCEPT_ASSERT((concept::Series<U>));
			const T s1;
			const U s2;
			const truncator<T,U> retval(s1,s2);
			return retval;
		}
	public:
		/// Concept usage pattern.
		BOOST_CONCEPT_USAGE(Truncator)
		{
			typedef truncator<Series...> truncator_type;
			const truncator_type t = check_ctor<Series...>();
			auto t2(t);
			std::cout << t;
			t.is_active();
			static_assert(std::is_same<decltype(t.is_active()),bool>::value,"Invalid return type for is_active() method.");
			// This should check the consistency of the traits.
			truncator_traits<Series...> tt;
			(void)tt;
		}
};

}
}

#endif
