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

#ifndef PIRANHA_CONCEPT_CONTAINER_ELEMENT_HPP
#define PIRANHA_CONCEPT_CONTAINER_ELEMENT_HPP

#include <boost/concept_check.hpp>

#include "../type_traits.hpp"

namespace piranha
{

namespace concept
{

/// Concept for well-behaved element types for containers.
/**
 * The requisites for type \p T are the following:
 * 
 * - must not be a reference type or cv-qualified,
 * - must be default-constructible,
 * - must be copy-constructible,
 * - must have nothrow move semantics,
 * - must be nothrow-destructible.
 */
template <typename T>
struct ContainerElement:
	boost::DefaultConstructible<T>,
	boost::CopyConstructible<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(ContainerElement)
	{
		static_assert(!is_cv_or_ref<T>::value,"T must not be a reference type or cv-qualified.");
		static_assert(is_nothrow_destructible<T>::value,"T must be nothrow-destructible.");
		static_assert(is_nothrow_move_constructible<T>::value && is_nothrow_move_assignable<T>::value,"T must have nothrow move semantics.");
	}
};

}
}

#endif
