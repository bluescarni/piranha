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

#ifndef PIRANHA_CONCEPT_POWER_SERIES_TERM_HPP
#define PIRANHA_CONCEPT_POWER_SERIES_TERM_HPP

#include <array>
#include <boost/concept_check.hpp>

#include "../config.hpp"
#include "../symbol_set.hpp"
#include "term.hpp"

namespace piranha
{

namespace detail
{

template <typename Cf>
struct cf_has_degree
{
	template <typename T>
	static auto test(const T *t) -> decltype(t->degree(),char(0)) {}
	static std::array<char,2u> test(...)
	{
		return std::array<char,2u>{};
	}
	static const bool value = (sizeof(test((Cf *)piranha_nullptr)) == 1u);
};

template <typename Key>
struct key_has_degree
{
	template <typename T>
	static auto test(const T *t) -> decltype(t->degree(std::declval<symbol_set>()),char(0)) {}
	static std::array<char,2u> test(...)
	{
		return std::array<char,2u>{};
	}
	static const bool value = (sizeof(test((Key *)piranha_nullptr)) == 1u);
};

}

namespace concept
{

/// Concept for terms suitable for use in piranha::power_series.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::Term,
 * - either the coefficient or the key or both must be provided with a <tt>degree()</tt> method, with the following
 *   signatures:
 *   - for coefficients, a method taking no parameters
 * TODO fix.
 */
template <typename T>
struct PowerSeriesTerm:
	Term<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(PowerSeriesTerm)
	{
		static_assert(detail::cf_has_degree<typename T::cf_type>::value || detail::key_has_degree<typename T::key_type>::value,"Missing degree() function in ");
	}
};

}
}

#endif
