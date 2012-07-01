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

#ifndef PIRANHA_CONCEPT_DIFFERENTIABLE_TERM_HPP
#define PIRANHA_CONCEPT_DIFFERENTIABLE_TERM_HPP

#include <boost/concept_check.hpp>
#include <type_traits>
#include <vector>

#include "../symbol.hpp"
#include "../symbol_set.hpp"
#include "term.hpp"

namespace piranha
{

namespace concept
{

/// Concept for differentiable series terms.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::Term,
 * - must be provided with a const <tt>partial()</tt> method taking as parameters an instance of piranha::symbol and an instance
 *   of piranha::symbol_set, and returning a vector of terms.
 */
template <typename T>
class DifferentiableTerm:
	Term<T>
{
	public:
		/// Concept usage pattern.
		BOOST_CONCEPT_USAGE(DifferentiableTerm)
		{
			const T inst = T();
			const symbol s("");
			const symbol_set ss;
			auto ret = inst.partial(s,ss);
			static_assert(std::is_same<decltype(ret),std::vector<T>>::value,"Invalid return type for partial() method.");
		}
};

}
}

#endif
