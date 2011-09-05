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

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "concepts/multaddable_coefficient.hpp"
#include "config.hpp"
#include "exceptions.hpp"
#include "polynomial_term.hpp"
#include "series_multiplier.hpp"
#include "symbol.hpp"
#include "top_level_series.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

struct polynomial_tag {};

}

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
 * \p Cf and \p Expo must be suitable for use in piranha::polynomial_term. Additionally, \p Cf must also be a model of piranha::concept::MultaddableCoefficient.
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
	public top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>,detail::polynomial_tag
{
		BOOST_CONCEPT_ASSERT((concept::MultaddableCoefficient<Cf>));
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
		/// Constructor from symbol name.
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
		 * This constructor, activated only if the number of arguments is at least 2 or if the only argument is not of type piranha::polynomial
		 * (disregarding cv-qualifications or references), will perfectly forward its arguments to a constructor in the base class.
		 * 
		 * @param[in] arg1 first argument for construction.
		 * @param[in] argn additional construction arguments.
		 * 
		 * @throws unspecified any exception thrown by the invoked base constructor.
		 */
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<polynomial,typename strip_cv_ref<T>::type>::value>::type*& = enabler>
		explicit polynomial(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {std::cout << "generic polyctor\n";}
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
		 * Will forward the assignment to the base class. This assignment operator is activated only when \p T is not
		 * piranha::polynomial and no other assignment operator applies.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the assignment operator in the base class.
		 */
		template <typename T>
		typename std::enable_if<!std::is_constructible<symbol,T &&>::value && !std::is_same<polynomial,typename strip_cv_ref<T>::type>::value,polynomial &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

/// Series multiplier tuned for polynomials.
/**
 * This piranha::series_multiplier specialisation is activated when both series are instances of
 * piranha::polynomial. It employs a specialised \p Functor that uses a multiply-accumulate operation
 * during coefficient arithmetic, that can result in faster computations (e.g., with piranha::integer coefficients or when
 * the host platform provides fast implementation of floating-point multiply-add operations).
 * 
 * Exception safety guarantee and move semantics are equivalent to the non-specialised series multiplier.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo use std::vector with custom allocator to make extra sure about cache alignment in m_tmp. NOTE: not sure this can be done,
 * would need hash set to be searchable with non-key types for this to work.
 */
template <typename Series1, typename Series2>
class series_multiplier<Series1,Series2,typename std::enable_if<std::is_base_of<detail::polynomial_tag,Series1>::value &&
	std::is_base_of<detail::polynomial_tag,Series2>::value>::type>:
	public series_multiplier<Series1,Series2,int>
{
		typedef series_multiplier<Series1,Series2,int> base;
	public:
		/// Constructor.
		/**
		 * Equivalent to the constructor of the non-specialised piranha::series_multiplier.
		 * 
		 * @param[in] s1 first multiplicand series.
		 * @param[in] s2 second multiplicand series.
		 */
		explicit series_multiplier(const Series1 &s1, const Series2 &s2):base(s1,s2) {}
		/// Multiplication implementations.
		/**
		 * Equivalent to calling piranha::series_multiplier::execute with a specialised \p Functor.
		 * 
		 * @param[in] ed piranha::echelon_descriptor that will be used to build (via repeated insertions)
		 * the result of the series multiplication.
		 * 
		 * @return the result of multiplying the two series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::base_series::insert(),
		 * - the <tt>multiply_accumulate()</tt> method of the coefficient type of \p Series1,
		 * - the <tt>multiply()</tt> method of the term type of \p Series1,
		 * - execute().
		 */
		template <typename Term>
		typename base::return_type operator()(const echelon_descriptor<Term> &ed) const
		{
			return base::template execute<poly_functor<Term>>(ed);
		}
	private:
		template <typename Term>
		struct poly_functor
		{
			explicit poly_functor(const std::vector<typename base::term_type1 const *> &v1, const std::vector<typename base::term_type2 const *> &v2,
				const echelon_descriptor<Term> &ed, typename base::return_type &retval):
				m_v1(v1),m_v2(v2),m_ed(ed),m_retval(retval)
			{}
			template <typename Size>
			void operator()(const Size &i, const Size &j) const
			{
				piranha_assert(i < m_v1.size() && j < m_v2.size());
				const auto &t1 = *m_v1[i];
				const auto &t2 = *m_v2[j];
				t1.m_key.multiply(m_tmp.m_key,t2.m_key,m_ed.template get_args<typename base::term_type1>());
				auto &container = m_retval.m_container;
				typedef decltype(container.bucket_count()) size_type;
				// Prepare the return series.
				if (unlikely(!container.bucket_count())) {
					container._increase_size();
				}
				// Try to locate the term into retval.
				size_type bucket_idx = container._bucket(m_tmp);
				const auto it = container._find(m_tmp,bucket_idx);
				if (it == container.end()) {
					if (unlikely(container.size() == boost::integer_traits<size_type>::const_max)) {
						piranha_throw(std::overflow_error,"maximum number of elements reached");
					}
					// Term is new. Handle the case in which we need to rehash because of load factor.
					if (unlikely(static_cast<double>(container.size() + size_type(1u)) / container.bucket_count() >
						container.max_load_factor()))
					{
						container._increase_size();
						// We need a new bucket index in case of a rehash.
						bucket_idx = container._bucket(m_tmp);
					}
					// Take care of multiplying the coefficient.
					t1.cf_mult_impl(m_tmp,t2,m_ed);
					// Insert and update size.
					container._unique_insert(m_tmp,bucket_idx);
					container._update_size(container.size() + 1u);
				} else {
					// Assert the existing term is not ignorable and it is compatible.
					piranha_assert(!it->is_ignorable(m_ed) && it->is_compatible(m_ed));
					// Cleanup function.
					auto cleanup = [&]() -> void {
						if (unlikely(!it->is_compatible(this->m_ed) || it->is_ignorable(this->m_ed))) {
							container.erase(it);
						}
					};
					try {
						it->m_cf.multiply_accumulate(t1.m_cf,t2.m_cf,m_ed);
						// Check if the term has become ignorable or incompatible after the modification.
						cleanup();
					} catch (...) {
						cleanup();
						throw;
					}
				}
			}
			const std::vector<typename base::term_type1 const *>	&m_v1;
			const std::vector<typename base::term_type2 const *>	&m_v2;
			const echelon_descriptor<Term>				&m_ed;
			typename base::return_type				&m_retval;
			mutable typename base::term_type1			m_tmp;
		};
};

}

#endif
