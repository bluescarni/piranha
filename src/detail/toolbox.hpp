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

#ifndef PIRANHA_TOOLBOX_HPP
#define PIRANHA_TOOLBOX_HPP

#include <boost/concept/assert.hpp>
#include <type_traits>

#include "../concepts/series.hpp"
#include "../detail/series_fwd.hpp"

namespace piranha
{

namespace detail
{

// Base toolbox class.
/*
 * This class implements convenient static checks for series toolbox classes. Specifically, it will check
 * that \p Base is an instance of piranha::series and \p Derived satisfies the piranha::concept::Series
 * concept. If any of these conditions is not met, a compile-time error will be produced.
 */
template <typename Base, typename Derived>
class toolbox
{
		static_assert(std::is_base_of<series_tag,Base>::value,"Toolbox base class must be an instance of piranha::series.");
	public:
		// Trivial destructor.
		/*
		 * Implements part of the static checking logic.
		 */
		~toolbox()
		{
			BOOST_CONCEPT_ASSERT((concept::Series<Derived>));
		}
};

}

}

#endif
