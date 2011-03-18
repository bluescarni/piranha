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

#ifndef PIRANHA_CRTP_CONCEPT_HPP
#define PIRANHA_CRTP_CONCEPT_HPP

#include <boost/concept_check.hpp>

#include <type_traits>

namespace piranha
{

/// Concept for classes using the CRTP.
/**
 * \p Derived must either derive from \p Base or be \p void.
 */
template <typename Base, typename Derived>
struct CRTPConcept
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(CRTPConcept)
	{
		static_assert(std::is_same<void,Derived>::value || std::is_base_of<Base,Derived>::value,"CRTP derived class does not derive from base and is not void.");
	}
};

}

#endif
