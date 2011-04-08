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

#ifndef PIRANHA_CONCEPT_SERIES_HPP
#define PIRANHA_CONCEPT_SERIES_HPP

#include <boost/concept_check.hpp>
#include <type_traits>

#include "../detail/base_series_fwd.hpp"
#include "container_element.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::ContainerElement,
 * - must derive from piranha::base_series.
 */
template <typename T>
struct Series:
	ContainerElement<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(Series)
	{
		static_assert(std::is_base_of<detail::base_series_tag,T>::value,"Series type must derive from base_series.");
	}
};

}
}

#endif
