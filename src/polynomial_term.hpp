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

#include <boost/concept/assert.hpp>
#include <type_traits>

#include "base_term.hpp"
#include "concepts/multipliable_coefficient.hpp"
#include "concepts/multipliable_term.hpp"
#include "config.hpp"
#include "detail/series_fwd.hpp"
#include "detail/series_multiplier_fwd.hpp"
#include "kronecker_monomial.hpp"
#include "monomial.hpp"
#include "power_series_term.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"
#include "univariate_monomial.hpp"

namespace piranha
{

namespace detail
{

template <typename T>
struct polynomial_term_key
{
	typedef monomial<T> type;
};

template <typename T>
struct polynomial_term_key<univariate_monomial<T>>
{
	typedef univariate_monomial<T> type;
};

template <typename T>
struct polynomial_term_key<kronecker_monomial<T>>
{
	typedef kronecker_monomial<T> type;
};

}

/// Polynomial term.
/**
 * This class extends piranha::base_term for use in polynomials. The coefficient type \p Cf is generic,
 * the key type is determined as follows:
 * 
 * - if \p ExpoType is piranha::univariate_monomial of \p T, the key will also be piranha::univariate_monomial of \p T,
 * - if \p ExpoType is piranha::kronecker_monomial of \p T, the key will also be piranha::kronecker_monomial of \p T,
 * - otherwise, the key will be piranha::monomial of \p ExpoType.
 * 
 * Examples:
 * @code
 * polynomial_term<double,int>
 * @endcode
 * is a multivariate polynomial term with double-precision coefficient and \p int exponents.
 * @code
 * polynomial_term<double,static_size<int,5>>
 * @endcode
 * is a multivariate polynomial term with double-precision coefficient and a maximum of 5 \p int exponents.
 * @code
 * polynomial_term<double,univariate_monomial<int>>
 * @endcode
 * is a univariate polynomial term with double-precision coefficient and \p int exponent.
 * @code
 * polynomial_term<double,kronecker_monomial<>>
 * @endcode
 * is a multivariate polynomial term with double-precision coefficient and integral exponents packed into a piranha::kronecker_monomial.
 * 
 * This class is a model of the piranha::concept::MultipliableTerm concept.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Cf must be a model of piranha::concept::MultipliableCoefficient.
 * - \p ExpoType must be suitable for use in piranha::monomial, or be piranha::univariate_monomial or piranha::kronecker_monomial.
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
class polynomial_term: public power_series_term<base_term<Cf,typename detail::polynomial_term_key<ExpoType>::type,polynomial_term<Cf,ExpoType>>>
{
		BOOST_CONCEPT_ASSERT((concept::MultipliableCoefficient<Cf>));
		typedef power_series_term<base_term<Cf,typename detail::polynomial_term_key<ExpoType>::type,polynomial_term<Cf,ExpoType>>> base;
		// Make friend with series multipliers.
		template <typename Series1, typename Series2, typename Enable>
		friend class series_multiplier;
	public:
		/// Result type for the multiplication by another term.
		typedef polynomial_term multiplication_result_type;
		/// Defaulted default constructor.
		polynomial_term() = default;
		/// Defaulted copy constructor.
		polynomial_term(const polynomial_term &) = default;
		/// Defaulted move constructor.
		polynomial_term(polynomial_term &&) = default;
		/// Generic constructor.
		/**
		 * Will perfectly forward all arguments to a matching constructor in piranha::base_term.
		 * This constructor is enabled only if the number of variadic arguments is nonzero or
		 * if \p T is not of the same type as \p this.
		 * 
		 * @param[in] arg1 first argument to be forwarded to the base constructor.
		 * @param[in] params other arguments to be forwarded to the base constructor.
		 * 
		 * @throws unspecified any exception thrown by the invoked constructor in piranha::base_term.
		 */
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<polynomial_term,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit polynomial_term(T &&arg1, Args && ... params):base(std::forward<T>(arg1),std::forward<Args>(params)...) {}
		/// Trivial destructor.
		~polynomial_term() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::MultipliableTerm<polynomial_term>));
		}
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
		template <typename Cf2, typename ExpoType2>
		void multiply(polynomial_term &retval, const polynomial_term<Cf2,ExpoType2> &other, const symbol_set &args) const
		{
			cf_mult_impl(retval,other);
			this->m_key.multiply(retval.m_key,other.m_key,args);
		}
	private:
		// Overload if no coefficient is series.
		template <typename Cf2, typename ExpoType2>
		void cf_mult_impl(polynomial_term &retval, const polynomial_term<Cf2,ExpoType2> &other,
			typename std::enable_if<!std::is_base_of<detail::series_tag,Cf>::value &&
			!std::is_base_of<detail::series_tag,Cf2>::value>::type * = piranha_nullptr) const
		{
			retval.m_cf = this->m_cf;
			retval.m_cf *= other.m_cf;
		}
		// Overload if at least one coefficient is series.
		template <typename Cf2, typename ExpoType2>
		void cf_mult_impl(polynomial_term &retval, const polynomial_term<Cf2,ExpoType2> &other,
			typename std::enable_if<std::is_base_of<detail::series_tag,Cf>::value ||
			std::is_base_of<detail::series_tag,Cf2>::value>::type * = piranha_nullptr) const
		{
			retval.m_cf = this->m_cf * other.m_cf;
		}
};

}

#endif
