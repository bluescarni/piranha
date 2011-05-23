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

#include "base_term.hpp"
#include "config.hpp"
#include "echelon_descriptor.hpp"
#include "monomial.hpp"

namespace piranha
{

/// Polynomial term.
/**
 * This class extends piranha::base_term for use in polynomials. The coefficient type \p Cf is generic,
 * the key is piranha::monomial of \p ExpoType.
 * This class is a model of the piranha::concept::MultipliableTerm concept.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Cf must be a model of piranha::concept::Coefficient.
 * - \p ExpoType must be suitable for use in piranha::monomial.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as piranha::base_term.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::base_term's move semantics.
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
		/// Generic constructor.
		/**
		 * Will perfectly forward all arguments to a matching constructor in piranha::base_term.
		 * 
		 * @param[in] params parameters for construction.
		 * 
		 * @throws unspecified any exception thrown by the invoked constructor in piranha::base_term.
		 */
		template <typename... Args>
		explicit polynomial_term(Args && ... params):base(std::forward<Args>(params)...) {}
		/// Defaulted destructor.
		~polynomial_term() = default;
		/// Defaulted copy assignment operator.
		polynomial_term &operator=(const polynomial_term &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		polynomial_term &operator=(polynomial_term &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		/// Term multiplication.
		/**
		 * Multiplication of \p this by \p other will produce a single term whose coefficient is the
		 * result of the multiplication of the current coefficient by the coefficient of \p other (via the coefficient's <tt>multiply()</tt> method)
		 * and whose key is the element-by-element sum of the vectors of the exponents of the two terms.
		 * 
		 * @param[in] other argument of the multiplication.
		 * @param[in] ed reference echelon descriptor.
		 * 
		 * @return the result of the multiplication of \p this by \p other.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the <tt>multiply()</tt> method of \p Cf,
		 * - the constructor of piranha::polynomial_term from coefficient and key,
		 * - the in-place addition operator of \p ExpoType.
		 */
		template <typename Cf2, typename ExpoType2, typename Term>
		polynomial_term multiply(const polynomial_term<Cf2,ExpoType2> &other, const echelon_descriptor<Term> &ed) const
		{
			piranha_assert(other.m_key.size() == this->m_key.size());
			piranha_assert(other.m_key.size() == ed.template get_args<polynomial_term>().size());
			polynomial_term pt(this->m_cf.multiply(other.m_cf,ed),this->m_key);
			// Key part.
			const auto size = this->m_key.size();
			for (decltype(this->m_key.size()) i = 0; i < size; ++i) {
				pt.m_key[i] += other.m_key[i];
			}
			return pt;
		}
};

}

#endif
