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

#ifndef PIRANHA_SERIES_MULTIPLIER_HPP
#define PIRANHA_SERIES_MULTIPLIER_HPP

#include <algorithm>
#include <boost/any.hpp>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <list>
#include <new> // For bad_alloc.
#include <numeric>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "cache_aligning_allocator.hpp"
#include "concepts/series.hpp"
#include "config.hpp"
#include "detail/series_fwd.hpp"
#include "detail/series_multiplier_fwd.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "task_group.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "tracing.hpp"
#include "truncator.hpp"

namespace piranha
{

/// Default series multiplier.
/**
 * This class is used by the multiplication operators involving two series operands with the same echelon size. The class works as follows:
 * 
 * - an instance of series multiplier is created using two series (possibly of different types) as construction arguments;
 * - when operator()() is called, an instance of the piranha::series type from which \p Series1 derives is returned, representing
 *   the result of the multiplication of the two series used for construction.
 * 
 * Any specialisation of this class must respect the protocol described above (i.e., construction from series
 * instances and operator()() to compute the result). Note that this class is guaranteed to be used after the symbolic arguments of the series used for construction
 * have been merged (in other words, the two series have identical symbolic arguments sets).
 * Note also that the return type of the multiplication is the piranha::series base type of \p Series1, independently from the rules governing coefficient arithmetics.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Series1 and \p Series2 must be models of piranha::concept::Series. Additionally, the echelon sizes of
 *   \p Series1 and \p Series2 must be the same, otherwise a compile-time assertion will fail.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo series multiplier concept? -> do the same thing done for truncator class.
 * \todo mention this must be specialised using enable_if.
 * \todo in the heuristic to decide single vs multithread we should take into account if coefficient is series or not, and maybe provide
 * means via template specialisation to customise the behaviour for different types of coefficients -> probably the easiest thing is to benchmark
 * the thread overhead in the simplest case (e.g., polynomial with double precision cf and univariate) and use that as heuristic. Might not be optimal
 * but should avoid excessive overhead practically always (which probably is what we want).
 * \todo optimization in case one series has 1 term with unitary key and both series same type: multiply directly coefficients.
 * \todo think about the possibility of caching optimizations. For instance: merge the arguments of series coefficients, avoiding n ** 2 merge
 * operations during multiplication. Or: have specialised functors cache degree/norms of terms for truncation purposes, in order to avoid
 * computing them each time.
 * \todo possibly we could adopt some of the optimizations adopted, e.g., in the Kronecker multiplier. For instance, have a fast mode for the multiplier
 * to kick in when doing the full computation in order to avoid some branching in the insertion routines. The code though is already quite complex,
 * so better be very sure it is worth before embarking in this.
 */
template <typename Series1, typename Series2, typename Enable = void>
class series_multiplier
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series1>));
		BOOST_CONCEPT_ASSERT((concept::Series<Series2>));
		static_assert(echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value,
			"Mismatch in echelon sizes.");
	protected:
		/// Alias for term type of \p Series1.
		typedef typename Series1::term_type term_type1;
		/// Alias for term type of \p Series2.
		typedef typename Series2::term_type term_type2;
		/// Type of the result of the multiplication.
		/**
		 * Return type is the base piranha::series type of \p Series1.
		 */
		typedef series<term_type1,Series1> return_type;
		/// Alias for the truncator type.
		typedef truncator<Series1,Series2> truncator_type;
	private:
		static_assert(std::is_same<return_type,decltype(std::declval<Series1>().multiply_by_series(std::declval<Series1>()))>::value,
			"Invalid return_type.");
		// Alias for truncator traits.
		typedef truncator_traits<Series1,Series2> ttraits;
		// Swap operands if series types are the same and first series is shorter than the second,
		// as the first series is used to split work among threads in mt mode.
		template <typename T>
		void swap_operands(const T &s1, const T &s2)
		{
			if (s1.size() < s2.size()) {
				std::swap(m_s1,m_s2);
			}
		}
		template <typename T, typename U>
		void swap_operands(const T &, const U &)
		{}
	public:
		/// Constructor.
		/**
		 * @param[in] s1 first series.
		 * @param[in] s2 second series.
		 * 
		 * @throws std::invalid_argument if the symbol sets of \p s1 and \p s2 differ.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers.
		 */
		explicit series_multiplier(const Series1 &s1, const Series2 &s2) : m_s1(&s1),m_s2(&s2)
		{
			swap_operands(s1,s2);
			if (unlikely(m_s1->m_symbol_set != m_s2->m_symbol_set)) {
				piranha_throw(std::invalid_argument,"incompatible arguments sets");
			}
			if (unlikely(m_s1->empty() || m_s2->empty())) {
				return;
			}
			m_v1.reserve(m_s1->size());
			m_v2.reserve(m_s2->size());
			// Fill in the vectors of pointers.
			std::back_insert_iterator<decltype(m_v1)> bii1(m_v1);
			std::transform(m_s1->m_container.begin(),m_s1->m_container.end(),bii1,[](const term_type1 &t) {return &t;});
			std::back_insert_iterator<decltype(m_v2)> bii2(m_v2);
			std::transform(m_s2->m_container.begin(),m_s2->m_container.end(),bii2,[](const term_type2 &t) {return &t;});
		}
		/// Deleted copy constructor.
		series_multiplier(const series_multiplier &) = delete;
		/// Deleted move constructor.
		series_multiplier(series_multiplier &&) = delete;
		/// Deleted copy assignment operator.
		series_multiplier &operator=(const series_multiplier &) = delete;
		/// Deleted move assignment operator.
		series_multiplier &operator=(series_multiplier &&) = delete;
		/// Compute result of series multiplication.
		/**
		 * This method will create a piranha::truncator object from the series operands and
		 * call execute() using default_functor as multiplication functor. The \p IsActive flag
		 * of the functor will be set according to the activity flag of the truncator.
		 * 
		 * @return the result of multiplying the two series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of a piranha::truncator object from the series operands,
		 * - the <tt>is_active()</tt> method of the truncator object,
		 * - execute().
		 */
		return_type operator()() const
		{
			const truncator_type t(*m_s1,*m_s2);
			if (t.is_active()) {
				return execute<default_functor<true>>(t);
			} else {
				return execute<default_functor<false>>(t);
			}
		}
	protected:
		/// Determine the number of threads to use in the multiplication.
		/**
		 * The number of threads that will be opened will never exceed \p n, the output of piranha::settings::get_n_threads(),
		 * and it is determined as follows:
		 * 
		 * - if the method is not called from the main thread, then 1 will be returned;
		 * - if the number of term-by-term multiplications that would be performed per thread using \p n
		 *   threads is greater than an implementation-defined minimum, then \p n is returned;
		 * - otherwise, the number of threads is decreased as necessary to meet the minimum thread workload.
		 * 
		 * The minimum threshold is determined by comparing the typical cost of thread creation and join
		 * with the cost of a single term-by-term multiplication in the best-case scenario. Currently, the
		 * value of the threshold is 100000.
		 * 
		 * @return the number of threads to use in the multiplication.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::settings::get_n_threads(),
		 * - the cast operator of piranha::integer to integral types.
		 */
		unsigned determine_n_threads() const
		{
			// Use just one thread if we are not in the main thread.
			if (runtime_info::get_main_thread_id() != this_thread::get_id()) {
				return 1u;
			}
			const unsigned candidate = settings::get_n_threads();
			piranha_assert(candidate);
			// Avoid further calculations for just 1 thread.
			if (candidate == 1u) {
				return 1u;
			}
			// Minimum amount of term-by-term multiplications.
			// NOTE: this corresponds to roughly a 10% overhead of thread creation/join
			// in the fastest multiplication scenario (polynomials with double-precision
			// coefficients suitable for Kronecker multiplication in lookup array - e.g.,
			// Fateman benchmarks) on an Intel Sandy Bridge from 2011.
			const auto min_work = 100000u;
			const auto work_size = integer(m_s1->size()) * m_s2->size();
			if (work_size / candidate >= min_work) {
				return candidate;
			} else {
				return static_cast<unsigned>(std::max<integer>(integer(1),work_size / min_work));
			}
		}
		/// Low-level implementation of series multiplication.
		/**
		 * The multiplication algorithm proceeds as follows:
		 * 
		 * - if one of the two series is empty, a default-constructed instance of \p return_type is returned;
		 * - a heuristic determines whether to enable multi-threaded mode or not;
		 * - in single-threaded mode:
		 *   - an instance of \p Functor is created (using \p trunc as truncator object for construction)
		 *     and used to compute the return value via term-by-term multiplications and
		 *     insertions in the return series;
		 * - in multi-threaded mode:
		 *   - the first series is subdivided into segments and the same process described for single-threaded mode is run in parallel,
		 *     storing the multiple resulting series in a list;
		 *   - the series in the result list are merged into a single series via piranha::series::insert().
		 * 
		 * \p Functor must be a type exposing the same public interface as default_functor. It will be used to compute term-by-term multiplications
		 * and insert the terms into the return series respecting the current truncator settings.
		 * 
		 * @param[in] trunc a piranha::truncator object constructed from the series operands of the multiplication.
		 * 
		 * @return the result of multiplying the first series by the second series.
		 * 
		 * @throws std::overflow_error in case of overflowing arithmetic operations on integral types.
		 * @throws unspecified any exception thrown by:
		 * - memory allocation errors in standard containers,
		 * - \p boost::numeric_cast (in case of out-of-range convertions from one integral type to another),
		 * - the public methods of \p Functor,
		 * - errors in threading primitives,
		 * - piranha::series::insert().
		 */
		template <typename Functor>
		return_type execute(const truncator_type &trunc) const
		{
			// Do not do anything if one of the two series is empty.
			if (unlikely(m_s1->empty() || m_s2->empty())) {
				return return_type{};
			}
			// This is the size type that will be used throughout the calculations.
			typedef decltype(m_v1.size()) size_type;
			const size_type size1 = m_v1.size(), size2 = boost::numeric_cast<size_type>(m_v2.size());
			piranha_assert(size1 && size2);
			// Establish the number of threads to use.
			size_type n_threads = boost::numeric_cast<size_type>(determine_n_threads());
			piranha_assert(n_threads);
			// An additional check on n_threads is that its size is not greater than the size of the first series,
			// as we are using the first operand to split up the work.
			if (n_threads > size1) {
				n_threads = size1;
			}
			piranha_assert(n_threads >= 1u);
			if (likely(n_threads == 1u)) {
				return_type retval;
				retval.m_symbol_set = m_s1->m_symbol_set;
				Functor f(&m_v1[0u],size1,&m_v2[0u],size2,trunc,retval);
				const auto tmp = rehasher(f);
				blocked_multiplication(f);
				if (tmp.first) {
					trace_estimates(retval.size(),tmp.second);
				}
				return retval;
			} else {
				// Build the return values and the multiplication functors.
				std::list<return_type,cache_aligning_allocator<return_type>> retval_list;
				std::list<Functor,cache_aligning_allocator<Functor>> functor_list;
				const auto block_size = size1 / n_threads;
				for (size_type i = 0u; i < n_threads; ++i) {
					// Last thread needs a different size from block_size.
					const size_type s1 = (i == n_threads - 1u) ? (size1 - i * block_size) : block_size;
					retval_list.push_back(return_type{});
					retval_list.back().m_symbol_set = m_s1->m_symbol_set;
					functor_list.push_back(Functor(&m_v1[0u] + i * block_size,s1,&m_v2[0u],size2,trunc,retval_list.back()));
				}
				auto f_it = functor_list.begin();
				auto r_it = retval_list.begin();
				task_group tg;
				try {
					for (size_type i = 0u; i < n_threads; ++i, ++f_it, ++r_it) {
						// Functor for use in the thread.
						auto f = [f_it,r_it,this]() {
							thread_management::binder binder;
							const auto tmp = this->rehasher(*f_it);
							this->blocked_multiplication(*f_it);
							if (tmp.first) {
								this->trace_estimates(r_it->m_container.size(),tmp.second);
							}
						};
						tg.add_task(f);
					}
					piranha_assert(tg.size() == n_threads);
					tg.wait_all();
					tg.get_all();
				} catch (...) {
					tg.wait_all();
					throw;
				}
				return_type retval;
				retval.m_symbol_set = m_s1->m_symbol_set;
				auto final_estimate = estimate_final_series_size(Functor(&m_v1[0u],size1,&m_v2[0u],size2,trunc,retval));
				// We want to make sure that final_estimate contains at least 1 element, so that we can use faster low-level
				// methods in hash_set.
				if (unlikely(!final_estimate)) {
					final_estimate = 1u;
				}
				// Try to see if a series already has enough buckets.
				auto it = std::find_if(retval_list.begin(),retval_list.end(),[&final_estimate](const return_type &r) {
					return r.m_container.bucket_count() * r.m_container.max_load_factor() >= final_estimate;
				});
				if (it != retval_list.end()) {
					retval = std::move(*it);
					retval_list.erase(it);
				} else {
					// Otherwise, just rehash to the desired value, corrected for max load factor.
					retval.m_container.rehash(
						boost::numeric_cast<typename Series1::size_type>(std::ceil(final_estimate / retval.m_container.max_load_factor()))
					);
				}
				// Cleanup functor that will erase all elements in retval_list.
				auto cleanup = [&retval_list]() -> void {
					std::for_each(retval_list.begin(),retval_list.end(),[](return_type &r) {r.m_container.clear();});
				};
				try {
					final_merge(retval,retval_list,n_threads);
					// Here the estimate will be correct if the initial estimate (corrected for load factor)
					// is at least equal to the final real bucket count.
					trace_estimates(retval.m_container.size(),final_estimate);
				} catch (...) {
					// Do the cleanup before re-throwing, as we use mutable iterators in final_merge.
					cleanup();
					// Clear also retval: it could have bogus number of elements if errors arise in the final merge.
					retval.m_container.clear();
					throw;
				}
				// Clean up the temporary series.
				cleanup();
				return retval;
			}
		}
		/// Default multiplication functor.
		/**
		 * This multiplication functor uses the <tt>multiply()</tt> method of \p term_type1 to compute the result of
		 * term-by-term multiplications, and employs the piranha::series::insert() method to accumulate the terms
		 * in the return value. The functor will use the active truncator settings to sort, skip and filter terms
		 * as necessary.
		 * 
		 * The \p IsActive template boolean flag is used to optimize those cases in which the truncator overhead can
		 * be avoided because the truncator itself is not active. In such a case, the functor must be instantiated with
		 * \p IsActive set to \p false, so that frequent truncation-related operations
		 * can be skipped altogether, hence improving performance.
		 * 
		 * The \p SortOnConstruction boolean flag is used during construction to conditionally prevent the sorting of input terms.
		 */
		template <bool IsActive, bool SortOnConstruction = true>
		class default_functor
		{
			public:
				/// Alias for the size type.
				/**
				 * This unsigned type will be used to represent sizes throughout the calculations.
				 */
				typedef typename std::vector<term_type1 const *>::size_type size_type;
			private:
				// Meta-programmed helpers for skipping and filtering.
				void sort_for_skip(const std::true_type &) const
				{
					auto sorter1 = [this](const term_type1 *t1, const term_type1 *t2) {
						return this->m_trunc.compare_terms(*t1,*t2);
					};
					auto sorter2 = [this](const term_type2 *t1, const term_type2 *t2) {
						return this->m_trunc.compare_terms(*t1,*t2);
					};
					std::sort(m_ptr1,m_ptr1 + m_s1,sorter1);
					std::sort(m_ptr2,m_ptr2 + m_s2,sorter2);
				}
				void sort_for_skip(const std::false_type &) const {}
				bool skip_impl(const size_type &i, const size_type &j, const std::true_type &) const
				{
					return m_trunc.skip(*m_ptr1[i],*m_ptr2[j]);
				}
				bool skip_impl(const size_type &, const size_type &, const std::false_type &) const
				{
					return false;
				}
				bool filter_impl(const term_type1 &t, const std::true_type &) const
				{
					return m_trunc.filter(t);
				}
				bool filter_impl(const term_type1 &, const std::false_type &) const
				{
					return false;
				}
				// Functor for the insertion of the terms.
				template <bool CheckFilter, typename Tuple, std::size_t N = 0u, typename Enable2 = void>
				struct inserter
				{
					static_assert(N < boost::integer_traits<std::size_t>::const_max,
							"Overflow error.");
					static void run(const default_functor &f, Tuple &t)
					{
						if (!ttraits::is_skipping && CheckFilter && f.filter(std::get<N>(t))) {
							// Do not insert.
						} else {
							f.m_retval.insert(std::get<N>(t));
						}
						inserter<CheckFilter,Tuple,N + 1u>::run(f,t);
					}
				};
				template <bool CheckFilter, typename Tuple, std::size_t N>
				struct inserter<CheckFilter,Tuple,N,typename std::enable_if<N == std::tuple_size<Tuple>::value>::type>
				{
					static void run(const default_functor &, Tuple &)
					{}
				};
				template <bool CheckFilter, typename... Args>
				void insert_impl(std::tuple<Args...> &mult_res) const
				{
					inserter<CheckFilter,std::tuple<Args...>>::run(*this,mult_res);
				}
				template <bool CheckFilter>
				void insert_impl(typename Series1::term_type &mult_res) const
				{
					// NOTE: the check on the traits here is because if the truncator is skipping, we assume that
					// any necessary filtering has been done by the skip method.
					if (!ttraits::is_skipping && CheckFilter && filter(mult_res)) {
						// Do not insert.
					} else {
						m_retval.insert(mult_res);
					}
				}
			public:
				/// Constructor.
				/**
				 * The functor is constructed from arrays of pointers to the input series terms on which the functor
				 * will operate, a piranha::truncator object and the return value into which the results of term-by-term
				 * multiplications will be accumulated. The input parameters (or references to them) are stored as public
				 * class members for later use.
				 * 
				 * If the associated truncator object is a skipping truncator, the \p IsActive flag is \p true
				 * and the \p SortOnConstruction flag is true, the arrays of
				 * pointers to terms will be sorted according to the truncator. Otherwise, the input term pointers arrays
				 * will be unmodified and it will be up to the user of the functor to sort them according to the active
				 * truncator.
				 * 
				 * @param[in] ptr1 start of the first array of pointers.
				 * @param[in] s1 size of the first array of pointers.
				 * @param[in] ptr2 start of the second array of pointers.
				 * @param[in] s2 size of the second array of pointers.
				 * @param[in] trunc piranha::truncator object associated to the multiplication.
				 * @param[in] retval series into which the result of the multiplication will be accumulated.
				 * 
				 * @throws std::invalid_argument if \p IsActive is different from the output of <tt>trunc.is_active()</tt>.
				 * @throws unspecified any exception thrown by the <tt>compare_terms()</tt> method of the truncator object,
				 * in case of an active skipping truncator.
				 */
				explicit default_functor(term_type1 const **ptr1, const size_type &s1,
					term_type2 const **ptr2, const size_type &s2, const truncator_type &trunc,
					return_type &retval):
					m_ptr1(ptr1),m_s1(s1),m_ptr2(ptr2),m_s2(s2),m_trunc(trunc),m_retval(retval)
				{
					if (unlikely(IsActive != trunc.is_active())) {
						piranha_throw(std::invalid_argument,"inconsistent activity flags for truncator");
					}
					sort_for_skip(std::integral_constant<bool,IsActive && ttraits::is_skipping && SortOnConstruction>());
				}
				/// Check skipping condition.
				/**
				 * In case of an active skipping truncator, this method will return the output of the <tt>skip()</tt> method
				 * of the truncator called on the pointees of the i-th and j-th elements of the first and second array of
				 * term pointers respectively.
				 * 
				 * If the truncator is not active or not skipping, the method will return \p false.
				 * 
				 * @param[in] i index of the term in the first array of terms pointers.
				 * @param[in] j index of the term in the second array of terms pointers.
				 * 
				 * @return the output of the <tt>skip()</tt> method of the truncator in case of a skipping active truncator,
				 * \p false otherwise.
				 * 
				 * @throws unspecified any exception thrown by the <tt>skip()</tt> method of the truncator.
				 */
				bool skip(const size_type &i, const size_type &j) const
				{
					return skip_impl(i,j,std::integral_constant<bool,IsActive && ttraits::is_skipping>());
				}
				/// Check filtering condition.
				/**
				 * In case of an active filtering truncator, this method will return the output of the <tt>filter()</tt> method
				 * of the truncator on the input term.
				 * 
				 * If the truncator is not active or not filtering, the method will return \p false.
				 * 
				 * @param[in] t term to be checked.
				 * 
				 * @return the output of the <tt>filter()</tt> method of the truncator in case of a filtering active truncator,
				 * \p false otherwise.
				 * 
				 * @throws unspecified any exception thrown by the <tt>filter()</tt> method of the truncator.
				 */
				bool filter(const term_type1 &t) const
				{
					return filter_impl(t,std::integral_constant<bool,IsActive && ttraits::is_filtering>());
				}
				/// Term multiplication.
				/**
				 * This function call operator will multiply the i-th term in the first array of pointers by the j-th term
				 * in the second array of pointers, and store the result in the \p m_tmp member variable.
				 * 
				 * @param[in] i index of the first term operand.
				 * @param[in] j index of the second term operand.
				 * 
				 * @throws unspecified any exception thrown by the <tt>multiply()</tt> method of the first term type.
				 */
				void operator()(const size_type &i, const size_type &j) const
				{
					piranha_assert(i < m_s1 && j < m_s2);
					(m_ptr1[i])->multiply(m_tmp,*(m_ptr2[j]),m_retval.m_symbol_set);
				}
				/// Term insertion.
				/**
				 * This method will insert into the return value \p m_retval the term(s) stored in \p m_tmp. The boolean flag \p CheckFilter
				 * establishes whether the insertion respects the filtering criterion: if \p true, before any term insertion the filter()
				 * method of the functor will be called to check if the term can be discarded, otherwise the terms will be unconditionally
				 * inserted.
				 * 
				 * Note that the term will always be unconditionally inserted when the truncator is skipping, as it is assumed that all the filtering
				 * has been performed by the skip() method.
				 * 
				 * @throws unspecified any exception thrown by series::insert().
				 */
				template <bool CheckFilter = true>
				void insert() const
				{
					insert_impl<CheckFilter>(m_tmp);
				}
				/// Pointer to the first array of pointers.
				term_type1 const **					m_ptr1;
				/// Size of the first array of pointers.
				const size_type						m_s1;
				/// Pointer to the second array of pointers.
				term_type2 const **					m_ptr2;
				/// Size of the second array of pointers.
				const size_type						m_s2;
				/// Const reference to the truncator object used during construction.
				const truncator_type					&m_trunc;
				/// Reference to the return series object used during construction.
				return_type						&m_retval;
				/// Object holding the result of term-by-term multiplications.
				/**
				 * The type of this object is either \p term_type1 or a tuple of \p term_type1.
				 */
				mutable typename term_type1::multiplication_result_type	m_tmp;
		};
		/// Block-by-block multiplication.
		/**
		 * This method expects a \p Functor type exposing the same inteface as default_functor. Functionally, the method
		 * is equivalent to repeated calls of the methods of \p Functor that will multiply term-by-term the terms
		 * of the input series and accumulate the result in the output series. Terms will be inserted respecting the
		 * skipping and filtering criterions established by the active truncator.
		 *
		 * The method will perform the multiplications after logically subdividing the input series in blocks, in order to
		 * optimize cache memory access patterns.
		 * 
		 * @param[in] f multiplication functor.
		 * 
		 * @throws unspecified any exception thrown by the public interface of \p Functor.
		 */
		template <typename Functor>
		static void blocked_multiplication(const Functor &f)
		{
			typedef typename std::decay<decltype(f.m_s1)>::type size_type;
			// NOTE: hard-coded block size of 256.
			static_assert(boost::integer_traits<size_type>::const_max >= 256u, "Invalid size type.");
			const size_type size1 = f.m_s1, size2 = f.m_s2, bsize1 = 256u, nblocks1 = size1 / bsize1, bsize2 = bsize1, nblocks2 = size2 / bsize2;
			// Start and end of last (possibly irregular) blocks.
			const size_type i_ir_start = nblocks1 * bsize1, i_ir_end = size1;
			const size_type j_ir_start = nblocks2 * bsize2, j_ir_end = size2;
			for (size_type n1 = 0u; n1 < nblocks1; ++n1) {
				const size_type i_start = n1 * bsize1, i_end = i_start + bsize1;
				// regulars1 * regulars2
				for (size_type n2 = 0u; n2 < nblocks2; ++n2) {
					const size_type j_start = n2 * bsize2, j_end = j_start + bsize2;
					for (size_type i = i_start; i < i_end; ++i) {
						for (size_type j = j_start; j < j_end; ++j) {
							if (f.skip(i,j)) {
								break;
							}
							f(i,j);
							f.insert();
						}
					}
				}
				// regulars1 * rem2
				for (size_type i = i_start; i < i_end; ++i) {
					for (size_type j = j_ir_start; j < j_ir_end; ++j) {
						if (f.skip(i,j)) {
							break;
						}
						f(i,j);
						f.insert();
					}
				}
			}
			// rem1 * regulars2
			for (size_type n2 = 0u; n2 < nblocks2; ++n2) {
				const size_type j_start = n2 * bsize2, j_end = j_start + bsize2;
				for (size_type i = i_ir_start; i < i_ir_end; ++i) {
					for (size_type j = j_start; j < j_end; ++j) {
						if (f.skip(i,j)) {
							break;
						}
						f(i,j);
						f.insert();
					}
				}
			}
			// rem1 * rem2.
			for (size_type i = i_ir_start; i < i_ir_end; ++i) {
				for (size_type j = j_ir_start; j < j_ir_end; ++j) {
					if (f.skip(i,j)) {
						break;
					}
					f(i,j);
					f.insert();
				}
			}
		}
		/// Estimate size of series multiplication.
		/**
		 * This method expects a \p Functor type exposing the same inteface as default_functor. The method
		 * will employ a statistical approach to estimate the size of the output of the series multiplication represented
		 * by \p f (without actually going through the whole calculation).
		 * 
		 * If either input series has a null size, zero will be returned.
		 * 
		 * @param[in] f multiplication functor.
		 * 
		 * @return the estimated size of the series multiplication represented by \p f.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - memory allocation errors in standard containers,
		 * - overflow errors while converting between integer types,
		 * - the public interface of \p Functor.
		 */
		template <typename Functor>
		static typename Series1::size_type estimate_final_series_size(const Functor &f)
		{
			typedef typename Series1::size_type bucket_size_type;
			typedef typename std::decay<decltype(f.m_s1)>::type size_type;
			const size_type size1 = f.m_s1, size2 = f.m_s2;
			// If one of the two series is empty, just return 0.
			if (unlikely(!size1 || !size2)) {
				return 0u;
			}
			// Cache reference to return series.
			auto &retval = f.m_retval;
			// NOTE: Hard-coded number of trials = 10.
			// NOTE: here consider that in case of extremely sparse series with few terms (e.g., next to the lower limit
			// for which this function is called) this will incur in noticeable overhead, since we will need many term-by-term
			// before encountering the first duplicate.
			const auto ntrials = 10u;
			// NOTE: Hard-coded value for the estimation multiplier.
			// NOTE: This value should be tuned for performance/memory usage tradeoffs.
			const auto multiplier = 2;
			// Size of the multiplication result
			// Vectors of indices.
			std::vector<size_type> v_idx1, v_idx2;
			for (size_type i = 0u; i < size1; ++i) {
				v_idx1.push_back(i);
			}
			for (size_type i = 0u; i < size2; ++i) {
				v_idx2.push_back(i);
			}
			// Maxium number of random multiplications before which a duplicate term must be generated.
			const size_type max_M = static_cast<size_type>(((integer(size1) * size2) / multiplier).sqrt());
			// Random number engine and generator. We need something more than vanilla random_shuffle as we
			// want to have a different engine per thread, in order to avoid potential problems with vanilla shuffle
			// sharing and syncing a state across threads.
			std::mt19937 engine;
			typedef typename std::iterator_traits<typename std::vector<size_type>::iterator>::difference_type diff_type;
			typedef std::uniform_int_distribution<diff_type> dist_type;
			auto rng = [&engine](const diff_type &n) {return dist_type(diff_type(0),n - diff_type(1))(engine);};
			// Init counters.
			integer total(0), filtered(0);
			// Go with the trials.
			// NOTE: This could be easily parallelised, but not sure if it is worth.
			for (auto n = 0u; n < ntrials; ++n) {
				// Randomise.
				std::random_shuffle(v_idx1.begin(),v_idx1.end(),rng);
				std::random_shuffle(v_idx2.begin(),v_idx2.end(),rng);
				size_type count = 0u, count_filtered = 0u;
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
					// Perform multiplication and check if it produces a new term.
					f(*it1,*it2);
					// Do not filter on insertion, we are going to check how many terms would be filtered later.
					f.template insert<false>();
					// Check for unlikely overflow when increasing count.
					if (unlikely(result_size(f.m_tmp) > boost::integer_traits<size_type>::const_max ||
						count > boost::integer_traits<size_type>::const_max - result_size(f.m_tmp)))
					{
						piranha_throw(std::overflow_error,"overflow error");
					}
					if (retval.size() != count + result_size(f.m_tmp)) {
						break;
					}
					// Check how many terms would be filtered.
					const std::size_t n_filtered = count_n_filtered(f.m_tmp,f);
					// Check for unlikely overflow, as above.
					if (unlikely(n_filtered > boost::integer_traits<size_type>::const_max ||
						count_filtered > boost::integer_traits<size_type>::const_max - n_filtered))
					{
						piranha_throw(std::overflow_error,"overflow error");
					}
					count_filtered += n_filtered;
					// Increase cycle variables.
					count += result_size(f.m_tmp);
					++it1;
					++it2;
				}
				total += count;
				filtered += count_filtered;
				// Reset retval.
				retval.m_container.clear();
			}
			piranha_assert(total >= filtered);
			const auto mean = total / ntrials;
			// Avoid division by zero.
			if (total.sign()) {
				const integer M = (mean * mean * multiplier * (total - filtered)) / total;
				return static_cast<bucket_size_type>(M);
			} else {
				return static_cast<bucket_size_type>(mean * mean * multiplier);
			}
		}
		/// Trace series size estimates.
		/**
		 * Record in the piranha::tracing framework the outcome of result size estimation via estimate_final_series_size().
		 * 
		 * The string descriptors and associated data are:
		 * 
		 * - <tt>number_of_estimates</tt>, <tt>unsigned long long</tt>, corresponding to the total number of times
		 *   this function has been called;
		 * - <tt>number_of_correct_estimates</tt>, <tt>unsigned long long</tt>, corresponding to the total number of times
		 *   this function has been called with <tt>estimate >= real_size</tt>;
		 * - <tt>accumulated_estimate_ratio</tt>, <tt>double</tt>, corresponding to the accumulated value of <tt>estimate / real_size</tt>.
		 * 
		 * In order to avoid potential divisions by zero, if \p real_size is zero no tracing will be performed.
		 * 
		 * @param[in] real_size the real size of the output series.
		 * @param[in] estimate the size of the output series as estimated by estimate_final_series_size().
		 * 
		 * @throws unspecified any exception thrown by tracing::trace().
		 */
		static void trace_estimates(const typename Series1::size_type &real_size, const typename Series1::size_type &estimate)
		{
			if (unlikely(!real_size)) {
				return;
			}
			tracing::trace("number_of_estimates",[](boost::any &x) -> void {
				if (unlikely(x.empty())) {
					x = 0ull;
				}
				auto ptr = boost::any_cast<unsigned long long>(&x);
				if (likely((bool)ptr)) {
					++*ptr;
				}
			});
			tracing::trace("number_of_correct_estimates",[real_size,estimate](boost::any &x) -> void {
				if (unlikely(x.empty())) {
					x = 0ull;
				}
				auto ptr = boost::any_cast<unsigned long long>(&x);
				if (likely((bool)ptr)) {
					*ptr += static_cast<unsigned long long>(estimate >= real_size);
				}
			});
			tracing::trace("accumulated_estimate_ratio",[real_size,estimate](boost::any &x) -> void {
				if (unlikely(x.empty())) {
					x = 0.;
				}
				auto ptr = boost::any_cast<double>(&x);
				if (likely((bool)ptr && estimate)) {
					*ptr += static_cast<double>(estimate) / real_size;
				}
			});
		}
	private:
		template <typename RetvalList, typename Size>
		static void final_merge(return_type &retval, RetvalList &retval_list, const Size &n_threads)
		{
			piranha_assert(n_threads > 1u);
			piranha_assert(retval.m_container.bucket_count());
			typedef typename std::vector<std::pair<typename Series1::size_type,decltype(retval.m_container._m_begin())>> container_type;
			typedef typename container_type::size_type size_type;
			// First, let's fill a vector assigning each term of each element in retval_list to a bucket in retval.
			const size_type size = static_cast<size_type>(
				std::accumulate(retval_list.begin(),retval_list.end(),integer(0),[](const integer &n, const return_type &r) {return n + r.size();}));
			container_type idx(size);
			task_group tg1;
			size_type i = 0u;
			piranha_assert(retval_list.size() <= n_threads);
			try {
				for (auto r_it = retval_list.begin(); r_it != retval_list.end(); ++r_it) {
					auto f = [&idx,&retval,i,r_it]() {
						thread_management::binder b;
						const auto it_f = r_it->m_container._m_end();
						// NOTE: size_type can represent the sum of the sizes of all retvals,
						// so it will not overflow here.
						size_type tmp_i = i;
						for (auto it = r_it->m_container._m_begin(); it != it_f; ++it, ++tmp_i) {
							piranha_assert(tmp_i < idx.size());
							idx[tmp_i] = std::make_pair(retval.m_container._bucket(*it),it);
						}
					};
					tg1.add_task(f);
					i += r_it->size();
				}
				tg1.wait_all();
				tg1.get_all();
			} catch (...) {
				tg1.wait_all();
				throw;
			}
			task_group tg2;
			mutex m;
			std::vector<integer> new_sizes;
			new_sizes.reserve(n_threads);
			if (unlikely(new_sizes.capacity() != n_threads)) {
				piranha_throw(std::bad_alloc,);
			}
			const typename Series1::size_type bucket_count = retval.m_container.bucket_count(), block_size = bucket_count / n_threads;
			try {
				for (Size n = 0u; n < n_threads; ++n) {
					// These are the bucket indices allowed to the current thread.
					const typename Series1::size_type start = n * block_size, end = (n == n_threads - 1u) ? bucket_count : (n + 1u) * block_size;
					auto f = [start,end,&size,&retval,&idx,&m,&new_sizes]() {
						thread_management::binder b;
						size_type count_plus = 0u, count_minus = 0u;
						for (size_type i = 0u; i < size; ++i) {
							const auto &bucket_idx = idx[i].first;
							auto &term_it = idx[i].second;
							if (bucket_idx >= start && bucket_idx < end) {
								// Reconstruct part of series insertion, without the update in the number of elements.
								// NOTE: compatibility and ignorability do not matter, as terms being inserted come from series
								// anyway. Same for check on bucket_count.
								const auto it = retval.m_container._find(*term_it,bucket_idx);
								if (it == retval.m_container.end()) {
									// Term is new.
									retval.m_container._unique_insert(std::move(*term_it),bucket_idx);
									// Update term count.
									piranha_assert(count_plus < boost::integer_traits<size_type>::const_max);
									++count_plus;
								} else {
									// Assert the existing term is not ignorable and it is compatible.
									piranha_assert(!it->is_ignorable(retval.m_symbol_set) && it->is_compatible(retval.m_symbol_set));
									// Cleanup function.
									auto cleanup = [&]() -> void {
										if (unlikely(!it->is_compatible(retval.m_symbol_set) || it->is_ignorable(retval.m_symbol_set))) {
											retval.m_container._erase(it);
											// After term is erased, update count.
											piranha_assert(count_minus < boost::integer_traits<size_type>::const_max);
											++count_minus;
										}
									};
									try {
										// The term exists already, update it.
										retval.template insertion_cf_arithmetics<true>(it,std::move(*term_it));
										// Check if the term has become ignorable or incompatible after the modification.
										cleanup();
									} catch (...) {
										cleanup();
										throw;
									}
								}
							}
						}
						// Store the new size.
						lock_guard<mutex>::type lock(m);
						new_sizes.push_back(integer(count_plus) - integer(count_minus));
					};
					tg2.add_task(f);
				}
				piranha_assert(tg2.size() == n_threads);
				tg2.wait_all();
				tg2.get_all();
			} catch (...) {
				tg2.wait_all();
				throw;
			}
			// Final update of retval's size.
			const auto new_size = std::accumulate(new_sizes.begin(),new_sizes.end(),integer(0));
			piranha_assert(new_size + retval.m_container.size() >= 0);
			retval.m_container._update_size(static_cast<decltype(retval.m_container.size())>(new_size + retval.m_container.size()));
			// Take care of rehashing, if needed.
			if (unlikely(retval.m_container.load_factor() > retval.m_container.max_load_factor())) {
				retval.m_container.rehash(
					boost::numeric_cast<typename Series1::size_type>(std::ceil(retval.size() / retval.m_container.max_load_factor()))
				);
			}
		}
		// Functor tasked to prepare return value(s) with estimated bucket sizes (if
		// it is worth to perform such analysis).
		template <typename Functor>
		static std::pair<bool,typename Series1::size_type> rehasher(const Functor &f)
		{
			const auto s1 = f.m_s1, s2 = f.m_s2;
			auto &r = f.m_retval;
			// NOTE: hard-coded value of 100000 for minimm number of terms multiplications.
			if (s2 && s1 >= 100000u / s2) {
				// NOTE: here we could have (very unlikely) some overflow or memory error in the computation
				// of the estimate or in rehashing. In such a case, just ignore the rehashing, clean
				// up retval just to be sure, and proceed.
				try {
					auto size = estimate_final_series_size(f);
					r.m_container.rehash(boost::numeric_cast<decltype(size)>(std::ceil(size / r.m_container.max_load_factor())));
					return std::make_pair(true,size);
				} catch (...) {
					r.m_container.clear();
					return std::make_pair(false,typename Series1::size_type(0u));
				}
			}
			return std::make_pair(false,typename Series1::size_type(0u));
		}
		// Size of the result of term multiplication.
		template <typename T>
		static std::size_t result_size(const T &)
		{
			return 1;
		}
		template <typename... T>
		static std::size_t result_size(const std::tuple<T...> &)
		{
			return std::tuple_size<std::tuple<T...>>::value;
		}
		// Meta-programming for counting the filtered terms in f.m_tmp.
		template <typename Functor>
		static std::size_t count_n_filtered(const term_type1 &t, const Functor &f)
		{
			return f.filter(t);
		}
		template <std::size_t N = 0u, typename Functor, typename... T>
		static std::size_t count_n_filtered(const std::tuple<T...> &t, const Functor &f,
			typename std::enable_if<(N != std::tuple_size<std::tuple<T...>>::value - 1u)>::type * = nullptr)
		{
			static_assert(N < boost::integer_traits<std::size_t>::const_max,"Overflow error.");
			return ((std::size_t)f.filter(std::get<N>(t))) + count_n_filtered<N + 1u>(t,f);
		}
		template <std::size_t N, typename Functor, typename... T>
		static std::size_t count_n_filtered(const std::tuple<T...> &t, const Functor &f,
			typename std::enable_if<N == std::tuple_size<std::tuple<T...>>::value - 1u>::type * = nullptr)
		{
			return f.filter(std::get<N>(t));
		}
	protected:
		/// Const pointer to the first series operand.
		Series1 const				*m_s1;
		/// Const pointer to the second series operand.
		Series2 const				*m_s2;
		/// Vector of const pointers to the terms in the first series.
		mutable std::vector<term_type1 const *>	m_v1;
		/// Vector of const pointers to the terms in the second series.
		mutable std::vector<term_type2 const *>	m_v2;
};

}

#endif
