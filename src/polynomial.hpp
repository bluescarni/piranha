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

#ifndef PIRANHA_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIAL_HPP

#include <type_traits>

#include "config.hpp"
#include "polynomial_term.hpp"
#include "symbol.hpp"
#include "top_level_series.hpp"

namespace piranha
{

/// Polynomial class.
/**
 * This class represents multivariate polynomials as collections of multivariate polynomial terms
 * (represented by the piranha::polynomial_term class). The coefficient
 * type \p Cf represents the ring over which the polynomial is defined, while \p Expo is the type used to represent
 * the value of the exponents. Depending on \p Expo, the class can represent various types of polynomials, including
 * Laurent polynomials and Puiseux polynomials.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Cf and \p ExpoType must be suitable for use in piranha::polynomial_term.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as piranha::top_level_series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::top_level_series's move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Cf, typename Expo>
class polynomial:
	public top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>
{
		typedef top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>> base;
	public:
		/// Defaulted default constructor.
		/**
		 * Will construct a polynomial with zero terms.
		 */
		polynomial() = default;
		/// Defaulted copy constructor.
		polynomial(const polynomial &) = default;
		/// Defaulted move constructor.
		polynomial(polynomial &&) = default;
		/// Constrcutor from symbol name.
		/**
		 * Will construct a univariate polynomial made of a single term with unitary coefficient and exponent, representing
		 * the symbolic variable \p name.
		 * 
		 * For this constructor to work, the coefficient type must be provided with a 2-arguments constructor from
		 * the literal constant 1 and the piranha::echelon_descriptor of \p this. The key will be constructed by invoking
		 * the constructor from the initializer list <tt>{1}</tt>.
		 * 
		 * This template constructor is activated iff the type <tt>String &&</tt> can be used to construct a piranha::symbol.
		 * 
		 * @param[in] name name of the symbolic variable that the polynomial will represent.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::echelon_descriptor::add_symbol,
		 * - the constructor of piranha::symbol from <tt>String &&</tt>,
		 * - the invoked constructor of the coefficient type,
		 * - the invoked constructor of the key type,
		 * - the constructor of the term type from coefficient and key,
		 * - piranha::base_series::insert().
		 */
		template <typename String>
		explicit polynomial(String &&name,
			typename std::enable_if<std::is_constructible<symbol,String &&>::value>::type * = piranha_nullptr) : base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(std::forward<String>(name)));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{1}),this->m_ed);
		}
		/// Generic constructor.
		/**
		 * Will perfectly forward the arguments to a constructor in the base class.
		 * 
		 * @param[in] params construction arguments.
		 * 
		 * @throws unspecified any exception thrown by the invoked base constructor.
		 */
		template <typename... Args>
		explicit polynomial(Args && ... params) : base(std::forward<Args>(params)...) {}
		/// Defaulted destructor.
		~polynomial() = default;
		/// Defaulted copy assignment operator.
		polynomial &operator=(const polynomial &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		polynomial &operator=(polynomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		/// Assignment from symbol name.
		/**
		 * Equivalent to invoking the constructor from symbol name and assigning the result to \p this.
		 * 
		 * @param[in] name name of the symbolic variable that the polynomial will represent.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the constructor from symbol name.
		 */
		template <typename String>
		typename std::enable_if<std::is_constructible<symbol,String &&>::value,polynomial &>::type operator=(String &&name)
		{
			operator=(polynomial(std::forward<String>(name)));
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * Will forward the assignment to the base class.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the assignment operator in the base class.
		 */
		template <typename T>
		typename std::enable_if<!std::is_constructible<symbol,T &&>::value,polynomial &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

}

#endif
