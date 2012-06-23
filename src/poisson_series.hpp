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

#ifndef PIRANHA_POISSON_SERIES_HPP
#define PIRANHA_POISSON_SERIES_HPP

#include <boost/concept/assert.hpp>
#include <type_traits>

#include "concepts/series.hpp"
#include "config.hpp"
#include "detail/polynomial_fwd.hpp"
#include "poisson_series_term.hpp"
#include "power_series.hpp"
#include "series.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Poisson series class.
/**
 * This class represents multivariate Poisson series as collections of multivariate Poisson series terms
 * (represented by the piranha::poisson_series_term class). The coefficient
 * type \p Cf represents the ring over which the Poisson series is defined.
 * 
 * This class is a model of the piranha::concept::Series concepts.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Cf must be suitable for use in piranha::poisson_series_term.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as piranha::power_series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::power_series's move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Cf>
class poisson_series:
	public power_series<series<poisson_series_term<Cf>,poisson_series<Cf>>>
{
		typedef power_series<series<poisson_series_term<Cf>,poisson_series<Cf>>> base;
	public:
		/// Defaulted default constructor.
		/**
		 * Will construct a Poisson series with zero terms.
		 */
		poisson_series() = default;
		/// Defaulted copy constructor.
		poisson_series(const poisson_series &) = default;
		/// Defaulted move constructor.
		poisson_series(poisson_series &&) = default;
		/// Constructor from symbol name.
		/**
		 * This constructor is enabled only if the coefficient type is an instance of piranha::polynomial
		 * and if type \p String can be used to construct a piranha::symbol.
		 * It will construct a Poisson series consisting of a single term with coefficient constructed from \p name,
		 * thus symbolically representing a variable called \p name.
		 * 
		 * @param[in] name name of the symbolic variable that the Poisson series will represent.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::symbol_set::add(),
		 * - the constructor of piranha::symbol from <tt>String &&</tt>,
		 * - the invoked constructor of the coefficient type,
		 * - the invoked constructor of the key type,
		 * - the constructor of the term type from coefficient and key,
		 * - piranha::series::insert().
		 */
		template <typename String>
		explicit poisson_series(String &&name,
			typename std::enable_if<std::is_base_of<detail::polynomial_tag,Cf>::value &&
			std::is_constructible<symbol,String &&>::value>::type * = piranha_nullptr) : base()
		{
			typedef typename base::term_type term_type;
			typename term_type::key_type key;
			// Construct and insert the term.
			this->insert(term_type(Cf(std::forward<String>(name)),std::move(key)));
		}
		/// Generic constructor.
		/**
		 * This constructor, activated only if the number of arguments is at least 2 or if the only argument is not of type piranha::poisson_series,
		 * will perfectly forward its arguments to a constructor in the base class.
		 * 
		 * @param[in] arg1 first argument for construction.
		 * @param[in] argn additional construction arguments.
		 * 
		 * @throws unspecified any exception thrown by the invoked base constructor.
		 */
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<poisson_series,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit poisson_series(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
		/// Trivial destructor.
		~poisson_series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Series<poisson_series>));
		}
		/// Defaulted copy assignment operator.
		poisson_series &operator=(const poisson_series &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		poisson_series &operator=(poisson_series &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		/// Assignment from symbol name.
		/**
		 * Equivalent to invoking the constructor from symbol name and assigning the result to \p this.
		 * 
		 * @param[in] name name of the symbolic variable that the Poisson series will represent.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the constructor from symbol name.
		 */
		template <typename String>
		typename std::enable_if<std::is_base_of<detail::polynomial_tag,Cf>::value &&
			std::is_constructible<symbol,String &&>::value,poisson_series &>::type operator=(String &&name)
		{
			operator=(poisson_series(std::forward<String>(name)));
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * Will forward the assignment to the base class. This assignment operator is activated only when \p T is not
		 * piranha::poisson_series and no other assignment operator applies.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the assignment operator in the base class.
		 */
		template <typename T>
		typename std::enable_if<(!std::is_constructible<symbol,T &&>::value ||
			!std::is_base_of<detail::polynomial_tag,Cf>::value) &&
			!std::is_same<poisson_series,typename std::decay<T>::type>::value,poisson_series &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

}

#endif
