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

#ifndef PIRANHA_DETAIL_BASE_SERIES_MULTIPLIER_HPP
#define PIRANHA_DETAIL_BASE_SERIES_MULTIPLIER_HPP

#include <algorithm>
#include <array>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <future>
#include <iterator>
#include <limits>
#include <mutex>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/atomic_utils.hpp"
#include "detail/gcd.hpp"
#include "exceptions.hpp"
#include "key_is_multipliable.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "safe_cast.hpp"
#include "series.hpp"
#include "settings.hpp"
#include "symbol_set.hpp"
#include "thread_pool.hpp"
#include "tuning.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

template <typename Series, typename = void>
struct base_series_multiplier_impl
{
	using term_type = typename Series::term_type;
	using container_type = typename std::decay<decltype(std::declval<Series>()._container())>::type;
	void fill_term_pointers(const container_type &c1, const container_type &c2,
		std::vector<term_type const *> &v1, std::vector<term_type const *> &v2)
	{
		std::transform(c1.begin(),c1.end(),std::back_inserter(v1),[](const term_type &t) {return &t;});
		std::transform(c2.begin(),c2.end(),std::back_inserter(v2),[](const term_type &t) {return &t;});
	}
};

template <typename Series>
struct base_series_multiplier_impl<Series,typename std::enable_if<is_mp_rational<typename Series::term_type::cf_type>::value>::type>
{
	// Useful shortcuts.
	using term_type = typename Series::term_type;
	using rat_type = typename term_type::cf_type;
	using int_type = typename std::decay<decltype(std::declval<rat_type>().num())>::type;
	using container_type = typename std::decay<decltype(std::declval<Series>()._container())>::type;
	void fill_term_pointers(const container_type &c1, const container_type &c2,
		std::vector<term_type const *> &v1, std::vector<term_type const *> &v2)
	{
		// Compute the least common multiplier.
		m_lcm = 1;
		auto it_f = c1.end();
		for (auto it = c1.begin(); it != it_f; ++it) {
			m_lcm = (m_lcm * it->m_cf.den()) / gcd(m_lcm,it->m_cf.den());
		}
		it_f = c2.end();
		for (auto it = c2.begin(); it != it_f; ++it) {
			m_lcm = (m_lcm * it->m_cf.den()) / gcd(m_lcm,it->m_cf.den());
		}
		// All these computations involve only positive numbers,
		// the GCD must always be positive.
		piranha_assert(m_lcm.sign() == 1);
		// Copy over the terms and renormalise to lcm.
		it_f = c1.end();
		for (auto it = c1.begin(); it != it_f; ++it) {
			m_terms1.push_back(term_type(rat_type(m_lcm / it->m_cf.den() * it->m_cf.num(),int_type(1)),it->m_key));
		}
		it_f = c2.end();
		for (auto it = c2.begin(); it != it_f; ++it) {
			m_terms2.push_back(term_type(rat_type(m_lcm / it->m_cf.den() * it->m_cf.num(),int_type(1)),it->m_key));
		}
		// Copy over the pointers.
		std::transform(m_terms1.begin(),m_terms1.end(),std::back_inserter(v1),[](const term_type &t) {return &t;});
		std::transform(m_terms2.begin(),m_terms2.end(),std::back_inserter(v2),[](const term_type &t) {return &t;});
		piranha_assert(v1.size() == c1.size());
		piranha_assert(v2.size() == c2.size());
	}
	std::vector<term_type>	m_terms1;
	std::vector<term_type>	m_terms2;
	int_type		m_lcm;
};

}

/// Base series multiplier.
/**
 * This is a class that provides functionality useful to define a piranha::series_multiplier specialisation. Note that this class
 * alone does *not* fulfill the requirements of a piranha::series_multiplier - it merely provides a set of utilities that can be
 * useful for the implementation of a piranha::series_multiplier.
 *
 * ## Type requirements ##
 *
 * \p Series must satisfy piranha::is_series.
 */
// Some performance ideas:
// - optimisation in case one series has 1 term with unitary key and both series same type: multiply directly coefficients;
// - optimisation for coefficient series that merges all args, similar to the rational optimisation;
// - optimisation for load balancing similar to the poly multiplier.
template <typename Series>
class base_series_multiplier: private detail::base_series_multiplier_impl<Series>
{
		PIRANHA_TT_CHECK(is_series,Series);
	public:
		/// Alias for a vector of const pointers to series terms.
		using v_ptr = std::vector<typename Series::term_type const *>;
		/// The size type of base_series_multiplier::v_ptr.
		using size_type = typename v_ptr::size_type;
		/// The size type of \p Series.
		using bucket_size_type = typename Series::size_type;
	private:
		// Default skip and filter functors.
		struct no_skip
		{
			bool operator()(const size_type &, const size_type &) const
			{
				return false;
			}
		};
		struct no_filter
		{
			unsigned operator()(const size_type &, const size_type &) const
			{
				return 0u;
			}
		};
		// RAII struct to force the clearing of the series used during estimation
		// in face of exceptions.
		struct series_clearer
		{
			explicit series_clearer(Series &s):m_s(s) {}
			~series_clearer()
			{
				m_s._container().clear();
			}
			Series &m_s;
		};
		// The purpose of this helper is to move in a coefficient series during insertion. For series,
		// we know that moves leave the series in a valid state, and series multiplications do not benefit
		// from an already-constructed destination - hence it is convenient to move them rather than copy.
		template <typename Term, typename std::enable_if<!is_series<typename Term::cf_type>::value,int>::type = 0>
		static Term &term_insertion(Term &t)
		{
			return t;
		}
		template <typename Term, typename std::enable_if<is_series<typename Term::cf_type>::value,int>::type = 0>
		static Term term_insertion(Term &t)
		{
			return Term{std::move(t.m_cf),t.m_key};
		}
	public:
		/// Constructor.
		/**
		 * This constructor will store in the base_series_multiplier::m_v1 and base_series_multiplier::m_v2 protected members
		 * references to the terms of the input series \p s1 and \p s2. \p m_v1 will store references to the larger series,
		 * \p m_v2 will store references to the smaller serier. The constructor will also store a copy of the symbol set of
		 * \p s1 and \p s2 in the protected member base_series_multiplier::m_ss.
		 *
		 * If the coefficient type of \p Series is an instance of piranha::mp_rational, then the pointers in \p m_v1 and \p m_v2
		 * will refer not to the original terms in \p s1 and \p s2 but to *copies* of these terms, in which all coefficients
		 * have unitary denominator and the numerators have all been multiplied by the global least common multiplier. This transformation
		 * allows to reduce the multiplication of series with rational coefficients to the multiplication of series with integral coefficients.
		 *
		 * @param[in] s1 first series.
		 * @param[in] s2 second series.
		 *
		 * @throws std::invalid_argument if the symbol sets of \p s1 and \p s2 differ.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers or by the construction
		 * of the term type of \p Series.
		 */
		explicit base_series_multiplier(const Series &s1, const Series &s2):m_ss(s1.get_symbol_set())
		{
			if (unlikely(s1.get_symbol_set() != s2.get_symbol_set())) {
				piranha_throw(std::invalid_argument,"incompatible arguments sets");
			}
			// The largest series goes first.
			const Series *p1 = &s1, *p2 = &s2;
			if (s1.size() < s2.size()) {
				std::swap(p1,p2);
			}
			// This is just an optimisation, no troubles if there is a truncation due to static_cast.
			m_v1.reserve(static_cast<size_type>(p1->size()));
			m_v2.reserve(static_cast<size_type>(p2->size()));
			// Fill in the vectors of pointers.
			this->fill_term_pointers(p1->_container(),p2->_container(),m_v1,m_v2);
		}
		/// Deleted default constructor.
		base_series_multiplier() = delete;
		/// Deleted copy constructor.
		base_series_multiplier(const base_series_multiplier &) = delete;
		/// Deleted move constructor.
		base_series_multiplier(base_series_multiplier &&) = delete;
		/// Deleted copy assignment operator.
		base_series_multiplier &operator=(const base_series_multiplier &) = delete;
		/// Deleted move assignment operator.
		base_series_multiplier &operator=(base_series_multiplier &&) = delete;
	protected:
		/// Blocked multiplication.
		/**
		 * \note
		 * If \p MultFunctor or \p SkipFunctor do not satisfy the requirements outlined below,
		 * a compile-time error will be produced.
		 *
		 * This method is logically equivalent to the following double loop:
		 *
		 * @code
		 * for (size_type i = start1; i < end1; ++i) {
		 *	for (size_type j = start2; j < end2; ++j) {
		 *		if (sf(i,j)) {
		 *			break;
		 *		}
		 *		mf(i,j);
		 *	}
		 * }
		 * @endcode
		 *
		 * \p mf and \p sf must be two function objects with a call operator accepting two instances of
		 * base_series_multiplier::size_type. The call operator of \p mf must return \p void, the call operator of \p sf
		 * must return \p bool. The default type for \p sf exposes a call operator that always
		 * returns \p false unconditionally.
		 *
		 * Internally, the double loops is decomposed in blocks of size tuning::get_multiplication_block_size() in an attempt
		 * to optimise cache memory access patterns.
		 *
		 * This method is meant to be used for series multiplication. \p mf is intended to be a function object that multiplies
		 * the <tt>i</tt>-th term of the first series by the <tt>j</tt>-th term of the second series. \p sf is intended to be
		 * a functor that detects when all subsequent multiplications in the inner loop can be skipped (e.g., if some
		 * truncation criterion is active).
		 *
		 * @param[in] mf the multiplication functor.
		 * @param[in] start1 start index in the first series.
		 * @param[in] end1 end index in the first series.
		 * @param[in] start2 start index in the second series.
		 * @param[in] end2 end index in the second series.
		 * @param[in] sf the skipping functor.
		 *
		 * @throws std::invalid_argument if \p start1 is greater than \p end1 or greater than the size of
		 * base_series_multiplier::m_v1, or if \p end1 is greater than the size of base_series_multiplier::m_v1
		 * (the same holds for the quantities relating to the second series).
		 * @throws unspecified any exception thrown by the call operator of \p mf or \p sf, or piranha::safe_cast.
		 */
		template <typename MultFunctor, typename SkipFunctor = no_skip>
		void blocked_multiplication(const MultFunctor &mf,
			const size_type &start1, const size_type &end1,
			const size_type &start2, const size_type &end2,
			const SkipFunctor &sf = no_skip{}) const
		{
			PIRANHA_TT_CHECK(is_function_object,MultFunctor,void,const size_type &, const size_type &);
			PIRANHA_TT_CHECK(is_function_object,SkipFunctor,bool,const size_type &, const size_type &);
			if (unlikely(start1 > end1 || start1 > m_v1.size() || end1 > m_v1.size())) {
				piranha_throw(std::invalid_argument,"invalid bounds in blocked_multiplication");
			}
			if (unlikely(start2 > end2 || start2 > m_v2.size() || end2 > m_v2.size())) {
				piranha_throw(std::invalid_argument,"invalid bounds in blocked_multiplication");
			}
			// Block size and number of regular blocks.
			const size_type bsize = safe_cast<size_type>(tuning::get_multiplication_block_size()),
				nblocks1 = static_cast<size_type>((end1 - start1) / bsize),
				nblocks2 = static_cast<size_type>((end2 - start2) / bsize);
			// Start and end of last (possibly irregular) blocks.
			const size_type i_ir_start = static_cast<size_type>(nblocks1 * bsize + start1), i_ir_end = end1;
			const size_type j_ir_start = static_cast<size_type>(nblocks2 * bsize + start2), j_ir_end = end2;
			for (size_type n1 = 0u; n1 < nblocks1; ++n1) {
				const size_type i_start = static_cast<size_type>(n1 * bsize + start1),
					i_end = static_cast<size_type>(i_start + bsize);
				// regulars1 * regulars2
				for (size_type n2 = 0u; n2 < nblocks2; ++n2) {
					const size_type j_start = static_cast<size_type>(n2 * bsize + start2),
						j_end = static_cast<size_type>(j_start + bsize);
					for (size_type i = i_start; i < i_end; ++i) {
						for (size_type j = j_start; j < j_end; ++j) {
							if (sf(i,j)) {
								break;
							}
							mf(i,j);
						}
					}
				}
				// regulars1 * rem2
				for (size_type i = i_start; i < i_end; ++i) {
					for (size_type j = j_ir_start; j < j_ir_end; ++j) {
						if (sf(i,j)) {
							break;
						}
						mf(i,j);
					}
				}
			}
			// rem1 * regulars2
			for (size_type n2 = 0u; n2 < nblocks2; ++n2) {
				const size_type j_start = static_cast<size_type>(n2 * bsize + start2),
					j_end = static_cast<size_type>(j_start + bsize);
				for (size_type i = i_ir_start; i < i_ir_end; ++i) {
					for (size_type j = j_start; j < j_end; ++j) {
						if (sf(i,j)) {
							break;
						}
						mf(i,j);
					}
				}
			}
			// rem1 * rem2.
			for (size_type i = i_ir_start; i < i_ir_end; ++i) {
				for (size_type j = j_ir_start; j < j_ir_end; ++j) {
					if (sf(i,j)) {
						break;
					}
					mf(i,j);
				}
			}
		}
		/// Estimate size of series multiplication.
		/**
		 * \note
		 * If \p MultArity, \p MultFunctor or \p FilterFunctor do not satisfy the requirements outlined below,
		 * a compile-time error will be produced.
		 *
		 * This method expects a \p MultFunctor type exposing the same inteface as explained in blocked_multiplication(),
		 * and it assumes that a call <tt>mf(i,j)</tt> multiplies the <tt>i</tt>-th term of the first series by the <tt>j</tt>-th term of
		 * the second series, accumulating the result into \p tmp.
		 *
		 * This method will apply a statistical approach to estimate the final size of the result of the multiplication of the first
		 * series by the second. It will perform random term-by-term multiplications
		 * and deduce the estimate from the number of term multiplications performed before finding the first duplicate term in \p tmp.
		 * The \p MultArity parameter represents the arity of term multiplications - that is, the number of terms generated by a single
		 * term-by-term multiplication. It must be strictly positive.
		 *
		 * The \p ff parameter must be a function object with a call operator accepting two instances of
		 * base_series_multiplier::size_type as input and returning an \p unsigned value not greater than \p MultArity. It represents the number of terms that need to be
		 * discarded in the result of the multiplication of the <tt>i</tt>-th term of the first series by the <tt>j</tt>-th term of the second series
		 * (e.g., because of some truncation criterion). The call operator of the default \p FilterFunctor will always return 0 unconditionally.
		 *
		 * The number returned by this function is always at least 1. \p tmp will always be cleared entering and exiting this method, even in face
		 * of exceptions.
		 *
		 * @param[in] tmp a temporary \p Series into which the term multiplications performed by \p mf will be accumulated.
		 * @param[in] mf the multiplication functor.
		 * @param[in] ff the filtering functor.
		 *
		 * @return the estimated size of the multiplication of the first series by the second, always at least 1.
		 *
		 * @throws std::overflow_error in case of (unlikely) overflows in integral arithmetics.
		 * @throws std::invalid_argument if a call to \p ff returns a value larger than \p MultArity.
		 * @throws unspecified any exception thrown by:
		 * - overflow errors in the conversion of piranha::integer to integral types,
		 * - the call operator of \p mf or \p ff,
		 * - memory errors in standard containers,
		 * - the conversion operator of piranha::integer.
		 */
		template <std::size_t MultArity, typename MultFunctor, typename FilterFunctor = no_filter>
		bucket_size_type estimate_final_series_size(Series &tmp, const MultFunctor &mf, const FilterFunctor &ff = no_filter{}) const
		{
			static_assert(MultArity != 0u,"Invalid multiplication arity in base_series_multiplier.");
			PIRANHA_TT_CHECK(is_function_object,MultFunctor,void,const size_type &, const size_type &);
			PIRANHA_TT_CHECK(is_function_object,FilterFunctor,unsigned,const size_type &, const size_type &);
			// Clear the input series.
			tmp._container().clear();
			// Make sure tmp is cleared in any case on exit.
			series_clearer sc(tmp);
			// Local shortcut.
			constexpr std::size_t result_size = MultArity;
			// Cache these.
			const size_type size1 = m_v1.size(), size2 = m_v2.size();
			// If one of the two series is empty, just return 0.
			if (unlikely(!size1 || !size2)) {
				return 1u;
			}
			// If either series has a size of 1, just return size1 * size2 * result_size.
			if (size1 == 1u || size2 == 1u) {
				return static_cast<bucket_size_type>(integer(size1) * size2 * result_size);
			}
			// NOTE: Hard-coded number of trials = 10.
			// NOTE: here consider that in case of extremely sparse series with few terms this will incur in noticeable
			// overhead, since we will need many term-by-term before encountering the first duplicate.
			const auto ntrials = 10u;
			// NOTE: Hard-coded value for the estimation multiplier.
			// NOTE: This value should be tuned for performance/memory usage tradeoffs.
			const auto multiplier = 2u;
			// Vectors of indices into m_v1/m_v2.
			std::vector<size_type> v_idx1, v_idx2;
			// Try to reserve space in advance.
			v_idx1.reserve(static_cast<typename std::vector<size_type>::size_type>(size1));
			v_idx2.reserve(static_cast<typename std::vector<size_type>::size_type>(size2));
			for (size_type i = 0u; i < size1; ++i) {
				v_idx1.push_back(i);
			}
			for (size_type i = 0u; i < size2; ++i) {
				v_idx2.push_back(i);
			}
			// Maximum number of random multiplications before which a duplicate term must be generated.
			const size_type max_M = static_cast<size_type>(((integer(size1) * size2) / multiplier).sqrt());
			// Random number engine.
			std::mt19937 engine;
			// Init total and filter counters.
			integer total(0), tot_filtered(0);
			// Go with the trials.
			// NOTE: This could be easily parallelised, but not sure if it is worth it.
			for (auto n = 0u; n < ntrials; ++n) {
				// Randomise.
				std::shuffle(v_idx1.begin(),v_idx1.end(),engine);
				std::shuffle(v_idx2.begin(),v_idx2.end(),engine);
				size_type count = 0u, filtered = 0u;
				auto it1 = v_idx1.begin(), it2 = v_idx2.begin();
				while (count < max_M) {
					if (it1 == v_idx1.end()) {
						// Each time we wrap around the first series,
						// wrap around also the second one and rotate it.
						it1 = v_idx1.begin();
						auto middle = v_idx2.end();
						--middle;
						std::rotate(v_idx2.begin(),middle,v_idx2.end());
						it2 = v_idx2.begin();
					}
					if (it2 == v_idx2.end()) {
						it2 = v_idx2.begin();
					}
					// Perform term multiplication.
					mf(*it1,*it2);
					// Count the filtered terms.
					const unsigned filter_count = ff(*it1,*it2);
					// Sanity check.
					if (unlikely(filter_count > result_size)) {
						piranha_throw(std::invalid_argument,
							"the number of filtered terms cannot be larger than result_size");
					}
					// Check for unlikely overflows when increasing counts.
					if (unlikely(result_size > std::numeric_limits<size_type>::max() ||
						count > std::numeric_limits<size_type>::max() - result_size))
					{
						piranha_throw(std::overflow_error,"overflow error");
					}
					if (unlikely(filter_count > std::numeric_limits<size_type>::max() ||
						filtered > std::numeric_limits<size_type>::max() - filter_count))
					{
						piranha_throw(std::overflow_error,"overflow error");
					}
					if (tmp.size() != count + result_size) {
						break;
					}
					// Increase cycle variables.
					count = static_cast<size_type>(count + result_size);
					filtered = static_cast<size_type>(filtered + filter_count);
					++it1;
					++it2;
				}
				total += count;
				tot_filtered += filtered;
				// Reset tmp.
				tmp._container().clear();
			}
			piranha_assert(total >= tot_filtered);
			const auto mean = (total - tot_filtered) / ntrials;
			auto retval = mean * mean * multiplier;
			// Return always at least one.
			if (retval.sign() == 0) {
				return 1u;
			} else {
				return static_cast<bucket_size_type>(retval);
			}
		}
		/// A plain multiplier functor.
		/**
		 * \note
		 * If the key and coefficient types of \p Series do not satisfy piranha::key_is_multipliable, a compile-time error will
		 * be produced.
		 *
		 * This is a functor that conforms to the protocol expected by base_series_multiplier::blocked_multiplication() and
		 * base_series_multiplier::estimate_final_series_size().
		 *
		 * This functor requires that the key and coefficient types of \p Series satisfy piranha::key_is_multipliable, and it will
		 * use the key's <tt>multiply()</tt> method to perform term-by-term multiplications.
		 *
		 * If the \p FastMode boolean parameter is \p true, then the call operator will insert terms into the return value series
		 * using the low-level interface of piranha::hash_set, otherwise the call operator will use piranha::series::insert() for
		 * term insertion.
		 */
		template <bool FastMode>
		class plain_multiplier
		{
				using term_type = typename Series::term_type;
				using key_type = typename term_type::key_type;
				PIRANHA_TT_CHECK(key_is_multipliable,typename term_type::cf_type,key_type);
				using it_type = decltype(std::declval<Series &>()._container().end());
				static constexpr std::size_t m_arity = key_type::multiply_arity;
			public:
				/// Constructor.
				/**
				 * The constructor will store references to the input arguments internally.
				 *
				 * @param[in] v1 a const reference to a vector of term pointers to the first series.
				 * @param[in] v2 a const reference to a vector of term pointers to the second series.
				 * @param[in] retval the \p Series instance into which terms resulting from multiplications will be inserted.
				 */
				explicit plain_multiplier(const std::vector<term_type const *>	&v1, const std::vector<term_type const *> &v2,
					Series &retval):m_v1(v1),m_v2(v2),m_retval(retval),m_c_end(retval._container().end())
				{}
				/// Call operator.
				/**
				 * The call operator will perform the multiplication of the <tt>i</tt>-th term of the first series by the
				 * <tt>j</tt>-th term of the second series, and it will insert the result into the return value.
				 *
				 * @param[in] i index of a term in the first series.
				 * @param[in] j index of a term in the second series.
				 *
				 * @throws unspecified any exception thrown by:
				 * - piranha::series::insert(),
				 * - the low-level interface of piranha::hash_set,
				 * - the in-place addition operator of the coefficient type,
				 * - term construction.
				 */
				void operator()(const size_type &i, const size_type &j) const
				{
					// First perform the multiplication.
					key_type::multiply(m_tmp_t,*m_v1[i],*m_v2[j],m_retval.get_symbol_set());
					for (std::size_t n = 0u; n < m_arity; ++n) {
						auto &tmp_term = m_tmp_t[n];
						if (FastMode) {
							auto &container = m_retval._container();
							// Try to locate the term into retval.
							auto bucket_idx = container._bucket(tmp_term);
							const auto it = container._find(tmp_term,bucket_idx);
							if (it == m_c_end) {
								container._unique_insert(term_insertion(tmp_term),bucket_idx);
							} else {
								it->m_cf += tmp_term.m_cf;
							}
						} else {
							m_retval.insert(term_insertion(tmp_term));
						}
					}
				}
			private:
				mutable std::array<term_type,m_arity>	m_tmp_t;
				const std::vector<term_type const *>	&m_v1;
				const std::vector<term_type const *>	&m_v2;
				Series					&m_retval;
				const it_type				m_c_end;
		};
		/// Sanitise series.
		/**
		 * When using the low-level interface of piranha::hash_set for term insertion, invariants might be violated
		 * both in piranha::hash_set and piranha::series. In particular:
		 *
		 * - terms may not be checked for compatibility or ignorability upon insertion,
		 * - the count of elements in piranha::hash_set might not be updated.
		 *
		 * This method can be used to fix these invariants: each term of \p retval will be checked for ignorability and compatibility,
		 * and the total count of terms in the series will be set to the number of non-ignorable terms. Ignorable terms will
		 * be erased.
		 *
		 * @param[in] retval the series to be sanitised.
		 * @param[in] n_threads the number of threads to be used.
		 *
		 * @throws std::invalid_argument if \p n_threads is zero, or if one of the terms in \p retval is not compatible
		 * with the symbol set of \p retval.
		 * @throws std::overflow_error if the number of terms in \p retval overflows the maximum value representable by
		 * base_series_multiplier::bucket_size_type.
		 * @throws unspecified any exception thrown by:
		 * - the cast operator of piranha::integer,
		 * - standard threading primitives,
		 * - thread_pool::enqueue(),
		 * - future_list::push_back().
		 */
		static void sanitize_series(Series &retval, unsigned n_threads)
		{
			using term_type = typename Series::term_type;
			if (unlikely(n_threads == 0u)) {
				piranha_throw(std::invalid_argument,"invalid number of threads");
			}
			auto &container = retval._container();
			const auto &args = retval.get_symbol_set();
			// Reset the size to zero before doing anything.
			container._update_size(static_cast<bucket_size_type>(0u));
			// Single-thread implementation.
			if (n_threads == 1u) {
				const auto it_end = container.end();
				for (auto it = container.begin(); it != it_end;) {
					if (unlikely(!it->is_compatible(args))) {
						piranha_throw(std::invalid_argument,"incompatible term");
					}
					if (unlikely(container.size() == std::numeric_limits<bucket_size_type>::max())) {
						piranha_throw(std::overflow_error,"overflow error in the number of terms of a series");
					}
					// First update the size, it will be scaled back in the erase() method if necessary.
					container._update_size(static_cast<bucket_size_type>(container.size() + 1u));
					if (unlikely(it->is_ignorable(args))) {
						it = container.erase(it);
					} else {
						++it;
					}
				}
				return;
			}
			// Multi-thread implementation.
			const auto b_count = container.bucket_count();
			std::mutex m;
			integer global_count(0);
			auto eraser = [b_count,&container,&m,&args,&global_count](const bucket_size_type &start, const bucket_size_type &end) {
				piranha_assert(start <= end && end <= b_count);
				(void)b_count;
				bucket_size_type count = 0u;
				std::vector<term_type> term_list;
				// Examine and count the terms bucket-by-bucket.
				for (bucket_size_type i = start; i != end; ++i) {
					term_list.clear();
					const auto &bl = container._get_bucket_list(i);
					const auto it_f = bl.end();
					for (auto it = bl.begin(); it != it_f; ++it) {
						// Check first for compatibility.
						if (unlikely(!it->is_compatible(args))) {
							piranha_throw(std::invalid_argument,"incompatible term");
						}
						// Check for ignorability.
						if (unlikely(it->is_ignorable(args))) {
							term_list.push_back(*it);
						}
						// Update the count of terms.
						if (unlikely(count == std::numeric_limits<bucket_size_type>::max())) {
							piranha_throw(std::overflow_error,"overflow error in the number of terms of a series");
						}
						count = static_cast<bucket_size_type>(count + 1u);
					}
					for (auto it = term_list.begin(); it != term_list.end(); ++it) {
						// NOTE: must use _erase to avoid concurrent modifications
						// to the number of elements in the table.
						container._erase(container._find(*it,i));
						// Account for the erased term.
						piranha_assert(count > 0u);
						count = static_cast<bucket_size_type>(count - 1u);
					}
				}
				// Update the global count.
				std::lock_guard<std::mutex> lock(m);
				global_count += count;
			};
			future_list<decltype(thread_pool::enqueue(0u,eraser,bucket_size_type(),bucket_size_type()))> f_list;
			try {
				for (unsigned i = 0u; i < n_threads; ++i) {
					const auto start = static_cast<bucket_size_type>((b_count / n_threads) * i),
						end = static_cast<bucket_size_type>((i == n_threads - 1u) ? b_count : (b_count / n_threads) * (i + 1u));
					f_list.push_back(thread_pool::enqueue(i,eraser,start,end));
				}
				// First let's wait for everything to finish.
				f_list.wait_all();
				// Then, let's handle the exceptions.
				f_list.get_all();
			} catch (...) {
				f_list.wait_all();
				// NOTE: there's not need to clear retval here - it was already in an inconsistent
				// state coming into this method. We rather need to make sure sanitize_series() is always
				// called in a try/catch block that clears retval in case of errors.
				throw;
			}
			// Final update of the total count.
			container._update_size(static_cast<bucket_size_type>(global_count));
		}
		/// A plain series multiplication routine.
		/**
		 * \note
		 * If the key and coefficient types of \p Series do not satisfy piranha::key_is_multipliable, or \p SkipFunctor does
		 * not satisfy the requirements outlined in base_series_multiplier::blocked_multiplication(), a compile-time error will
		 * be produced.
		 *
		 * This method implements a generic series multiplication routine suitable for key types that satisfy piranha::key_is_multipliable.
		 * The implementation is either single-threaded or multi-threaded, depending on the sizes of the input series, and it will use
		 * either base_series_multiplier::plain_multiplier or a similar thread-safe multiplier for the term-by-term multiplications.
		 *
		 * Note that, in multithreaded mode, \p sf will be shared among (and called concurrently from) all the threads.
		 *
		 * @param[in] sf the skipping functor (see base_series_multiplier::blocked_multiplication()).
		 *
		 * @return the series resulting from the multiplication of the two series used to construct \p this.
		 *
		 * @throws unspecified any exception thrown by:
		 * - piranha::safe_cast(),
		 * - base_series_multiplier::estimate_final_series_size(),
		 * - <tt>boost::numeric_cast()</tt>,
		 * - the low-level interface of piranha::hash_set,
		 * - base_series_multiplier::blocked_multiplication(),
		 * - base_series_multiplier::sanitize_series(),
		 * - the <tt>multiply()</tt> method of the key type of \p Series,
		 * - thread_pool::enqueue(),
		 * - future_list::push_back(),
		 * - the construction of terms,
		 * - in-place addition of coefficients.
		 */
		template <typename SkipFunctor = no_skip>
		Series plain_multiplication(const SkipFunctor &sf = no_skip{}) const
		{
			// Shortcuts.
			using term_type = typename Series::term_type;
			using cf_type = typename term_type::cf_type;
			using key_type = typename term_type::key_type;
			PIRANHA_TT_CHECK(key_is_multipliable,cf_type,key_type);
			PIRANHA_TT_CHECK(is_function_object,SkipFunctor,bool,const size_type &, const size_type &);
			constexpr std::size_t m_arity = key_type::multiply_arity;
			// Do not do anything if one of the two series is empty.
			if (unlikely(m_v1.empty() || m_v2.empty())) {
				// NOTE: requirement is ok, a series must be def-ctible.
				return Series{};
			}
			const size_type size1 = m_v1.size(), size2 = m_v2.size();
			piranha_assert(size1 && size2);
			// Establish the number of threads to use.
			size_type n_threads = safe_cast<size_type>(thread_pool::use_threads(
				integer(size1) * size2,integer(settings::get_min_work_per_thread())
			));
			piranha_assert(n_threads);
			// An additional check on n_threads is that its size is not greater than the size of the first series,
			// as we are using the first operand to split up the work.
			if (n_threads > size1) {
				n_threads = size1;
			}
			piranha_assert(n_threads > 0u);
			// Common setup for st/mt.
			Series retval;
			retval.set_symbol_set(m_ss);
			// Estimate and rehash.
			const auto est = estimate_final_series_size<m_arity>(retval,plain_multiplier<false>(m_v1,m_v2,retval));
			// NOTE: use numeric cast here as safe_cast is expensive, going through an integer-double conversion,
			// and in this case the behaviour of numeric_cast is appropriate.
			const auto n_buckets = boost::numeric_cast<bucket_size_type>(std::ceil(static_cast<double>(est)
					/ retval._container().max_load_factor()));
			piranha_assert(n_buckets > 0u);
			// Check if we want to use the parallel memory set.
			// NOTE: it is important here that we use the same n_threads for multiplication and memset as
			// we tie together pinned threads with potentially different NUMA regions.
			const unsigned n_threads_rehash = tuning::get_parallel_memory_set() ?
				static_cast<unsigned>(n_threads) : 1u;
			retval._container().rehash(n_buckets,n_threads_rehash);
			if (n_threads == 1u) {
				try {
					// Single-thread case.
					blocked_multiplication(plain_multiplier<true>(m_v1,m_v2,retval),0u,size1,0u,size2,sf);
					sanitize_series(retval,static_cast<unsigned>(n_threads));
					return retval;
				} catch (...) {
					retval._container().clear();
					throw;
				}
			}
			// Multi-threaded case.
			// Init the vector of spinlocks.
			detail::atomic_flag_array sl_array(safe_cast<std::size_t>(retval._container().bucket_count()));
			// Init the future list.
			future_list<std::future<void>> f_list;
			// Thread block size.
			const auto block_size = size1 / n_threads;
			piranha_assert(block_size > 0u);
			try {
				for (size_type i = 0u; i < n_threads; ++i) {
					// Thread functor.
					auto tf = [i,this,block_size,n_threads,&sl_array,&retval,&sf]()
					{
						// Used to store the result of term multiplication.
						std::array<term_type,key_type::multiply_arity> tmp_t;
						// End of retval container (thread-safe).
						const auto c_end = retval._container().end();
						// Block functor.
						// NOTE: this is very similar to the plain functor, but it does the bucket locking
						// additionally.
						auto f = [&c_end,&tmp_t,this,&retval,&sl_array] (const size_type &i, const size_type &j)
						{
							// Run the term multiplication.
							key_type::multiply(tmp_t,*(this->m_v1[i]),*(this->m_v2[j]),retval.get_symbol_set());
							for (std::size_t n = 0u; n < key_type::multiply_arity; ++n) {
								auto &container = retval._container();
								auto &tmp_term = tmp_t[n];
								// Try to locate the term into retval.
								auto bucket_idx = container._bucket(tmp_term);
								// Lock the bucket.
								detail::atomic_lock_guard alg(sl_array[static_cast<std::size_t>(bucket_idx)]);
								const auto it = container._find(tmp_term,bucket_idx);
								if (it == c_end) {
									container._unique_insert(term_insertion(tmp_term),bucket_idx);
								} else {
									it->m_cf += tmp_term.m_cf;
								}
							}
						};
						// Thread block limit.
						const auto e1 = (i == n_threads - 1u) ? this->m_v1.size() :
							static_cast<size_type>((i + 1u) * block_size);
						this->blocked_multiplication(f,static_cast<size_type>(i * block_size),e1,0u,this->m_v2.size(),sf);
					};
					f_list.push_back(thread_pool::enqueue(static_cast<unsigned>(i),tf));
				}
				f_list.wait_all();
				f_list.get_all();
				sanitize_series(retval,static_cast<unsigned>(n_threads));
			} catch (...) {
				f_list.wait_all();
				// Clean up retval as it might be in an inconsistent state.
				retval._container().clear();
				throw;
			}
			return retval;
		}
	protected:
		/// Vector of const pointers to the terms in the larger series.
		mutable v_ptr		m_v1;
		/// Vector of const pointers to the terms in the smaller series.
		mutable v_ptr		m_v2;
		/// The symbol set of the series used during construction.
		const symbol_set	m_ss;
};

}

#endif
