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

#ifndef PIRANHA_POLYNOMIAL_TERM_HPP
#define PIRANHA_POLYNOMIAL_TERM_HPP

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include "base_term.hpp"
#include "echelon_descriptor.hpp"
#include "config.hpp"
#include "monomial.hpp"
#include "symbol.hpp"
#include "term_series_converter.hpp"

namespace piranha
{

/// Polynomial term.
/**
 * This class extends piranha::base_term for use in polynomials. The coefficient type \p Cf is generic,
 * the key is piranha::monomial of \p ExpoType.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Cf must be a model of piranha::concept::Coefficient.
 * - \p ExpoType must be suitable for use in piranha::monomial.
 * 
 * \section exception_safety Exception safety guarantees
 * 
 * This class provides the same guarantees as piranha::base_term.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to that of piranha::base_term.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Cf, typename ExpoType>
class polynomial_term: public base_term<Cf,monomial<ExpoType>,polynomial_term<Cf,ExpoType>>
{
		typedef base_term<Cf,monomial<ExpoType>,polynomial_term<Cf,ExpoType>> base;
	public:
		/// Defaulted default constructor.
		polynomial_term() = default;
		/// Defaulted copy constructor.
		polynomial_term(const polynomial_term &) = default;
		/// Defaulted move constructor.
		polynomial_term(polynomial_term &&) = default;
		/// Defaulted destructor.
		~polynomial_term() = default;
		/// Defaulted copy assignment operator.
		polynomial_term(const polynomial_term &) = default;
		/// Defaulted move assignment operator.
		polynomial_term(polynomial_term &&) = default;
		/// Compatibility test.
		/**
		 * @param[in] ed reference piranha::echelon_descriptor.
		 * 
		 * @return \p true if both these conditions hold:
		 * 
		 * - the number of arguments in \p ed at the echelon position corresponding to this term type is the same as the
		 *   key's size,
		 * - the coefficient's <tt>is_compatible(ed)</tt> method returns true,
		 * 
		 * \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the coefficient's <tt>is_compatible()</tt> method.
		 */
		template <typename Term>
		bool is_compatible(const echelon_descriptor<Term> &ed) const
		{
			return (this->m_cf.is_compatible(ed) && this->m_key.size() == ed.template get_args<polynomial_term<Cf,ExpoType>>().size());
		}
		/// Ignorability test.
		/**
		 * A polynomial term is ignorable iff its coefficient is zero.
		 * 
		 * @param[in] ed reference piranha::echelon_descriptor.
		 * 
		 * @return return value of the coefficient's <tt>is_zero(ed)</tt> method.
		 * 
		 * @throws unspecified any exception thrown by the coefficient's <tt>is_zero()</tt> method.
		 */
		template <typename Term>
		bool is_ignorable(const echelon_descriptor<Term> &ed) const
		{
			return this->m_cf.is_zero(ed);
		}
};

}

#endif
