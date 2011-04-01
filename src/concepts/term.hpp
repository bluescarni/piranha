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

#ifndef PIRANHA_CONCEPT_TERM_HPP
#define PIRANHA_CONCEPT_TERM_HPP

#include <boost/concept_check.hpp>
#include <type_traits>

#include "../detail/base_term_fwd.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series terms.
/**
 * The requisites for type \p T are the following:
 * 
 * - must derive from piranha::base_term,
 * - must be provided with a two-arguments constructor from piranha::base_term::cf_type and piranha::base_term::key_type.
 */
template <typename T>
struct Term
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(Term)
	{
		static_assert(std::is_base_of<detail::base_term_tag,T>::value,"Term type must derive from base_term.");
		// Test the two-arguments constructor.
		typedef typename T::cf_type cf_type;
		typedef typename T::key_type key_type;
		// NOTE: here cf and key types are required to have default constructors from basE_term.
		cf_type cf;
		key_type key;
		T t(cf,key);
	}
};

}
}

#endif
