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

#ifndef PIRANHA_ECHELON_POSITION_HPP
#define PIRANHA_ECHELON_POSITION_HPP

#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <type_traits>

#include "concepts/term.hpp"
#include "detail/base_series_fwd.hpp"

namespace piranha
{

namespace detail
{

template <typename Term1, typename Term2, std::size_t CurLevel = 0, typename Enable = void>
struct echelon_position_impl
{
	static_assert(std::is_base_of<base_series_tag,typename Term1::cf_type>::value,"Term type does not appear in echelon hierarchy.");
	static_assert(echelon_position_impl<typename Term1::cf_type::term_type,Term2>::value < boost::integer_traits<std::size_t>::const_max,"Overflow error.");
	static const std::size_t value = echelon_position_impl<typename Term1::cf_type::term_type,Term2>::value + static_cast<std::size_t>(1);
};

template <typename Term1, typename Term2, std::size_t CurLevel>
struct echelon_position_impl<Term1,Term2,CurLevel,typename std::enable_if<std::is_same<Term1,Term2>::value>::type>
{
	static const std::size_t value = CurLevel;
};

}

/// Echelon position type-trait.
/**
 * Echelon position of \p Term with respect to \p TopLevelTerm.
 * The echelon position is an index, starting from zero, corresponding to the level in the echelon hierarchy of \p TopLevelTerm
 * in which \p Term appears.
 * 
 * For instance, if \p TopLevelTerm and \p Term are the same type, then the echelon position of \p Term is 0, because \p Term is the
 * first type encountered in the echelon hierarchy of \p TopLevelTerm. If \p TopLevelTerm is a Poisson series term, then the echelon position
 * of the polynomial term type defined by the coefficient of \p TopLevelTerm is 1: the polynomial term is the term type of the coefficient
 * series of the term type at echelon position 0.
 * 
 * If \p Term does not appear in the echelon hierarchy of \p TopLevelTerm, a compile-time error will be produced.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Term and \p TopLevelTerm must be models of piranha::concept::Term.
 */
template <typename TopLevelTerm, typename Term>
class echelon_position
{
	private:
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		BOOST_CONCEPT_ASSERT((concept::Term<TopLevelTerm>));
		static_assert(std::is_same<Term,TopLevelTerm>::value || detail::echelon_position_impl<TopLevelTerm,Term>::value,
			"Assertion error in the calculation of echelon position.");
	public:
		/// Value of echelon position.
		static const std::size_t value = detail::echelon_position_impl<TopLevelTerm,Term>::value;
};

// Instantiation of type-trait value.
template <typename TopLevelTerm, typename Term>
const std::size_t echelon_position<TopLevelTerm,Term>::value;

}

#endif
