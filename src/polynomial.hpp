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
#include <tuple>
#include <type_traits>
#include <vector>

#include "concepts/multipliable_coefficient.hpp"
#include "concepts/series.hpp"
#include "concepts/truncator.hpp"
#include "config.hpp"
#include "degree_truncator_settings.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "polynomial_term.hpp"
#include "power_series.hpp"
#include "power_series_truncator.hpp"
#include "series.hpp"
#include "series_multiplier.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "truncator.hpp"
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
 * This class is a model of the piranha::concept::Series and piranha::concept::MultipliableCoefficient concepts.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Cf and \p Expo must be suitable for use in piranha::polynomial_term.
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
template <typename Cf, typename Expo>
class polynomial:
	public power_series<series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>>,
	detail::polynomial_tag
{
		typedef power_series<series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>> base;
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
		 * The coefficient type must be constructible from the literal constant 1.
		 * 
		 * This template constructor is activated iff the type <tt>String &&</tt> can be used to construct a piranha::symbol.
		 * 
		 * @param[in] name name of the symbolic variable that the polynomial will represent.
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
		explicit polynomial(String &&name,
			typename std::enable_if<std::is_constructible<symbol,String &&>::value>::type * = piranha_nullptr) : base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(symbol(std::forward<String>(name)));
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{1}));
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
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<polynomial,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit polynomial(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
		/// Trivial destructor.
		~polynomial() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Series<polynomial>));
			BOOST_CONCEPT_ASSERT((concept::MultipliableCoefficient<polynomial>));
		}
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
		typename std::enable_if<!std::is_constructible<symbol,T &&>::value && !std::is_same<polynomial,typename std::decay<T>::type>::value,polynomial &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

/// Specialisation of unary piranha::truncator for piranha::polynomial.
/**
 * This specialisation will use piranha::power_series_truncator to rank terms according to their low degree.
 * 
 * \section type_requirements Type requirements
 * 
 * This specialisation is activated for instances of piranha::polynomial.
 */
template <typename Cf, typename Expo>
class truncator<polynomial<Cf,Expo>>: public power_series_truncator
{
		BOOST_CONCEPT_ASSERT((concept::Truncator<polynomial<Cf,Expo>>));
	public:
		/// Alias for the polynomial type.
		typedef polynomial<Cf,Expo> polynomial_type;
		/// Alias for the polynomial term type.
		typedef typename polynomial_type::term_type term_type;
		/// Constructor from polynomial.
		/**
		 * The constructor will store internally a reference to the input series and will invoke the default
		 * constructor of piranha::power_series_truncator.
		 * 
		 * @param[in] poly series on which the truncator will operate.
		 * 
		 * @throws unspecified any exception thrown by the default constructor of piranha::power_series_truncator.
		 */
		explicit truncator(const polynomial_type &poly):power_series_truncator(),m_poly(poly) {}
		/// Deleted copy assignment.
		truncator &operator=(const truncator &) = delete;
		/// Deleted move assignment.
		truncator &operator=(truncator &&) = delete;
		/// Term comparison.
		/**
		 * This method will compare the partial or total low degree (depending on the truncator settings) of \p t1 and \p t2,
		 * and will return \p true if the low degree of \p t1 is less than the low degree of \p t2, \p false otherwise.
		 * If the truncator is inactive, a runtime error will be produced.
		 * 
		 * @param[in] t1 first term.
		 * @param[in] t2 second term.
		 * 
		 * @return the result of comparing the low degrees of \p t1 and \p t2.
		 * 
		 * @throws std::invalid_argument if the truncator is inactive.
		 * @throws unspecified any exception thrown by the calculation and comparison of the low degrees of the terms.
		 */
		bool compare_terms(const term_type &t1, const term_type &t2) const
		{
			switch (std::get<0u>(m_state)) {
				case total:
					return compare_ldegree(t1,t2,m_poly.m_symbol_set);
				case partial:
					return compare_pldegree(t1,t2,m_poly.m_symbol_set);
				default:
					piranha_throw(std::invalid_argument,"cannot compare terms if truncator is inactive");
			}
		}
	private:
		const polynomial_type &m_poly;
};

/// Specialisation of binary piranha::truncator for piranha::polynomial.
/**
 * This specialisation will use piranha::power_series_truncator to rank terms according to their low degree.
 * 
 * \section type_requirements Type requirements
 * 
 * This specialisation is activated for instances of piranha::polynomial. If the echelon sizes of the two polynomial term types
 * are not equal, a compile-time error will be produced.
 */
template <typename Cf1, typename Expo1, typename Cf2, typename Expo2>
class truncator<polynomial<Cf1,Expo1>,polynomial<Cf2,Expo2>>: public power_series_truncator
{
		BOOST_CONCEPT_ASSERT((concept::Truncator<polynomial<Cf1,Expo1>>));
		BOOST_CONCEPT_ASSERT((concept::Truncator<polynomial<Cf2,Expo2>>));
	public:
		/// Alias for the first polynomial type.
		typedef polynomial<Cf1,Expo1> polynomial_type1;
		/// Alias for the second polynomial type.
		typedef polynomial<Cf2,Expo2> polynomial_type2;
		/// Alias for the first polynomial term type.
		typedef typename polynomial_type1::term_type term_type1;
		/// Alias for the second polynomial term type.
		typedef typename polynomial_type2::term_type term_type2;
	private:
		static_assert(echelon_size<term_type1>::value == echelon_size<term_type2>::value,"Inconsistent echelon sizes.");
	public:
		/// Constructor from polynomials.
		/**
		 * The constructor will store internally references to the input series and will invoke the default
		 * constructor of piranha::power_series_truncator.
		 * 
		 * @param[in] poly1 first series on which the truncator will operate.
		 * @param[in] poly2 second series on which the truncator will operate.
		 * 
		 * @throws std::invalid_argument if the arguments sets of \p poly1 and \p poly2 differ.
		 * @throws unspecified any exception thrown by the default constructor of piranha::power_series_truncator.
		 */
		explicit truncator(const polynomial_type1 &poly1, const polynomial_type2 &poly2):power_series_truncator(),
			m_poly1(poly1),m_poly2(poly2)
		{
			if (unlikely(m_poly1.m_symbol_set != m_poly2.m_symbol_set)) {
				piranha_throw(std::invalid_argument,"incompatible sets of arguments");
			}
		}
		/// Deleted copy assignment.
		truncator &operator=(const truncator &) = delete;
		/// Deleted move assignment.
		truncator &operator=(truncator &&) = delete;
		/// Term comparison.
		/**
		 * This method will compare the partial or total low degree (depending on the truncator settings) of \p t1 and \p t2,
		 * and will return \p true if the low degree of \p t1 is less than the low degree of \p t2, \p false otherwise.
		 * If the truncator is inactive, a runtime error will be produced.
		 * 
		 * This template method is activated only if \p Term is \p term_type1 or \p term_type2. If \p Term is \p term_type1 the set
		 * of arguments used to calculate and compare the degree of the terms will be the one of the first series, otherwise the set of arguments
		 * of the second series will be used.
		 * 
		 * @param[in] t1 first term.
		 * @param[in] t2 second term.
		 * 
		 * @return the result of comparing the low degrees of \p t1 and \p t2.
		 * 
		 * @throws std::invalid_argument if the truncator is inactive.
		 * @throws unspecified any exception thrown by the calculation and comparison of the low degrees of the terms.
		 */
		template <typename Term>
		bool compare_terms(const Term &t1, const Term &t2, typename std::enable_if<std::is_same<Term,term_type1>::value ||
			std::is_same<Term,term_type2>::value>::type * = piranha_nullptr) const
		{
			const auto &args = (std::is_same<Term,term_type1>::value) ? m_poly1.m_symbol_set : m_poly2.m_symbol_set;
			switch (std::get<0u>(m_state)) {
				case total:
					return compare_ldegree(t1,t2,args);
				case partial:
					return compare_pldegree(t1,t2,args);
				default:
					piranha_throw(std::invalid_argument,"cannot compare terms if truncator is inactive");
			}
		}
		/// Skip next term multiplications.
		/**
		 * This method will return \p true if the multiplication of all terms following \p t1 and \p t2 in the ordering established by
		 * compare_terms() can be skipped, given the current truncation settings, \p false otherwise.
		 * 
		 * @param[in] t1 first term.
		 * @param[in] t2 second term.
		 * 
		 * @return \p true if all next term-by-term multiplications can be skipped, false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the calculation and comparison of the low degrees of the terms.
		 */
		bool skip(const term_type1 &t1, const term_type2 &t2) const
		{
			switch (std::get<0u>(m_state)) {
				case total:
					return (t1.ldegree(m_poly1.m_symbol_set) + t2.ldegree(m_poly2.m_symbol_set)) >= std::get<1u>(m_state);
				case partial:
					return (t1.ldegree(std::get<2u>(m_state),m_poly1.m_symbol_set) +
						t2.ldegree(std::get<2u>(m_state),m_poly2.m_symbol_set)) >= std::get<1u>(m_state);
				default:
					return false;
			}
		}
	private:
		const polynomial_type1	&m_poly1;
		const polynomial_type2	&m_poly2;
};

#if 0
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

#endif

}

#endif
