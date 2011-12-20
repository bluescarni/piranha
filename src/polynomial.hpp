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
			const auto &limits = ka::get_limits()[this->m_s1->m_symbol_set.size()];
			// Check that the multiplication will not overflow the bounds of the Kronecker
			// representation.
			const auto min_max_it1 = boost::minmax_element(this->m_v1.begin(),this->m_v1.end(),
				[](term_type1 const *ptr1, term_type1 const *ptr2) {return ptr1->m_key.get_int() < ptr2->m_key.get_int();});
			const auto min_max_it2 = boost::minmax_element(this->m_v2.begin(),this->m_v2.end(),
				[](term_type2 const *ptr1, term_type2 const *ptr2) {return ptr1->m_key.get_int() < ptr2->m_key.get_int();});
			// Bounds of the Kronecker representation for each component.
			const auto &minmax_vec = std::get<0u>(limits);
			// Decode the min-max values from the two series.
			const auto min_vec1 = (*min_max_it1.first)->m_key.unpack(this->m_s1->m_symbol_set), max_vec1 = (*min_max_it1.second)->m_key.unpack(this->m_s1->m_symbol_set),
				min_vec2 = (*min_max_it2.first)->m_key.unpack(this->m_s1->m_symbol_set), max_vec2 = (*min_max_it2.second)->m_key.unpack(this->m_s1->m_symbol_set);
			// Determine the ranges of the components of the monomials in retval.
			integer tmp_min(0), tmp_max(0);
			for (decltype(min_vec1.size()) i = 0u; i < min_vec1.size(); ++i) {
				tmp_min = min_vec1[i];
				tmp_min += min_vec2[i];
				tmp_max = max_vec1[i];
				tmp_max += max_vec2[i];
				if (unlikely(tmp_min < -minmax_vec[i] || tmp_max > minmax_vec[i])) {
					piranha_throw(std::overflow_error,"Kronecker monomial components are out of bounds");
				}
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
		typename base::return_type operator()() const
		{
			truncator_type trunc(*this->m_s1,*this->m_s2);
			if (trunc.is_active()) {
				return execute<functor<true>>(trunc);
			} else {
				return execute<functor<false>>(trunc);
			}
		}
	private:
		// Have to place this here because if created as a lambda, it will result in a
		// compiler error in GCC 4.5. In GCC 4.6 there is no such problem.
		struct task_sorter
		{
			explicit task_sorter(const return_type &retval,const std::vector<term_type1 const *> &v1,
				const std::vector<term_type2 const *> &v2):
				m_retval(retval),m_v1(v1),m_v2(v2)
			{}
			template <typename T>
			bool operator()(const T &t1, const T &t2) const
			{
				piranha_assert(m_retval.m_container.bucket_count());
				// NOTE: here we are sure there is no overflow as the max size of the hash table is 2 ** (n - 1), hence the highest
				// possible bucket is 2 ** (n - 1) - 1, so the highest value of the sum will be 2 ** n - 2. But the range of the size type
				// is [0, 2 ** n - 1], so it is safe.
				// NOTE: might use faster mod calculation here, but probably not worth it.
				return (m_retval.m_container._bucket(*m_v1[t1.first.first]) + m_retval.m_container._bucket(*m_v2[t1.second.first])) %
					m_retval.m_container.bucket_count() <
					(m_retval.m_container._bucket(*m_v1[t2.first.first]) + m_retval.m_container._bucket(*m_v2[t2.second.first])) %
					m_retval.m_container.bucket_count();
			}
			const return_type			&m_retval;
			const std::vector<term_type1 const *>	&m_v1;
			const std::vector<term_type2 const *>	&m_v2;
		};
		template <typename Functor>
		typename base::return_type execute(const truncator_type &trunc) const
		{
			typedef decltype(this->m_v1.size()) size_type;
			const size_type size1 = this->m_v1.size(), size2 = this->m_v2.size();
			// Do not do anything if one of the two series is empty.
			if (unlikely(this->m_s1->empty() || this->m_s2->empty())) {
				return return_type{};
			}
			// This check is done here to avoid controlling the number of elements of the output series
			// at every iteration of the functor.
			const auto max_size = integer(size1) * size2;
			if (unlikely(max_size > boost::integer_traits<typename Series1::size_type>::const_max)) {
				piranha_throw(std::overflow_error,"overflow in series size");
			}
			// First, let's get the estimation on the size of the final series.
			return_type retval;
			retval.m_symbol_set = this->m_s1->m_symbol_set;
			auto estimate = base::estimate_final_series_size(Functor(&this->m_v1[0u],size1,&this->m_v2[0u],size2,trunc,retval));
			// Correct the unlikely case of zero estimate.
			if (unlikely(!estimate)) {
				estimate = 1u;
			}
			// Rehash the retun value's container accordingly.
			// NOTE: if something goes wrong here, no big deal as retval is still empty.
			retval.m_container.rehash(boost::numeric_cast<decltype(estimate)>(std::ceil(estimate / retval.m_container.max_load_factor())));
			piranha_assert(retval.m_container.bucket_count() >= 1u);
			// Now let's sort the input terms according to the position of the Kronecker keys in the estimated return value.
			auto sorter1 = [&retval](term_type1 const *ptr1, term_type1 const *ptr2) {
				return retval.m_container._bucket(*ptr1) < retval.m_container._bucket(*ptr2);
			};
			auto sorter2 = [&retval](term_type2 const *ptr1, term_type2 const *ptr2) {
				return retval.m_container._bucket(*ptr1) < retval.m_container._bucket(*ptr2);
			};
			std::sort(this->m_v1.begin(),this->m_v1.end(),sorter1);
			std::sort(this->m_v2.begin(),this->m_v2.end(),sorter2);
			// Start defining the blocks for series multiplication.
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
			// Cast to hardware integers.
			const auto bsize1 = static_cast<size_type>(block_size1), bsize2 = static_cast<size_type>(block_size2);
			// This is a multiplication task: (Block1,Block2), where Block1(2) is a range of this->m_v1(2).
			typedef std::pair<std::pair<size_type,size_type>,std::pair<size_type,size_type>> task_type;
			// Create the list of tasks.
			std::vector<task_type> task_list;
			for (size_type i = 0u; i < size1 / bsize1; ++i) {
				for (size_type j = 0u; j < size2 / bsize2; ++j) {
					task_list.push_back({{i * bsize1,(i + 1u) * bsize1},{j * bsize2,(j + 1u) * bsize2}});
				}
				if (size2 % bsize2) {
					task_list.push_back({{i * bsize1,(i + 1u) * bsize1},{(size2 / bsize2) * bsize2,size2}});
				}
			}
			if (size1 % bsize1) {
				for (size_type j = 0u; j < size2 / bsize2; ++j) {
					task_list.push_back({{(size1 / bsize1) * bsize1,size1},{j * bsize2,(j + 1u) * bsize2}});
				}
				if (size2 % bsize2) {
					task_list.push_back({{(size1 / bsize1) * bsize1,size1},{(size2 / bsize2) * bsize2,size2}});
				}
			}
			// Sort the task list according to where each task would start writing in the output series.
			std::sort(task_list.begin(),task_list.end(),task_sorter(retval,this->m_v1,this->m_v2));
			// Perform the multiplication.
			typedef typename Functor::fast_rebind fast_functor_type;
			fast_functor_type ff(&this->m_v1[0u],size1,&this->m_v2[0u],size2,trunc,retval);
			// We need this try/catch because, by using the fast interface, in case of an error the container in retval could be left in
			// an inconsistent state.
			try {
				task_multiplication(ff,task_list);
			} catch (...) {
				retval.m_container.clear();
				throw;
			}
			// Trace the result of estimation.
			this->trace_estimates(retval.size(),estimate);
			return retval;
		}
		template <typename Functor, typename TaskList>
		void task_multiplication(const Functor &f, const TaskList &task_list) const
		{
			const auto it_f = task_list.end();
			for (auto it = task_list.begin(); it != it_f; ++it) {
				const auto i_end = it->first.second;
				for (auto i = it->first.first; i < i_end; ++i) {
					const auto j_end = it->second.second;
					for (auto j = it->second.first; j < j_end; ++j) {
						if (f.skip(i,j)) {
							break;
						}
						f(i,j);
						f.insert();
					}
				}
			}
			sanitize_series(f);
		}
		template <typename Functor>
		static void sanitize_series(const Functor &func)
		{
			auto &retval = func.m_retval;
			// Here we have to do the following things:
			// - check ignorability of terms,
			// - cope with excessive load factor,
			// - update the size of the series.
			// Compatibility is not a concern for polynomials.
			// First, let's fix the size of inserted terms.
			retval.m_container._update_size(func.m_insertion_count);
			// Second, erase the ignorable terms.
			const auto it_f = retval.m_container.end();
			for (auto it = retval.m_container.begin(); it != it_f;) {
				if (unlikely(it->is_ignorable(retval.m_symbol_set))) {
					it = retval.m_container.erase(it);
				} else {
					++it;
				}
			}
			// Finally, cope with excessive load factor.
			if (unlikely(retval.m_container.load_factor() > retval.m_container.max_load_factor())) {
				retval.m_container.rehash(
					boost::numeric_cast<typename Series1::size_type>(std::ceil(retval.m_container.size() / retval.m_container.max_load_factor()))
				);
			}
		}
		template <bool IsActive, bool FastMode = false>
		struct functor: base::template default_functor<IsActive>
		{
			// Fast version of functor.
			typedef functor<IsActive,true> fast_rebind;
			typedef typename base::template default_functor<IsActive>::size_type size_type;
			// NOTE: here the coefficient of m_tmp gets default-inited explicitly by the default constructor of base term.
			explicit functor(term_type1 const **ptr1, const size_type &s1,
				term_type2 const **ptr2, const size_type &s2, const truncator_type &trunc,
				return_type &retval):
				base::template default_functor<IsActive>(ptr1,s1,ptr2,s2,trunc,retval),
				m_cached_i(0u),m_cached_j(0u),m_insertion_count(0u)
			{}
			void operator()(const size_type &i, const size_type &j) const
			{
				piranha_assert(i < this->m_s1 && j < this->m_s2);
				this->m_tmp.m_key.set_int(this->m_ptr1[i]->m_key.get_int() + this->m_ptr2[j]->m_key.get_int());
				m_cached_i = i;
				m_cached_j = j;
			}
			typedef typename Series1::size_type bucket_size_type;
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
					// NOTE: in fast mode, the check will be done at the end.
					// NOTE: the counters are protected from overflows by the check done in the operator() of the multiplier.
					if (FastMode) {
						container._unique_insert(tmp,bucket_idx);
						++m_insertion_count;
					} else if (likely(!tmp.is_ignorable(args))) {
						container._unique_insert(tmp,bucket_idx);
						container._update_size(container.size() + 1u);
					}
				} else {
					// Assert the existing term is not ignorable, in non-fast mode.
					piranha_assert(FastMode || !it->is_ignorable(args));
					if (FastMode) {
						// In fast mode we do not care if this throws or produces a null coefficient,
						// as we will be dealing with that from outside.
						math::multiply_accumulate(it->m_cf,cf1,cf2);
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
			mutable size_type		m_cached_i;
			mutable size_type		m_cached_j;
			mutable bucket_size_type	m_insertion_count;
		};
};

}

#endif
