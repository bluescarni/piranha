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

#include <type_traits>

#include "base_term.hpp"
#include "detail/series_fwd.hpp"
#include "forwarding.hpp"
#include "kronecker_monomial.hpp"
#include "math.hpp"
#include "monomial.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

template <typename T, typename S>
struct polynomial_term_key
{
	typedef monomial<T,S> type;
};

template <typename T, typename S>
struct polynomial_term_key<kronecker_monomial<T>,S>
{
	typedef kronecker_monomial<T> type;
};

}

/// Polynomial term.
/**
 * This class extends piranha::base_term for use in polynomials. The coefficient type \p Cf is generic,
 * the key type is determined as follows:
 * 
 * - if \p Expo is piranha::kronecker_monomial of \p T, the key will also be piranha::kronecker_monomial of \p T,
 * - otherwise, the key will be piranha::monomial of \p Expo and \p S.
 * 
 * Examples:
 @code
 polynomial_term<double,int>
 @endcode
 * is a multivariate polynomial term with double-precision coefficient and \p int exponents.
 @code
 polynomial_term<double,short,std::integral_constant<std::size_t,5>>
 @endcode
 * is a multivariate polynomial term with double-precision coefficient and \p short exponents, up to 5 of which
 * will be stored in static storage.
 @code
 polynomial_term<double,kronecker_monomial<>>
 @endcode
 * is a multivariate polynomial term with double-precision coefficient and integral exponents packed into a piranha::kronecker_monomial.
 * 
 * ## Type requirements ##
 * 
 * - \p Cf must be suitable for use in piranha::base_term;
 * - \p Cf must satisfy the following type traits:
 *   - piranha::is_multipliable and piranha::is_multipliable_in_place,
 *   - piranha::has_multiply_accumulate.
 * - \p Expo and \p S must be suitable for use in piranha::monomial, or \p Expo must be an instance of
 *   piranha::kronecker_monomial.
 * 
 * ## Exception safety guarantee ##
 * 
 * This class provides the same guarantee as piranha::base_term.
 * 
 * ## Move semantics ##
 * 
 * Move semantics is equivalent to piranha::base_term's move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 *
 * \todo sfinaeing or something analogous for the multiplication method, once we sort out series multiplication.
 */
template <typename Cf, typename Expo, typename S = std::integral_constant<std::size_t,0u>>
class polynomial_term: public base_term<Cf,typename detail::polynomial_term_key<Expo,S>::type,polynomial_term<Cf,Expo,S>>
{
		PIRANHA_TT_CHECK(is_multipliable,Cf);
		PIRANHA_TT_CHECK(is_multipliable_in_place,Cf);
		PIRANHA_TT_CHECK(has_multiply_accumulate,Cf);
		using base = base_term<Cf,typename detail::polynomial_term_key<Expo,S>::type,polynomial_term<Cf,Expo,S>>;
	public:
		/// Result type for the multiplication by another term.
		typedef polynomial_term multiplication_result_type;
		/// Defaulted default constructor.
		polynomial_term() = default;
		/// Defaulted copy constructor.
		polynomial_term(const polynomial_term &) = default;
		/// Defaulted move constructor.
		polynomial_term(polynomial_term &&) = default;
		PIRANHA_FORWARDING_CTOR(polynomial_term,base)
		/// Trivial destructor.
		~polynomial_term() = default;
		/// Defaulted copy assignment operator.
		polynomial_term &operator=(const polynomial_term &) = default;
		/// Defaulted move assignment operator.
		polynomial_term &operator=(polynomial_term &&) = default;
		/// Term multiplication.
		/**
		 * Multiplication of \p this by \p other will produce a single term whose coefficient is the
		 * result of the multiplication of the current coefficient by the coefficient of \p other
		 * and whose key is the element-by-element sum of the vectors of the exponents of the two terms.
		 * 
		 * The operator used for coefficient multiplication will be in-place multiplication
		 * if the coefficient types (\p Cf and \p Cf2) are not series, otherwise it will be
		 * the binary multiplication operator.
		 * 
		 * This method provides the basic exception safety guarantee: in face of exceptions, \p retval will be left in an
		 * undefined but valid state.
		 * 
		 * @param[out] retval return value of the multiplication.
		 * @param[in] other argument of the multiplication.
		 * @param[in] args reference set of arguments.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the assignment operators of the coefficient type,
		 * - the <tt>multiply()</tt> method of the key type,
		 * - the multiplication operators of the coefficient types.
		 */
		template <typename Cf2>
		void multiply(polynomial_term &retval, const polynomial_term<Cf2,Expo,S> &other, const symbol_set &args) const
		{
			cf_mult_impl(retval,other);
			this->m_key.multiply(retval.m_key,other.m_key,args);
		}
	private:
		// Overload if no coefficient is series.
		template <typename Cf2>
		void cf_mult_impl(polynomial_term &retval, const polynomial_term<Cf2,Expo,S> &other,
			typename std::enable_if<!is_instance_of<Cf,series>::value &&
			!is_instance_of<Cf2,series>::value>::type * = nullptr) const
		{
			retval.m_cf = this->m_cf;
			retval.m_cf *= other.m_cf;
		}
		// Overload if at least one coefficient is series.
		template <typename Cf2>
		void cf_mult_impl(polynomial_term &retval, const polynomial_term<Cf2,Expo,S> &other,
			typename std::enable_if<is_instance_of<Cf,series>::value ||
			is_instance_of<Cf2,series>::value>::type * = nullptr) const
		{
			retval.m_cf = this->m_cf * other.m_cf;
		}
};

}

#endif
