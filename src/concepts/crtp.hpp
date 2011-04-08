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

#ifndef PIRANHA_CONCEPT_CRTP_HPP
#define PIRANHA_CONCEPT_CRTP_HPP

#include <boost/concept_check.hpp>

#include <type_traits>

namespace piranha
{
namespace concept
{

/// Concept for classes using the curiously recurring template pattern.
/**
 * \p Derived must derive from \p Base (as determined by \p std::is_base_of).
 * 
 * @see http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 */
template <typename Base, typename Derived>
struct CRTP
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(CRTP)
	{
		static_assert(std::is_base_of<Base,Derived>::value,"CRTP derived class does not derive from base.");
	}
};

}
}

#endif
