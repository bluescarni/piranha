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

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "base_term.hpp"
#include "detail/series_fwd.hpp"
#include "kronecker_monomial.hpp"
#include "math.hpp"
#include "monomial.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "univariate_monomial.hpp"
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
struct polynomial_term_key<univariate_monomial<T>,S>
{
	typedef univariate_monomial<T> type;
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
 * - if \p Expo is piranha::univariate_monomial of \p T, the key will also be piranha::univariate_monomial of \p T,
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
 polynomial_term<double,univariate_monomial<int>>
 @endcode
 * is a univariate polynomial term with double-precision coefficient and \p int exponent.
 @code
 polynomial_term<double,kronecker_monomial<>>
 @endcode
 * is a multivariate polynomial term with double-precision coefficient and integral exponents packed into a piranha::kronecker_monomial.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Cf must be suitable for use in piranha::base_term;
 * - \p Cf must satisfy the following type traits:
 *   - piranha::is_multipliable and piranha::is_multipliable_in_place,
 *   - piranha::has_multiply_accumulate.
 * - \p Expo and \p S must be suitable for use in piranha::monomial, or \p Expo must be an instance of
 *   piranha::univariate_monomial or piranha::kronecker_monomial.
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
		// MP for enabling partial derivative.
		template <typename Cf2, typename Key2, typename = void>
		struct partial_enabler
		{
			static const bool value = false;
		};
		template <typename Cf2, typename Key2>
		struct partial_enabler<Cf2,Key2,typename std::enable_if<is_differentiable<Cf2>::value &&
			is_multipliable<const Cf2 &,decltype(std::declval<Key2 const &>().partial(std::declval<const symbol &>(),
			std::declval<const symbol_set &>()).first)>::value>::type>
		{
			typedef decltype(math::partial(std::declval<Cf2>(),std::declval<std::string>())) diff_type;
			typedef decltype(std::declval<Key2 const &>().partial(std::declval<const symbol &>(),
				std::declval<const symbol_set &>()).first) key_diff_type;
			static const bool value = has_is_zero<diff_type>::value &&
						  has_is_zero<key_diff_type>::value &&
						  std::is_move_constructible<diff_type>::value &&
						  std::is_constructible<polynomial_term,diff_type,const Key2 &>::value &&
						  std::is_constructible<polynomial_term,decltype(std::declval<const Cf2 &>() *
						  std::declval<key_diff_type &>()),Key2>::value;
		};
	public:
		/// Result type for the multiplication by another term.
		typedef polynomial_term multiplication_result_type;
		/// Inherited constructors.
		using base::base;
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
		/// Partial derivative.
		/**
		 * \note
		 * This method is enabled if:
		 * - the coefficient type satisfies piranha::is_differentiable, it is multipliable by the scalar type
		 *   returned by the differentiation method of the monomial and it satisfies piranha::has_is_zero,
		 * - the type resulting from the differentiation of the coefficient type is move-constructible,
		 * - the scalar type returned by the differentiation method of the monomial satisfies piranha::has_is_zero,
		 * - the polynomial term can be constructed from coefficient-key argument pairs resulting from the arithmetic operations
		 *   necessary to compute the derivative.
		 * 
		 * Will return a vector of polynomial terms representing the partial derivative of \p this with respect to
		 * symbol \p s. The partial derivative is computed via piranha::math::partial() and the differentiation method
		 * of the monomial type.
		 * 
		 * @param[in] s piranha::symbol with respect to which the derivative will be calculated.
		 * @param[in] args reference set of arguments.
		 * 
		 * @return partial derivative of \p this with respect to \p s, represented as a vector of terms.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::partial() and piranha::math::is_zero(),
		 * - coefficient, term and key constructors,
		 * - memory allocation errors in standard containers,
		 * - the differentiation method of the monomial type,
		 * - the multiplication operator of the coefficient type.
		 */
		template <typename T = Cf, typename U = typename base::key_type,
			typename = typename std::enable_if<partial_enabler<T,U>::value>::type>
		std::vector<polynomial_term> partial(const symbol &s, const symbol_set &args) const
		{
			std::vector<polynomial_term> retval;
			auto cf_partial = math::partial(this->m_cf,s.get_name());
			if (!math::is_zero(cf_partial)) {
				retval.push_back(polynomial_term(std::move(cf_partial),this->m_key));
			}
			auto key_partial = this->m_key.partial(s,args);
			if (!math::is_zero(key_partial.first)) {
				retval.push_back(polynomial_term(this->m_cf * key_partial.first,std::move(key_partial.second)));
			}
			return retval;
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
