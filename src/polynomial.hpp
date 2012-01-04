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
#include <boost/algorithm/minmax_element.hpp>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath> // For std::ceil.
#include <initializer_list> // NOTE: this could go away when there's no need to use it explicitly, see below.
#include <iterator>
#include <list>
#include <numeric>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "cache_aligning_allocator.hpp"
#include "concepts/multipliable_coefficient.hpp"
#include "concepts/series.hpp"
#include "concepts/truncator.hpp"
#include "config.hpp"
#include "degree_truncator_settings.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "math.hpp"
#include "polynomial_term.hpp"
#include "power_series.hpp"
#include "power_series_truncator.hpp"
#include "series.hpp"
#include "series_multiplier.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "threading.hpp"
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
 * 
 * \todo here we could optimize the comparisons with the state snapshot by creating another snapshot in case the degree_type is not integer ->
 * cast the degree limit to degree_type for faster comparisons with builtin C++ types.
 * \todo also, hash_set on the degrees of the terms of the input series for optimization.
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
		/// Defaulted copy constructor.
		truncator(const truncator &) = default;
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
		/// Filter term.
		/**
		 * @param[in] t term argument.
		 * 
		 * @return \p true if \p t can be discarded given the current truncation settings, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by piranha::power_series_truncator::filter_term().
		 */
		bool filter(const term_type &t) const
		{
			return filter_term(t,m_poly.m_symbol_set);
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
		/// Defaulted copy constructor.
		truncator(const truncator &) = default;
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
		/// Filter term.
		/**
		 * @param[in] t term argument.
		 * 
		 * @return \p true if \p t can be discarded given the current truncation settings, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by piranha::power_series_truncator::filter_term().
		 */
		bool filter(const term_type1 &t) const
		{
			return filter_term(t,m_poly1.m_symbol_set);
		}
		/// Skip term multiplications.
		/**
		 * This method will return \p true if the multiplication of \p t1 by \p t2 and of all terms following \p t1 and \p t2 in the ordering established by
		 * compare_terms() can be skipped, given the current truncation settings, \p false otherwise.
		 * 
		 * @param[in] t1 first term.
		 * @param[in] t2 second term.
		 * 
		 * @return \p true if the result of the multiplication of \p t1 by \p t2 and of all subsequent term-by-term multiplications
		 * can be discarded, \p false otherwise.
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

namespace detail
{

template <typename Series1, typename Series2>
struct kronecker_enabler
{
	BOOST_CONCEPT_ASSERT((concept::Series<Series1>));
	BOOST_CONCEPT_ASSERT((concept::Series<Series2>));
	template <typename Key1, typename Key2>
	struct are_same_kronecker_monomial
	{
		static const bool value = false;
	};
	template <typename T>
	struct are_same_kronecker_monomial<kronecker_monomial<T>,kronecker_monomial<T>>
	{
		static const bool value = true;
	};
	typedef typename Series1::term_type::key_type key_type1;
	typedef typename Series2::term_type::key_type key_type2;
	static const bool value = std::is_base_of<detail::polynomial_tag,Series1>::value &&
		std::is_base_of<detail::polynomial_tag,Series2>::value && are_same_kronecker_monomial<key_type1,key_type2>::value;
};

}

/// Series multiplier specialisation for polynomials with Kronecker monomials.
/**
 * This specialisation of piranha::series_multiplier is enabled when both \p Series1 and \p Series2 are instances of
 * piranha::polynomial with monomials represented as piranha::kronecker_monomial of the same type.
 * This multiplier will employ optimized algorithms that take advantage of the properties of Kronecker monomials.
 * It will also take advantage of piranha::math::multiply_accumulate() in place of plain coefficient multiplication
 * when possible.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as the non-specialised piranha::series_multiplier.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::series_multiplier's move semantics.
 * 
 * \todo optimize task list in single thread and maybe also for small operands.
 */
template <typename Series1, typename Series2>
class series_multiplier<Series1,Series2,typename std::enable_if<detail::kronecker_enabler<Series1,Series2>::value>::type>:
	public series_multiplier<Series1,Series2,int>
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series1>));
		BOOST_CONCEPT_ASSERT((concept::Series<Series2>));
		typedef typename Series1::term_type::key_type::value_type value_type;
		typedef kronecker_array<value_type> ka;
	public:
		/// Base multiplier type.
		typedef series_multiplier<Series1,Series2,int> base;
		/// Alias for term type of \p Series1.
		typedef typename Series1::term_type term_type1;
		/// Alias for term type of \p Series2.
		typedef typename Series2::term_type term_type2;
		/// Alias for the return type.
		typedef typename base::return_type return_type;
		/// Alias for the truncator type.
		typedef truncator<Series1,Series2> truncator_type;
		/// Constructor.
		/**
		 * Will call the base constructor and additionally check that the result of the multiplication will not overflow
		 * the representation limits of piranha::kronecker_monomial. In such a case, a runtime error will be produced.
		 * 
		 * @param[in] s1 first series operand.
		 * @param[in] s2 second series operand.
		 * 
		 * @throws std::invalid_argument if the the result of the multiplication overflows the representation limits of
		 * piranha::kronecker_monomial.
		 * @throws unspecified any exception thrown by the base constructor.
		 */
		explicit series_multiplier(const Series1 &s1, const Series2 &s2):base(s1,s2)
		{
			if (unlikely(this->m_s1->empty() || this->m_s2->empty())) {
				return;
			}
			// NOTE: here we are sure about this since the symbol set in a series should never
			// overflow the size of the limits, as the check for compatibility in Kronecker monomial
			// would kick in.
			piranha_assert(this->m_s1->m_symbol_set.size() < ka::get_limits().size());
			piranha_assert(this->m_s1->m_symbol_set == this->m_s2->m_symbol_set);
			const auto &limits = ka::get_limits()[this->m_s1->m_symbol_set.size()];
			// NOTE: We need to check that the exponents of the monomials in the result do not
			// go outside the bounds of the Kronecker codification. We need to unpack all monomials
			// in the operands and examine them, we cannot operate on the codes for this.
			auto it1 = this->m_v1.begin();
			auto it2 = this->m_v2.begin();
			typedef typename term_type1::key_type::value_type value_type1;
			typedef typename term_type2::key_type::value_type value_type2;
			// Initialise minmax values.
			std::vector<std::pair<value_type1,value_type1>> minmax_values1;
			std::vector<std::pair<value_type2,value_type2>> minmax_values2;
			auto tmp_vec1 = (*it1)->m_key.unpack(this->m_s1->m_symbol_set);
			auto tmp_vec2 = (*it2)->m_key.unpack(this->m_s1->m_symbol_set);
			// Bounds of the Kronecker representation for each component.
			const auto &minmax_vec = std::get<0u>(limits);
			std::transform(tmp_vec1.begin(),tmp_vec1.end(),std::back_inserter(minmax_values1),[](const value_type &v) {
				return std::make_pair(v,v);
			});
			std::transform(tmp_vec2.begin(),tmp_vec2.end(),std::back_inserter(minmax_values2),[](const value_type &v) {
				return std::make_pair(v,v);
			});
			// Find the minmaxs.
			for (; it1 != this->m_v1.end(); ++it1) {
				tmp_vec1 = (*it1)->m_key.unpack(this->m_s1->m_symbol_set);
				piranha_assert(tmp_vec1.size() == minmax_values1.size());
				std::transform(minmax_values1.begin(),minmax_values1.end(),tmp_vec1.begin(),minmax_values1.begin(),
					[](const std::pair<value_type1,value_type1> &p, const value_type1 &v) {
						return std::make_pair(
							v < p.first ? v : p.first,
							v > p.second ? v : p.second
						);
				});
			}
			for (; it2 != this->m_v2.end(); ++it2) {
				tmp_vec2 = (*it2)->m_key.unpack(this->m_s2->m_symbol_set);
				piranha_assert(tmp_vec2.size() == minmax_values2.size());
				std::transform(minmax_values2.begin(),minmax_values2.end(),tmp_vec2.begin(),minmax_values2.begin(),
					[](const std::pair<value_type2,value_type2> &p, const value_type2 &v) {
						return std::make_pair(
							v < p.first ? v : p.first,
							v > p.second ? v : p.second
						);
				});
			}
			// Compute the sum of the two minmaxs, using multiprecision to avoid overflow.
			// NOTE: use m_minmax_values for the ranges of the result only, update it to include
			// the ranges of the operands below.
			std::transform(minmax_values1.begin(),minmax_values1.end(),minmax_values2.begin(),
				std::back_inserter(m_minmax_values),[](const std::pair<value_type1,value_type1> &p1,
				const std::pair<value_type2,value_type2> &p2) {
					return std::make_pair(integer(p1.first) + integer(p2.first),integer(p1.second) + integer(p2.second));
			});
			piranha_assert(m_minmax_values.size() == minmax_vec.size());
			piranha_assert(m_minmax_values.size() == minmax_values1.size());
			piranha_assert(m_minmax_values.size() == minmax_values2.size());
			for (decltype(m_minmax_values.size()) i = 0u; i < m_minmax_values.size(); ++i) {
				if (unlikely(m_minmax_values[i].first < -minmax_vec[i] || m_minmax_values[i].second > minmax_vec[i])) {
					piranha_throw(std::overflow_error,"Kronecker monomial components are out of bounds");
				}
				// Update with the ranges of the operands.
				// NOTE: the fact that we have to use std::initializer_list explicitly here seems
				// a compiler bug, should probably investigate with GCC > 4.5.
				m_minmax_values[i] = std::minmax(std::initializer_list<integer>({m_minmax_values[i].first,
					integer(minmax_values1[i].first),integer(minmax_values2[i].first),m_minmax_values[i].second,
					integer(minmax_values1[i].second),integer(minmax_values2[i].second)}));
			}
		}
		/// Perform multiplication.
		/**
		 * @return the result of the multiplication of the input series operands.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - (unlikely) conversion errors between numeric types,
		 * - the public interface of piranha::hash_set,
		 * - piranha::series_multiplier::estimate_final_series_size(),
		 * - piranha::series_multiplier::blocked_multiplication(),
		 * - piranha::math::multiply_accumulate() on the coefficient types.
		 */
		return_type operator()() const
		{
			truncator_type trunc(*this->m_s1,*this->m_s2);
			return execute(trunc);
		}
	private:
		typedef typename std::vector<term_type1 const *>::size_type index_type;
		typedef typename Series1::size_type bucket_size_type;
		// This is a bucket region, i.e., a _closed_ interval [a,b] of bucket indices in a hash set.
		typedef std::pair<bucket_size_type,bucket_size_type> region_type;
		// Block-by-block multiplication task.
		struct task_type
		{
			// First block: semi-open range [a,b[ of indices in first input series.
			std::pair<index_type,index_type>	m_b1;
			// Second block (indices in second input series).
			std::pair<index_type,index_type>	m_b2;
			// First region memory region involved in the multiplication.
			region_type				m_r1;
			// Second memory region.
			region_type				m_r2;
			// Boolean flag to signal if the second region is present or not.
			bool					m_second_region;
		};
		// Create task from indices i in first series, j in second series (semi-open intervals).
		task_type task_from_indices(const index_type &i_start, const index_type &i_end,
			const index_type &j_start, const index_type &j_end, const return_type &retval) const
		{
			const auto &v1 = this->m_v1;
			const auto &v2 = this->m_v2;
			piranha_assert(i_start < i_end && j_start < j_end);
			piranha_assert(i_end <= v1.size() && j_end <= v2.size());
			const auto b_count = retval.m_container.bucket_count();
			piranha_assert(b_count > 0u);
			region_type r1{0u,0u}, r2{0u,0u};
			bool second_region = false;
			// Addition is safe because of the limits on the max bucket count of hash_set.
			const auto a = retval.m_container._bucket_from_hash(v1[i_start]->hash()) +
				retval.m_container._bucket_from_hash(v2[j_start]->hash()),
				// NOTE: we are sure that the tasks are not empty, so the -1 is safe.
				b = retval.m_container._bucket_from_hash(v1[i_end - 1u]->hash()) +
				retval.m_container._bucket_from_hash(v2[j_end - 1u]->hash());
			// NOTE: using <= here as [a,b] is now a closed interval.
			piranha_assert(a <= b);
			piranha_assert(b <= b_count * 2u - 2u);
			if (b < b_count || a >= b_count) {
				r1.first = a % b_count;
				r1.second = b % b_count;
			} else {
				second_region = true;
				r1.first = a;
				r1.second = b_count - 1u;
				r2.first = 0u;
				r2.second = b % b_count;
			}
			return task_type{{i_start,i_end},{j_start,j_end},r1,r2,second_region};
		}
		// Functor to check if region r does not overlap any of the busy ones.
		template <typename RegionSet>
		static bool region_checker(const region_type &r, const RegionSet &busy_regions)
		{
			if (busy_regions.empty()) {
				return true;
			}
			// NOTE: lower bound means that it_b->first >= r.first.
			auto it_b = busy_regions.lower_bound(r);
			// Handle end().
			if (it_b == busy_regions.end()) {
				// NOTE: safe because busy_regions is not empty.
				--it_b;
				// Now check that the region in it_b does not overlap with
				// the current one.
				piranha_assert(it_b->first < r.first);
				return it_b->second < r.first;
			}
			if (r.second >= it_b->first) {
				// The end point of r1 overlaps the start point of *it_b, no good.
				return false;
			}
			// Lastly, we have to check that the previous element (if any) does not
			// overlap r.first.
			if (it_b != busy_regions.begin()) {
				--it_b;
				if (it_b->second >= r.first) {
					return false;
				}
			}
			return true;
		}
		// Check that a set of busy regions is well-formed, for debug purposes.
		template <typename RegionSet>
		static bool region_set_checker(const RegionSet &busy_regions)
		{
			if (busy_regions.empty()) {
				return true;
			}
			const auto it_f = busy_regions.end();
			auto it = busy_regions.begin();
			auto prev_it(it);
			++it;
			for (; it != it_f; ++it, ++prev_it) {
				if (it->first > it->second || prev_it->first > prev_it->second) {
					return false;
				}
				if (prev_it->second >= it->first) {
					return false;
				}
			}
			return true;
		}
		// Have to place this here because if created as a lambda, it will result in a
		// compiler error in GCC 4.5. In GCC 4.6 there is no such problem.
		// This will sort tasks according to the initial writing position in the hash table
		// of the result.
		struct sparse_task_sorter
		{
			explicit sparse_task_sorter(const return_type &retval,const std::vector<term_type1 const *> &v1,
				const std::vector<term_type2 const *> &v2):
				m_retval(retval),m_v1(v1),m_v2(v2)
			{}
			bool operator()(const task_type &t1, const task_type &t2) const
			{
				piranha_assert(m_retval.m_container.bucket_count());
				// NOTE: here we are sure there is no overflow as the max size of the hash table is 2 ** (n - 1), hence the highest
				// possible bucket is 2 ** (n - 1) - 1, so the highest value of the sum will be 2 ** n - 2. But the range of the size type
				// is [0, 2 ** n - 1], so it is safe.
				// NOTE: might use faster mod calculation here, but probably not worth it.
				// NOTE: because tasks cannot contain empty intervals, the start of each interval will be a valid index (i.e., not end())
				// in the term pointers vectors.
				piranha_assert(t1.m_b1.first < m_v1.size() && t2.m_b1.first < m_v1.size() &&
					t1.m_b2.first < m_v2.size() && t2.m_b2.first < m_v2.size() &&
					t1.m_b1.first < t1.m_b1.second && t1.m_b2.first < t1.m_b2.second &&
					t2.m_b1.first < t2.m_b1.second && t2.m_b2.first < t2.m_b2.second
				);
				return (m_retval.m_container._bucket_from_hash(m_v1[t1.m_b1.first]->hash()) +
					m_retval.m_container._bucket_from_hash(m_v2[t1.m_b2.first]->hash())) %
					m_retval.m_container.bucket_count() <
					(m_retval.m_container._bucket_from_hash(m_v1[t2.m_b1.first]->hash()) +
					m_retval.m_container._bucket_from_hash(m_v2[t2.m_b2.first]->hash())) %
					m_retval.m_container.bucket_count();
			}
			const return_type			&m_retval;
			const std::vector<term_type1 const *>	&m_v1;
			const std::vector<term_type2 const *>	&m_v2;
		};
		// Pre-sort individual blocks if truncator is skipping and active.
		template <typename S1, typename S2>
		void sparse_sort_blocks(const index_type &, const index_type &, const truncator<S1,S2> &,
			typename std::enable_if<!truncator_traits<S1,S2>::is_skipping>::type * = piranha_nullptr) const
		{}
		template <typename S1, typename S2>
		void sparse_sort_blocks(const index_type &bsize1, const index_type &bsize2, const truncator<S1,S2> &trunc,
			typename std::enable_if<truncator_traits<S1,S2>::is_skipping>::type * = piranha_nullptr) const
		{
			if (!trunc.is_active()) {
				return;
			}
			auto &v1 = this->m_v1;
			auto &v2 = this->m_v2;
			const auto size1 = v1.size(), size2 = v2.size();
			auto sorter1 = [&trunc](const term_type1 *t1, const term_type1 *t2) {return trunc.compare_terms(*t1,*t2);};
			auto sorter2 = [&trunc](const term_type2 *t1, const term_type2 *t2) {return trunc.compare_terms(*t1,*t2);};
			for (index_type i = 0u; i < size1 / bsize1; ++i) {
				std::sort(&v1[0u] + i * bsize1,&v1[0u] + (i + 1u) * bsize1,sorter1);
			}
			if (size1 % bsize1) {
				std::sort(&v1[0u] + (size1 / bsize1) * bsize1,&v1[0u] + size1,sorter1);
			}
			for (index_type i = 0u; i < size2 / bsize2; ++i) {
				std::sort(&v2[0u] + i * bsize2,&v2[0u] + (i + 1u) * bsize2,sorter2);
			}
			if (size2 % bsize2) {
				std::sort(&v2[0u] + (size2 / bsize2) * bsize2,&v2[0u] + size2,sorter2);
			}
		}
		template <typename S1, typename S2, typename Keys1, typename Keys2>
		static void dense_sort_blocks(const index_type &, const index_type &, const truncator<S1,S2> &,
			Keys1 &, Keys2 &, typename std::enable_if<!truncator_traits<S1,S2>::is_skipping>::type * = piranha_nullptr)
		{}
		template <typename S1, typename S2, typename Keys1, typename Keys2>
		static void dense_sort_blocks(const index_type &bsize1, const index_type &bsize2, const truncator<S1,S2> &trunc,
			Keys1 &new_keys1, Keys2 &new_keys2, typename std::enable_if<truncator_traits<S1,S2>::is_skipping>::type * = piranha_nullptr)
		{
			if (!trunc.is_active()) {
				return;
			}
			typedef typename Keys1::value_type pair_type1;
			typedef typename Keys2::value_type pair_type2;
			const auto size1 = new_keys1.size(), size2 = new_keys2.size();
			auto sorter1 = [&trunc](const pair_type1 &p1, const pair_type1 &p2) {return trunc.compare_terms(*p1.second,*p2.second);};
			auto sorter2 = [&trunc](const pair_type2 &p1, const pair_type2 &p2) {return trunc.compare_terms(*p1.second,*p2.second);};
			for (index_type i = 0u; i < size1 / bsize1; ++i) {
				std::sort(&new_keys1[0u] + i * bsize1,&new_keys1[0u] + (i + 1u) * bsize1,sorter1);
			}
			if (size1 % bsize1) {
				std::sort(&new_keys1[0u] + (size1 / bsize1) * bsize1,&new_keys1[0u] + size1,sorter1);
			}
			for (index_type i = 0u; i < size2 / bsize2; ++i) {
				std::sort(&new_keys2[0u] + i * bsize2,&new_keys2[0u] + (i + 1u) * bsize2,sorter2);
			}
			if (size2 % bsize2) {
				std::sort(&new_keys2[0u] + (size2 / bsize2) * bsize2,&new_keys2[0u] + size2,sorter2);
			}
		}
		return_type execute(const truncator_type &trunc) const
		{
			const index_type size1 = this->m_v1.size(), size2 = boost::numeric_cast<index_type>(this->m_v2.size());
			// Do not do anything if one of the two series is empty, just return an empty series.
			if (unlikely(!size1 || !size2)) {
				return return_type{};
			}
			// This check is done here to avoid controlling the number of elements of the output series
			// at every iteration of the functor.
			const auto max_size = integer(size1) * size2;
			if (unlikely(max_size > boost::integer_traits<bucket_size_type>::const_max)) {
				piranha_throw(std::overflow_error,"possible overflow in series size");
			}
			// First, let's get the estimation on the size of the final series.
			return_type retval;
			retval.m_symbol_set = this->m_s1->m_symbol_set;
			typename Series1::size_type estimate;
			// Use the sparse functor for the estimation.
			if (trunc.is_active()) {
				estimate = base::estimate_final_series_size(sparse_functor<true>(&this->m_v1[0u],size1,&this->m_v2[0u],size2,trunc,retval));
			} else {
				estimate = base::estimate_final_series_size(sparse_functor<false>(&this->m_v1[0u],size1,&this->m_v2[0u],size2,trunc,retval));
			}
			// Correct the unlikely case of zero estimate.
			if (unlikely(!estimate)) {
				estimate = 1u;
			}
			// Rehash the retun value's container accordingly.
			// NOTE: if something goes wrong here, no big deal as retval is still empty.
			retval.m_container.rehash(boost::numeric_cast<typename Series1::size_type>(std::ceil(estimate / retval.m_container.max_load_factor())));
			piranha_assert(retval.m_container.bucket_count());
			if ((integer(size1) * integer(size2)) / estimate > 200) {
				if (trunc.is_active()) {
					dense_multiplication<true>(retval,trunc);
				} else {
					dense_multiplication<false>(retval,trunc);
				}
			} else {
				if (trunc.is_active()) {
					sparse_multiplication<sparse_functor<true>>(retval,trunc);
				} else {
					sparse_multiplication<sparse_functor<false>>(retval,trunc);
				}
			}
			// Trace the result of estimation.
			this->trace_estimates(retval.size(),estimate);
			return retval;
		}
		// Utility function to determine block sizes.
		static std::pair<integer,integer> get_block_sizes(const index_type &size1, const index_type &size2)
		{
			const integer block_size(256u), job_size = block_size.pow(2u);
			// Rescale the block sizes according to the relative sizes of the input series.
			auto block_size1 = (block_size * size1) / size2,
				block_size2 = (block_size * size2) / size1;
			// Avoid having zero block sizes, or block sizes exceeding job_size.
			if (!block_size1) {
				block_size1 = 1u;
			} else if (block_size1 > job_size) {
				block_size1 = job_size;
			}
			if (!block_size2) {
				block_size2 = 1u;
			} else if (block_size2 > job_size) {
				block_size2 = job_size;
			}
			return std::make_pair(std::move(block_size1),std::move(block_size2));
		}
		template <bool IsActive,typename S1, typename S2>
		static bool dense_skip(const term_type1 &t1,const term_type2 &t2, const truncator<S1,S2> &trunc,
			typename std::enable_if<truncator_traits<S1,S2>::is_skipping>::type * = piranha_nullptr)
		{
			return IsActive && trunc.skip(t1,t2);
		}
		template <bool IsActive,typename S1, typename S2>
		static bool dense_skip(const term_type1 &,const term_type2 &, const truncator<S1,S2> &,
			typename std::enable_if<!truncator_traits<S1,S2>::is_skipping>::type * = piranha_nullptr)
		{
			return false;
		}
		// Compare regions by lower bound.
		struct region_comparer
		{
			bool operator()(const region_type &r1, const region_type &r2) const
			{
				return r1.first < r2.first;
			}
		};
		// Function to remove the regions associated to a task from the set of busy regions.
		template <typename RegionSet>
		static void cleanup_regions(RegionSet &busy_regions, const task_type &task)
		{
			auto tmp_it = busy_regions.find(task.m_r1);
			if (tmp_it != busy_regions.end()) {
				busy_regions.erase(tmp_it);
			}
			// Deal with second region only if present.
			if (!task.m_second_region) {
				return;
			}
			tmp_it = busy_regions.find(task.m_r2);
			if (tmp_it != busy_regions.end()) {
				busy_regions.erase(tmp_it);
			}
			piranha_assert(region_set_checker(busy_regions));
		}
		// Dense multiplication method.
		template <bool IsActive>
		void dense_multiplication(return_type &retval, const truncator_type &trunc) const
		{
			piranha_assert(IsActive == trunc.is_active());
			typedef typename term_type1::key_type::value_type value_type;
			// Vectors of minimum / maximum values, cast to hardware int.
			std::vector<value_type> mins;
			std::transform(m_minmax_values.begin(),m_minmax_values.end(),std::back_inserter(mins),[](const std::pair<integer,integer> &p) {
				return static_cast<value_type>(p.first);
			});
			std::vector<value_type> maxs;
			std::transform(m_minmax_values.begin(),m_minmax_values.end(),std::back_inserter(maxs),[](const std::pair<integer,integer> &p) {
				return static_cast<value_type>(p.second);
			});
			// Build the encoding vector.
			std::vector<value_type> c_vec;
			integer f_delta(1);
			std::transform(m_minmax_values.begin(),m_minmax_values.end(),std::back_inserter(c_vec),
				[&f_delta](const std::pair<integer,integer> &p) -> value_type {
					auto old(f_delta);
					f_delta *= p.second - p.first + 1;
					return static_cast<value_type>(old);
			});
			// Try casting final delta.
			static_cast<value_type>(f_delta);
			// Compute hmax and hmin.
			piranha_assert(m_minmax_values.size() == c_vec.size());
			const auto h_minmax = std::inner_product(m_minmax_values.begin(),m_minmax_values.end(),c_vec.begin(),
				std::make_pair(integer(0),integer(0)),
				[](const std::pair<integer,integer> &p1, const std::pair<integer,integer> &p2) {
					return std::make_pair(p1.first + p2.first,p1.second + p2.second);
				},[](const std::pair<integer,integer> &p, const value_type &value) {
					return std::make_pair(p.first * value,p.second * value);
				}
			);
			piranha_assert(f_delta == h_minmax.second - h_minmax.first + 1);
			// Try casting hmax and hmin.
			const auto hmin = static_cast<value_type>(h_minmax.first);
			const auto hmax = static_cast<value_type>(h_minmax.second);
			// Encoding functor.
			typedef typename term_type1::key_type::v_type unpack_type;
			auto encoder = [&c_vec,hmin,&mins](const unpack_type &v) -> value_type {
				piranha_assert(c_vec.size() == v.size());
				piranha_assert(c_vec.size() == mins.size());
				decltype(mins.size()) i = 0u;
				value_type retval = std::inner_product(c_vec.begin(),c_vec.end(),v.begin(),value_type(0),
					std::plus<value_type>(),[&i,&mins](const value_type &c, const value_type &n) -> value_type {
						const auto old_i(i);
						++i;
						piranha_assert(n >= mins[old_i]);
						return c * (n - mins[old_i]);
					}
				);
				return retval + hmin;
			};
			// Build copies of the input keys repacked according to the new Kronecker substitution. Attach
			// also a pointer to the term.
			typedef std::pair<value_type,term_type1 const *> new_key_type1;
			typedef std::pair<value_type,term_type2 const *> new_key_type2;
			std::vector<new_key_type1> new_keys1;
			std::vector<new_key_type2> new_keys2;
			std::transform(this->m_v1.begin(),this->m_v1.end(),std::back_inserter(new_keys1),
				[this,encoder](term_type1 const *ptr) {
				return std::make_pair(encoder(ptr->m_key.unpack(this->m_s1->m_symbol_set)),ptr);
			});
			std::transform(this->m_v2.begin(),this->m_v2.end(),std::back_inserter(new_keys2),
				[this,encoder](term_type2 const *ptr) {
				return std::make_pair(encoder(ptr->m_key.unpack(this->m_s1->m_symbol_set)),ptr);
			});
			// Sort the the new keys.
			std::sort(new_keys1.begin(),new_keys1.end(),[](const new_key_type1 &p1, const new_key_type1 &p2) {
				return p1.first < p2.first;
			});
			std::sort(new_keys2.begin(),new_keys2.end(),[](const new_key_type2 &p1, const new_key_type2 &p2) {
				return p1.first < p2.first;
			});
			// Store the sizes and compute the block sizes.
			const index_type size1 = boost::numeric_cast<index_type>(new_keys1.size()),
				size2 = boost::numeric_cast<index_type>(new_keys2.size());
			piranha_assert(size1 == this->m_s1->size());
			piranha_assert(size2 == this->m_s2->size());
			const auto bsizes = get_block_sizes(size1,size2);
			// Cast to hardware integers.
			const auto bsize1 = static_cast<index_type>(bsizes.first), bsize2 = static_cast<index_type>(bsizes.second);
			// Build the list of tasks.
			auto dense_task_sorter = [&new_keys1,&new_keys2](const task_type &t1, const task_type &t2) {
				return new_keys1[t1.m_b1.first].first + new_keys2[t1.m_b2.first].first <
					new_keys1[t2.m_b1.first].first + new_keys2[t2.m_b2.first].first;
			};
			std::multiset<task_type,decltype(dense_task_sorter)> task_list(dense_task_sorter);
			decltype(task_list.insert(std::declval<task_type>())) ins_result;
			auto dense_task_from_indices = [hmin,hmax,&new_keys1,&new_keys2](const index_type &i_start, const index_type &i_end,
				const index_type &j_start, const index_type &j_end) -> task_type
			{
				piranha_assert(i_end <= new_keys1.size() && j_end <= new_keys2.size());
				piranha_assert(i_start < i_end && j_start < j_end);
				const bucket_size_type a = boost::numeric_cast<bucket_size_type>((new_keys1[i_start].first + new_keys2[j_start].first) - hmin);
				const bucket_size_type b = boost::numeric_cast<bucket_size_type>((new_keys1[i_end - 1u].first + new_keys2[j_end - 1u].first) - hmin);
				piranha_assert(a <= b);
				piranha_assert(b <= boost::numeric_cast<bucket_size_type>(hmax - hmin));
				return task_type{{i_start,i_end},{j_start,j_end},{a,b},{0u,0u},false};
			};
			for (index_type i = 0u; i < size1 / bsize1; ++i) {
				for (index_type j = 0u; j < size2 / bsize2; ++j) {
					ins_result = task_list.insert(dense_task_from_indices(i * bsize1,(i + 1u) * bsize1,j * bsize2,(j + 1u) * bsize2));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
				if (size2 % bsize2) {
					ins_result = task_list.insert(dense_task_from_indices(i * bsize1,(i + 1u) * bsize1,(size2 / bsize2) * bsize2,size2));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
			}
			if (size1 % bsize1) {
				for (index_type j = 0u; j < size2 / bsize2; ++j) {
					ins_result = task_list.insert(dense_task_from_indices((size1 / bsize1) * bsize1,size1,j * bsize2,(j + 1u) * bsize2));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
				if (size2 % bsize2) {
					ins_result = task_list.insert(dense_task_from_indices((size1 / bsize1) * bsize1,size1,(size2 / bsize2) * bsize2,size2));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
			}
			// Pre-sort each block according to the truncator, if necessary.
			dense_sort_blocks(bsize1,bsize2,trunc,new_keys1,new_keys2);
			// Prepare the storage for multiplication.
			std::vector<typename term_type1::cf_type> cf_vector;
			cf_vector.resize(boost::numeric_cast<decltype(cf_vector.size())>((hmax - hmin) + 1));
			// Get the number of threads.
			typedef decltype(this->determine_n_threads()) thread_size_type;
			const thread_size_type n_threads = this->determine_n_threads();
			if (n_threads == 1u) {
				// Single-thread multiplication.
				const auto it_f = task_list.end();
				for (auto it = task_list.begin(); it != it_f; ++it) {
					const index_type i_start = it->m_b1.first, j_start = it->m_b2.first,
						i_end = it->m_b1.second, j_end = it->m_b2.second;
					piranha_assert(i_end > i_start && j_end > j_start);
					for (index_type i = i_start; i < i_end; ++i) {
						for (index_type j = j_start; j < j_end; ++j) {
							if (dense_skip<IsActive>(*new_keys1[i].second,*new_keys2[j].second,trunc)) {
								break;
							}
							const auto idx = (new_keys1[i].first + new_keys2[j].first) - hmin;
							piranha_assert(idx < boost::numeric_cast<value_type>(cf_vector.size()));
							math::multiply_accumulate(cf_vector[static_cast<decltype(cf_vector.size())>(idx)],
								new_keys1[i].second->m_cf,new_keys2[j].second->m_cf);
						}
					}
				}
			} else {
				// Set of busy bucket regions, ordered by starting point.
				// NOTE: we do not need a multiset, as regions that compare equal (i.e., same starting point) will not exist
				// at the same time in the set by design.
				region_comparer rc;
				std::set<region_type,region_comparer> busy_regions(rc);
				// Synchronization.
				mutex m;
				condition_variable cond;
				// Thread function.
				auto thread_function = [&trunc,&cond,&m,&task_list,&busy_regions,&new_keys1,&new_keys2,hmin,&cf_vector,this] () {
					task_type task;
					while (true) {
						{
							// First, lock down everything.
							unique_lock<mutex>::type lock(m);
							if (task_list.empty()) {
								break;
							}
							// Look for a suitable task.
							const auto it_f = task_list.end();
							auto it = task_list.begin();
							for (; it != it_f; ++it) {
								piranha_assert(!it->m_second_region);
								// Check the region.
								if (this->region_checker(it->m_r1,busy_regions)) {
									try {
										// The region is ok, insert it and break the cycle.
										auto tmp = busy_regions.insert(it->m_r1);
										(void)tmp;
										piranha_assert(tmp.second);
										piranha_assert(this->region_set_checker(busy_regions));
									} catch (...) {
										// NOTE: the idea here is that in case of errors
										// we want to restore the original situation
										// as if nothing happened.
										this->cleanup_regions(busy_regions,*it);
										throw;
									}
									break;
								}
							}
							// We might have identified a suitable task, check it is not end().
							if (it == it_f) {
								// The thread can't do anything, will have to wait until something happens
								// and then re-identify a good task.
								cond.wait(lock);
								continue;
							}
							// Now we have a good task, pop it from the task list.
							task = *it;
							task_list.erase(it);
						}
						try {
							// Perform the multiplication on the selected task.
							const index_type i_start = task.m_b1.first, j_start = task.m_b2.first,
								i_end = task.m_b1.second, j_end = task.m_b2.second;
							piranha_assert(i_end > i_start && j_end > j_start);
							for (index_type i = i_start; i < i_end; ++i) {
								for (index_type j = j_start; j < j_end; ++j) {
									if (this->dense_skip<IsActive>(*new_keys1[i].second,*new_keys2[j].second,trunc)) {
										break;
									}
									const auto idx = (new_keys1[i].first + new_keys2[j].first) - hmin;
									piranha_assert(idx < boost::numeric_cast<value_type>(cf_vector.size()));
									math::multiply_accumulate(cf_vector[static_cast<decltype(cf_vector.size())>(idx)],
										new_keys1[i].second->m_cf,new_keys2[j].second->m_cf);
								}
							}
						} catch (...) {
							// Re-acquire the lock.
							lock_guard<mutex>::type lock(m);
							// Cleanup the regions.
							this->cleanup_regions(busy_regions,task);
							// Notify all waiting threads that a region was removed from the busy set.
							cond.notify_all();
							throw;
						}
						{
							// Re-acquire the lock.
							lock_guard<mutex>::type lock(m);
							// Take out the regions in which we just wrote from the set of busy regions.
							this->cleanup_regions(busy_regions,task);
							// Notify all waiting threads that a region was removed from the busy set.
							cond.notify_all();
						}
					}
				};
				// NOTE: we need to wrap the task and the future in shared pointers because
				// of a GCC 4.6 bug (will try to copy move-only objects because of internal use
				// of std::bind).
				typedef std::tuple<std::shared_ptr<packaged_task<void()>::type>,std::shared_ptr<future<void>::type>> tuple_type;
				std::list<tuple_type,cache_aligning_allocator<tuple_type>> pf_list;
				try {
					for (thread_size_type i = 0u; i < n_threads; ++i) {
						try {
							// First let's try to append an empty item.
							pf_list.push_back(tuple_type{});
							// Second, create the real tuple.
							tuple_type t;
							std::get<0u>(t).reset(new packaged_task<void()>::type(thread_function));
							std::get<1u>(t).reset(new future<void>::type(std::get<0u>(t)->get_future()));
							auto pt_ptr = std::get<0u>(t);
							// Try launching the thread.
							thread thr(pt_launcher_dense,pt_ptr);
							piranha_assert(thr.joinable());
							try {
								thr.detach();
							} catch (...) {
								// Last ditch effort: try joining before re-throwing.
								thr.join();
								throw;
							}
							// If everything went ok, move in the real tuple.
							pf_list.back() = std::move(t);
						} catch (...) {
							// If the error happened *after* the empty tuple was added,
							// then we have to remove it as it is invalid.
							if (pf_list.size() != i) {
								auto it_f = pf_list.end();
								--it_f;
								pf_list.erase(it_f);
							}
							throw;
						}
					}
					piranha_assert(pf_list.size() == n_threads);
					// First let's wait for everything to finish.
					waiter(pf_list);
					// Then, let's handle the exceptions.
					for (auto it = pf_list.begin(); it != pf_list.end(); ++it) {
						std::get<1u>(*it)->get();
					}
				} catch (...) {
					// Make sure any pending task is finished -> this is for
					// the case the exception was thrown in the thread creation
					// loop.
					waiter(pf_list);
					throw;
				}
			}
			// Build the return value.
			// NOTE: the thing here is that if the truncator were not skipping, we would need to filter out
			// the terms when building the return value. This is here as a reminder if we use this code,
			// e.g., in Poisson series multiplication where the truncator could be filtering but not skipping.
			static_assert(truncator_traits<Series1,Series2>::is_skipping,"Skipping truncator expected.");
			// Append the final delta to the coding vector for use in the decoding routine.
			c_vec.push_back(static_cast<value_type>(f_delta));
			// Temp vector for decoding.
			std::vector<value_type> tmp_v;
			tmp_v.resize(boost::numeric_cast<decltype(tmp_v.size())>(this->m_s1->m_symbol_set.size()));
			piranha_assert(c_vec.size() - 1u == tmp_v.size());
			piranha_assert(mins.size() == tmp_v.size());
			auto decoder = [&tmp_v,&c_vec,&mins,&maxs](const value_type &n) {
				decltype(c_vec.size()) i = 0u;
				std::generate(tmp_v.begin(),tmp_v.end(),
					[&n,&i,&mins,&maxs,&c_vec]() -> value_type
				{
					auto retval = (n % c_vec[i + 1u]) / c_vec[i] + mins[i];
					piranha_assert(retval >= mins[i] && retval <= maxs[i]);
					++i;
					return retval;
				});
			};
			const auto cf_size = cf_vector.size();
			term_type1 tmp_term;
			for (decltype(cf_vector.size()) i = 0u; i < cf_size; ++i) {
				if (!math::is_zero(cf_vector[i])) {
					tmp_term.m_cf = std::move(cf_vector[i]);
					decoder(boost::numeric_cast<value_type>(i));
					tmp_term.m_key = decltype(tmp_term.m_key)(tmp_v.begin(),tmp_v.end());
					retval.insert(std::move(tmp_term));
				}
			}
		}
		template <typename Functor>
		void sparse_multiplication(return_type &retval, const truncator_type &trunc) const
		{
			const index_type size1 = this->m_v1.size(), size2 = boost::numeric_cast<index_type>(this->m_v2.size());
			// Sort the input terms according to the position of the Kronecker keys in the estimated return value.
			auto sorter1 = [&retval](term_type1 const *ptr1, term_type1 const *ptr2) {
				return retval.m_container._bucket_from_hash(ptr1->hash()) < retval.m_container._bucket_from_hash(ptr2->hash());
			};
			auto sorter2 = [&retval](term_type2 const *ptr1, term_type2 const *ptr2) {
				return retval.m_container._bucket_from_hash(ptr1->hash()) < retval.m_container._bucket_from_hash(ptr2->hash());
			};
			std::sort(this->m_v1.begin(),this->m_v1.end(),sorter1);
			std::sort(this->m_v2.begin(),this->m_v2.end(),sorter2);
			// Start defining the blocks for series multiplication.
			const auto bsizes = get_block_sizes(size1,size2);
			// Cast to hardware integers.
			const auto bsize1 = static_cast<index_type>(bsizes.first), bsize2 = static_cast<index_type>(bsizes.second);
			// Create the list of tasks.
			// NOTE: the way tasks are created, there is never an empty task - all intervals have nonzero sizes.
			// The task are sorted according to the index of the first bucket of retval that will be written to,
			// so we need a multiset as different tasks might have the same starting position.
			std::multiset<task_type,sparse_task_sorter> task_list(sparse_task_sorter(retval,this->m_v1,this->m_v2));
			decltype(task_list.insert(std::declval<task_type>())) ins_result;
			for (index_type i = 0u; i < size1 / bsize1; ++i) {
				for (index_type j = 0u; j < size2 / bsize2; ++j) {
					ins_result = task_list.insert(task_from_indices(i * bsize1,(i + 1u) * bsize1,j * bsize2,(j + 1u) * bsize2,retval));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
				if (size2 % bsize2) {
					ins_result = task_list.insert(task_from_indices(i * bsize1,(i + 1u) * bsize1,(size2 / bsize2) * bsize2,size2,retval));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
			}
			if (size1 % bsize1) {
				for (index_type j = 0u; j < size2 / bsize2; ++j) {
					ins_result = task_list.insert(task_from_indices((size1 / bsize1) * bsize1,size1,j * bsize2,(j + 1u) * bsize2,retval));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
				if (size2 % bsize2) {
					ins_result = task_list.insert(task_from_indices((size1 / bsize1) * bsize1,size1,(size2 / bsize2) * bsize2,size2,retval));
					piranha_assert(ins_result->m_b1.first != ins_result->m_b1.second && ins_result->m_b2.first != ins_result->m_b2.second);
				}
			}
			// Pre-sort each block according to the truncator, if necessary.
			sparse_sort_blocks(bsize1,bsize2,trunc);
			typedef decltype(this->determine_n_threads()) thread_size_type;
			const thread_size_type n_threads = this->determine_n_threads();
			if (n_threads == 1u) {
				// Perform the multiplication. We need this try/catch because, by using the fast interface,
				// in case of an error the container in retval could be left in an inconsistent state.
				try {
					sparse_single_thread<Functor>(retval,trunc,task_list);
				} catch (...) {
					retval.m_container.clear();
					throw;
				}
			} else {
				// Insertion counter.
				bucket_size_type insertion_count = 0u;
				// Set of busy bucket regions, ordered by starting point.
				// NOTE: we do not need a multiset, as regions that compare equal (i.e., same starting point) will not exist
				// at the same time in the set by design.
				region_comparer rc;
				std::set<region_type,region_comparer> busy_regions(rc);
				// Synchronization.
				mutex m;
				condition_variable cond;
				typedef std::set<bucket_size_type> b_set_type;
				// Thread function.
				auto thread_function = [&trunc,&cond,&m,&insertion_count,&task_list,&busy_regions,&retval,this]() -> b_set_type {
					b_set_type b_check;
					task_type task;
					while (true) {
						{
							// First, lock down everything.
							unique_lock<mutex>::type lock(m);
							if (task_list.empty()) {
								break;
							}
							// Look for a suitable task.
							const auto it_f = task_list.end();
							auto it = task_list.begin();
							for (; it != it_f; ++it) {
								// Check the regions.
								if (this->region_checker(it->m_r1,busy_regions) &&
									(!it->m_second_region || this->region_checker(it->m_r2,busy_regions)))
								{
									try {
										// The regions are ok, insert them and break the cycle.
										auto tmp = busy_regions.insert(it->m_r1);
										piranha_assert(tmp.second);
										if (it->m_second_region) {
											tmp = busy_regions.insert(it->m_r2);
											piranha_assert(tmp.second);
										}
										piranha_assert(this->region_set_checker(busy_regions));
									} catch (...) {
										// NOTE: the idea here is that in case of errors
										// we want to restore the original situation
										// as if nothing happened.
										this->cleanup_regions(busy_regions,*it);
										throw;
									}
									break;
								}
							}
							// We might have identified a suitable task, check it is not end().
							if (it == it_f) {
								// The thread can't do anything, will have to wait until something happens
								// and then re-identify a good task.
								cond.wait(lock);
								continue;
							}
							// Now we have a good task, pop it from the task list.
							task = *it;
							task_list.erase(it);
						}
						bucket_size_type tmp_ins_count = 0u;
						try {
							// Perform the multiplication on the selected task.
							typedef typename Functor::fast_rebind fast_functor_type;
							const index_type i_start = task.m_b1.first, j_start = task.m_b2.first,
								i_end = task.m_b1.second, j_end = task.m_b2.second;
							piranha_assert(i_end > i_start && j_end > j_start);
							const index_type i_size = i_end - i_start, j_size = j_end - j_start;
							fast_functor_type f(&this->m_v1[0u] + i_start,i_size,&this->m_v2[0u] + j_start,j_size,trunc,retval);
							for (index_type i = 0u; i < i_size; ++i) {
								for (index_type j = 0u; j < j_size; ++j) {
									if (f.skip(i,j)) {
										break;
									}
									f(i,j);
									f.insert();
								}
							}
							tmp_ins_count = f.m_insertion_count;
							b_check.insert(f.m_bucket_check.begin(),f.m_bucket_check.end());
						} catch (...) {
							// Re-acquire the lock.
							lock_guard<mutex>::type lock(m);
							// Cleanup the regions.
							this->cleanup_regions(busy_regions,task);
							// Notify all waiting threads that a region was removed from the busy set.
							cond.notify_all();
							throw;
						}
						{
							// Re-acquire the lock.
							lock_guard<mutex>::type lock(m);
							// Update the insertion count.
							insertion_count += tmp_ins_count;
							// Take out the regions in which we just wrote from the set of busy regions.
							this->cleanup_regions(busy_regions,task);
							// Notify all waiting threads that a region was removed from the busy set.
							cond.notify_all();
						}
					}
					return b_check;
				};
				// NOTE: we need to wrap the task and the future in shared pointers because
				// of a GCC 4.6 bug (will try to copy move-only objects because of internal use
				// of std::bind).
				typedef std::tuple<std::shared_ptr<typename packaged_task<b_set_type ()>::type>,std::shared_ptr<typename future<b_set_type>::type>> tuple_type;
				std::list<tuple_type,cache_aligning_allocator<tuple_type>> pf_list;
				b_set_type b_check_list;
				try {
					for (thread_size_type i = 0u; i < n_threads; ++i) {
						try {
							// First let's try to append an empty item.
							pf_list.push_back(tuple_type{});
							// Second, create the real tuple.
							tuple_type t;
							std::get<0u>(t).reset(new typename packaged_task<b_set_type ()>::type(thread_function));
							std::get<1u>(t).reset(new typename future<b_set_type>::type(std::get<0u>(t)->get_future()));
							auto pt_ptr = std::get<0u>(t);
							// Try launching the thread.
							thread thr(pt_launcher_sparse,pt_ptr);
							piranha_assert(thr.joinable());
							try {
								thr.detach();
							} catch (...) {
								// Last ditch effort: try joining before re-throwing.
								thr.join();
								throw;
							}
							// If everything went ok, move in the real tuple.
							pf_list.back() = std::move(t);
						} catch (...) {
							// If the error happened *after* the empty tuple was added,
							// then we have to remove it as it is invalid.
							if (pf_list.size() != i) {
								auto it_f = pf_list.end();
								--it_f;
								pf_list.erase(it_f);
							}
							throw;
						}
					}
					piranha_assert(pf_list.size() == n_threads);
					// First let's wait for everything to finish.
					waiter(pf_list);
					// Then, let's handle the exceptions and retvals.
					for (auto it = pf_list.begin(); it != pf_list.end(); ++it) {
						auto tmp_b_set = std::get<1u>(*it)->get();
						b_check_list.insert(tmp_b_set.begin(),tmp_b_set.end());
					}
					// Finally, fix the series.
					sanitize_series(retval,insertion_count,b_check_list);
				} catch (...) {
					// Make sure any pending task is finished -> this is for
					// the case the exception was thrown in the thread creation
					// loop.
					waiter(pf_list);
					// Clean up and re-throw.
					retval.m_container.clear();
					throw;
				}
			}
		}
		// NOTE: these two static functions are for internal use above, we have to place them
		// here because of GCC bugs (in the first case it is the same std::bind problem above,
		// in the second case a problem cropping up when using LTO).
		// Helper function for use above.
		static void pt_launcher_dense(std::shared_ptr<packaged_task<void()>::type> pt_ptr)
		{
			(*pt_ptr)();
		}
		static void pt_launcher_sparse(std::shared_ptr<typename packaged_task<std::set<bucket_size_type> ()>::type> pt_ptr)
		{
			(*pt_ptr)();
		}
		// Functor to wait for completion of all threads.
		template <typename PfList>
		static void waiter(PfList &pf_list)
		{
			std::for_each(pf_list.begin(),pf_list.end(),[](typename PfList::value_type &t) {std::get<1u>(t)->wait();});
		}
		// Single thread function.
		template <typename Functor, typename Truncator, typename TaskList>
		void sparse_single_thread(return_type &retval, const Truncator &trunc, const TaskList &task_list) const
		{
			typedef typename Functor::fast_rebind fast_functor_type;
			std::set<bucket_size_type> b_check;
			bucket_size_type insertion_count = 0u;
			const auto it_f = task_list.end();
			for (auto it = task_list.begin(); it != it_f; ++it) {
				const index_type i_start = it->m_b1.first, j_start = it->m_b2.first,
					i_end = it->m_b1.second, j_end = it->m_b2.second;
				piranha_assert(i_end > i_start && j_end > j_start);
				const index_type i_size = i_end - i_start, j_size = j_end - j_start;
				fast_functor_type f(&this->m_v1[0u] + i_start,i_size,&this->m_v2[0u] + j_start,j_size,trunc,retval);
				for (index_type i = 0u; i < i_size; ++i) {
					for (index_type j = 0u; j < j_size; ++j) {
						if (f.skip(i,j)) {
							break;
						}
						f(i,j);
						f.insert();
					}
				}
				insertion_count += f.m_insertion_count;
				b_check.insert(f.m_bucket_check.begin(),f.m_bucket_check.end());
			}
			sanitize_series(retval,insertion_count,b_check);
		}
		// Sanitize series after completion of sparse multi-thread multiplication.
		static void sanitize_series(return_type &retval, const bucket_size_type &insertion_count, const std::set<bucket_size_type> &b_check)
		{
			// Here we have to do the following things:
			// - check ignorability of terms,
			// - cope with excessive load factor,
			// - update the size of the series.
			// Compatibility is not a concern for polynomials.
			// First, let's fix the size of inserted terms.
			retval.m_container._update_size(insertion_count);
			// Second, check flagged buckets.
			// TODO improve performance here by really checking only the buckets.
			if (!b_check.empty()) {
				const auto it_f = retval.m_container.end();
				for (auto it = retval.m_container.begin(); it != it_f;) {
					if (unlikely(it->is_ignorable(retval.m_symbol_set))) {
						it = retval.m_container.erase(it);
					} else {
						++it;
					}
				}
			}
			// Finally, cope with excessive load factor.
			if (unlikely(retval.m_container.load_factor() > retval.m_container.max_load_factor())) {
				retval.m_container.rehash(
					boost::numeric_cast<bucket_size_type>(std::ceil(retval.m_container.size() / retval.m_container.max_load_factor()))
				);
			}
		}
		// Functor for use in sparse multiplication.
		template <bool IsActive, bool FastMode = false>
		struct sparse_functor: base::template default_functor<IsActive,false>
		{
			// Fast version of functor.
			typedef sparse_functor<IsActive,true> fast_rebind;
			// NOTE: here the coefficient of m_tmp gets default-inited explicitly by the default constructor of base term.
			explicit sparse_functor(term_type1 const **ptr1, const index_type &s1,
				term_type2 const **ptr2, const index_type &s2, const truncator_type &trunc,
				return_type &retval):
				base::template default_functor<IsActive,false>(ptr1,s1,ptr2,s2,trunc,retval),
				m_cached_i(0u),m_cached_j(0u),m_insertion_count(0u)
			{}
			void operator()(const index_type &i, const index_type &j) const
			{
				piranha_assert(i < this->m_s1 && j < this->m_s2);
				this->m_tmp.m_key.set_int(this->m_ptr1[i]->m_key.get_int() + this->m_ptr2[j]->m_key.get_int());
				m_cached_i = i;
				m_cached_j = j;
			}
			template <bool CheckFilter = true>
			void insert() const
			{
				// NOTE: be very careful: every kind of optimization in here must involve only the key part,
				// as the coefficient part is still generic.
				auto &container = this->m_retval.m_container;
				auto &tmp = this->m_tmp;
				const auto &cf1 = this->m_ptr1[m_cached_i]->m_cf;
				const auto &cf2 = this->m_ptr2[m_cached_j]->m_cf;
				const auto &args = this->m_retval.m_symbol_set;
				// Prepare the return series.
				piranha_assert(!FastMode || container.bucket_count());
				if (!FastMode && unlikely(!container.bucket_count())) {
					container._increase_size();
				}
				// Try to locate the term into retval.
				auto bucket_idx = container._bucket(tmp);
				const auto it = container._find(tmp,bucket_idx);
				if (it == container.end()) {
					// NOTE: the check here is done outside.
					piranha_assert(container.size() < boost::integer_traits<bucket_size_type>::const_max);
					// Term is new. Handle the case in which we need to rehash because of load factor.
					if (!FastMode && unlikely(static_cast<double>(container.size() + bucket_size_type(1u)) / container.bucket_count() >
						container.max_load_factor()))
					{
						container._increase_size();
						// We need a new bucket index in case of a rehash.
						bucket_idx = container._bucket(tmp);
					}
					// TODO optimize this in case of series and integer (?), now it is optimized for simple coefficients.
					// Note that the best course of action here for integer multiplication would seem to resize tmp.m_cf appropriately
					// and then use something like mpz_mul. On the other hand, it seems like in the insertion below we need to perform
					// a copy anyway, so insertion with move seems ok after all? Mmmh...
					// TODO: other important thing: for coefficient series, we probably want to insert with move() below,
					// as we are not going to re-use the allocated resources in tmp.m_cf -> in other words, optimize this
					// as much as possible.
					// Take care of multiplying the coefficient.
					tmp.m_cf = cf1;
					tmp.m_cf *= cf2;
					// Insert and update size.
					// NOTE: the counters are protected from overflows by the check done in the operator() of the multiplier.
					const auto not_ignorable = !tmp.is_ignorable(args);
					if (likely(not_ignorable)) {
						container._unique_insert(tmp,bucket_idx);
						if (FastMode) {
							++m_insertion_count;
						} else {
							container._update_size(container.size() + 1u);
						}
					}
				} else {
					// Assert the existing term is not ignorable, in non-fast mode.
					piranha_assert(FastMode || !it->is_ignorable(args));
					if (FastMode) {
						// In fast mode we do not care if this throws,
						// as we will be dealing with that from outside.
						// If the coefficient is null, add the bucket
						// to the check list for later use.
						math::multiply_accumulate(it->m_cf,cf1,cf2);
						const auto ignorable = it->is_ignorable(args);
						if (unlikely(ignorable)) {
							m_bucket_check.insert(bucket_idx);
						}
					} else {
						// Cleanup function.
						auto cleanup = [&it,&args,&container]() -> void {
							if (unlikely(it->is_ignorable(args))) {
								container.erase(it);
							}
						};
						try {
							math::multiply_accumulate(it->m_cf,cf1,cf2);
							// Check if the term has become ignorable or incompatible after the modification.
							cleanup();
						} catch (...) {
							// In case of exceptions, do the check before re-throwing.
							cleanup();
							throw;
						}
					}
				}
			}
			mutable index_type			m_cached_i;
			mutable index_type			m_cached_j;
			mutable bucket_size_type		m_insertion_count;
			mutable std::set<bucket_size_type>	m_bucket_check;
		};
	private:
		// Vector of closed ranges of the exponents in both the operands and the result.
		std::vector<std::pair<integer,integer>> m_minmax_values;
};

}

#endif
