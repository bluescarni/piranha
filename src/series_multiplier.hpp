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
#include "concepts/multipliable_term.hpp"
#include "concepts/series.hpp"
#include "config.hpp"
#include "detail/series_fwd.hpp"
#include "detail/series_multiplier_fwd.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "thread_group.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "tracing.hpp"

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
 *   \p Series1 and \p Series2 must be the same, otherwise a compile-time assertion will fail;
 * - the term type of \p Series1 must be a model of piranha::concept::MultipliableTerm.
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
 * means via template specialisation to customise the behaviour for different types of coefficients.
 * \todo when estimating series size, we should take into account that term-by-term multiplication can generate multiple terms (e.g., Poisson series).
 * \todo swap series if they are the same type and if first one is smaller than second -> more opportunity for subdivision in mt mode
 * \todo try to abstract exception handling in mt-mode into a generic functor that re-throws exceptions transported from threads.
 * \todo optimization in case one series has 1 term with unitary key and both series same type: multiply directly coefficients.
 */
template <typename Series1, typename Series2, typename Enable = void>
class series_multiplier
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series1>));
		BOOST_CONCEPT_ASSERT((concept::Series<Series2>));
		BOOST_CONCEPT_ASSERT((concept::MultipliableTerm<typename Series1::term_type>));
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
	private:
		static_assert(std::is_same<return_type,decltype(std::declval<Series1>().multiply_by_series(std::declval<Series1>()))>::value,
			"Invalid return_type.");
	public:
		/// Constructor.
		/**
		 * @param[in] s1 first series.
		 * @param[in] s2 second series.
		 * 
		 * @throws std::invalid_argument if the symbol sets of \p s1 and \p s2 differ.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers.
		 */
		explicit series_multiplier(const Series1 &s1, const Series2 &s2) : m_s1(s1),m_s2(s2)
		{
			if (unlikely(s1.m_symbol_set != s2.m_symbol_set)) {
				piranha_throw(std::invalid_argument,"incompatible arguments sets");
			}
			if (unlikely(m_s1.empty() || m_s2.empty())) {
				return;
			}
			m_v1.reserve(m_s1.size());
			m_v2.reserve(m_s2.size());
			// Fill in the vectors of pointers.
			std::back_insert_iterator<decltype(m_v1)> bii1(m_v1);
			std::transform(m_s1.m_container.begin(),m_s1.m_container.end(),bii1,[](const term_type1 &t) {return &t;});
			std::back_insert_iterator<decltype(m_v2)> bii2(m_v2);
			std::transform(m_s2.m_container.begin(),m_s2.m_container.end(),bii2,[](const term_type2 &t) {return &t;});
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
		 * This method will call execute() with a \p Functor type that uses the <tt>multiply()</tt> method
		 * of the first series' term type to produce the results of term-by-term multiplications, and
		 * piranha::series::insert() to insert such results into the return value.
		 * 
		 * @return the result of multiplying the two series.
		 * 
		 * @throws unspecified any exception thrown by execute().
		 */
		return_type operator()() const
		{
			return execute<plain_functor>();
		}
	protected:
		/// Low-level implementation of series multiplication.
		/**
		 * The multiplication algorithm proceeds as follows:
		 * 
		 * - if one of the two series is empty, a default-constructed instance of \p return_type is returned;
		 * - a heuristic determines whether to enable multi-threaded mode or not;
		 * - in single-threaded mode:
		 *   - an instance of \p Functor is created and its <tt>operator()</tt> is run iteratively over all the terms
		 *     of the two series to construct the return value;
		 * - in multi-threaded mode:
		 *   - the first series is subdivided into segments and the same process described for single-threaded mode is run in parallel,
		 *     storing the multiple resulting series in a list;
		 *   - the series in the result list are merged into a single series via piranha::series::insert().
		 * 
		 * The protocol expected by an instance of type \p Functor is the following:
		 * 
		 * - it must be constructible from a 5-ary constructor from:
		 *   - a pointer to an array of const pointers to terms in the first input series, and the array's size;
		 *   - the same quantities for the second series,
		 *   - an instance of type \p return_type that will be used to accumulate the terms during multiplication;
		 * - it must be provided with an <tt>operator()()</tt>, taking two unsigned integers (e.g., \p i and \p j) as input
		 *   parameters and returning void. A call of this operator will multiply the <tt>i</tt>-th term in the first array
		 *   by the <tt>j</tt>-th term in the second array,
		 *   and insert the result into the the instance of \p return_type used for construction.
		 * 
		 * Note that the parameters passed to the constructor exist outside the \p Functor. In particular,
		 * the \p return_type instance used for construction will then be used to create the return value (and
		 * thus \p Functor is expected to use a reference or a pointer to such object in its operations).
		 * Instances of \p Functor are created sequentially (when operating in multi-threaded mode), and they are
		 * allowed to mutate the arrays of terms pointers (in particular, they are allowed to reorder them).
		 * 
		 * @return the result of multiplying the first series by the second series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - memory allocation errors in standard containers,
		 * - \p boost::numeric_cast (in case of out-of-range convertions from one integral type to another),
		 * - the cast operator of piranha::integer to integral types,
		 * - the constructor and call operator of \p Functor,
		 * - errors in threading primitives,
		 * - piranha::series::insert().
		 */
		template <typename Functor>
		return_type execute() const
		{
			// Do not do anything if one of the two series is empty.
			if (unlikely(m_s1.empty() || m_s2.empty())) {
				return return_type{};
			}
			// This is the size type that will be used throughout the calculations.
			typedef decltype(m_v1.size()) size_type;
			const auto size1 = m_v1.size(), size2 = m_v2.size();
			piranha_assert(size1 && size2);
			// Establish the number of threads to use.
			auto n_threads = boost::numeric_cast<size_type>(settings::get_n_threads());
			piranha_assert(n_threads >= 1u);
			// Make sure that each thread has a minimum amount of work to do, otherwise reduce thread number.
			if (n_threads != 1u) {
				// NOTE: hard-coded minimum work of 100000 term-by-terms per thread.
				const auto min_work = 100000u;
				const auto work_size = integer(size1) * size2;
				n_threads = (work_size / n_threads >= min_work) ? n_threads :
					static_cast<size_type>(std::max<integer>(integer(1),work_size / min_work));
			}
			// Check we did not actually increase the number of threads...
			piranha_assert(n_threads <= settings::get_n_threads());
			// Final check on n_threads is that its size is not greater than the size of the first series,
			// as we are using the first operand to split up the work.
			if (n_threads > size1) {
				n_threads = size1;
			}
			// Go single-thread if heurisitcs says so or if we are already in a different thread from the main one.
			// NOTE: the check on the thread id might go above, to avoid allocating memory when using integers.
			if (likely(n_threads == 1u || runtime_info::get_main_thread_id() != this_thread::get_id())) {
				return_type retval;
				retval.m_symbol_set = m_s1.m_symbol_set;
				Functor f(&m_v1[0u],size1,&m_v2[0u],size2,retval);
				const auto tmp = rehasher(f,retval,size1,size2);
				blocked_multiplication(f,size1,size2);
				if (tmp.first) {
					trace_estimates(retval.size(),tmp.second);
				}
				return retval;
			} else {
// boost::posix_time::ptime time0 = boost::posix_time::microsec_clock::local_time();
				// Tools to handle exceptions thrown in the threads.
				std::vector<exception_ptr> exceptions;
				exceptions.reserve(n_threads);
				if (exceptions.capacity() != n_threads) {
					piranha_throw(std::bad_alloc,0);
				}
				mutex exceptions_mutex;
				// Build the return values and the multiplication functors.
				std::list<return_type,cache_aligning_allocator<return_type>> retval_list;
				std::list<Functor,cache_aligning_allocator<Functor>> functor_list;
				const auto block_size = size1 / n_threads;
				for (size_type i = 0u; i < n_threads; ++i) {
					// Last thread needs a different size from block_size.
					const size_type s1 = (i == n_threads - 1u) ? (size1 - i * block_size) : block_size;
					retval_list.push_back(return_type{});
					retval_list.back().m_symbol_set = m_s1.m_symbol_set;
					functor_list.push_back(Functor(&m_v1[0u] + i * block_size,s1,&m_v2[0u],size2,retval_list.back()));
				}
				thread_group tg;
				auto f_it = functor_list.begin();
				auto r_it = retval_list.begin();
				for (size_type i = 0u; i < n_threads; ++i, ++f_it, ++r_it) {
					// Last thread needs a different size from block_size.
					const size_type s1 = (i == n_threads - 1u) ? (size1 - i * block_size) : block_size;
					// Functor for use in the thread.
					// NOTE: here we need to pass in and use this for the static methods (instead of using them directly)
					// because of a GCC bug in 4.6.
					auto f = [f_it,r_it,i,block_size,s1,size2,&exceptions,&exceptions_mutex,this]() -> void {
						try {
							thread_management::binder binder;
							const auto tmp = this->rehasher(*f_it,*r_it,s1,size2);
// std::cout << "bsize : " << r_it->m_container.bucket_count() << '\n';
							this->blocked_multiplication(*f_it,s1,size2);
							if (tmp.first) {
								this->trace_estimates(r_it->m_container.size(),tmp.second);
							}
// std::cout << "final series size: " << f_it->m_retval.size() << '\n';
						} catch (...) {
							lock_guard<mutex>::type lock(exceptions_mutex);
							exceptions.push_back(current_exception());
						}
					};
					tg.create_thread(f);
				}
				tg.join_all();
				if (unlikely(exceptions.size())) {
					// Re-throw the first exception found.
					piranha::rethrow_exception(exceptions[0u]);
				}
// std::cout << "Elapsed time for multimul: " << (double)(boost::posix_time::microsec_clock::local_time() - time0).total_microseconds() / 1000 << '\n';
				return_type retval;
				retval.m_symbol_set = m_s1.m_symbol_set;
				auto final_estimate = estimate_final_series_size(Functor(&m_v1[0],size1,&m_v2[0],size2,retval),size1,size2,retval);
				// We want to make sure that final_estimate contains at least 1 element, so that we can use faster low-level
				// methods in hash_set.
				if (unlikely(!final_estimate)) {
					final_estimate = 1u;
				}
// std::cout << "Final estimate: " <<  final_estimate << '\n';
				// Try to see if a series already has enough buckets.
				auto it = std::find_if(retval_list.begin(),retval_list.end(),[&final_estimate](const return_type &r) {
					return r.m_container.bucket_count() * r.m_container.max_load_factor() >= final_estimate;
				});
				if (it != retval_list.end()) {
// std::cout << "Found candidate series for final summation\n";
					retval = std::move(*it);
					retval_list.erase(it);
				} else {
					// Otherwise, just rehash to the desired value.
					retval.m_container.rehash(final_estimate);
// std::cout << "After final estimate: " << retval.m_container.bucket_count() << '\n';
				}
// time0 = boost::posix_time::microsec_clock::local_time();
				// Cleanup functor that will erase all elements in retval_list.
				auto cleanup = [&retval_list]() -> void {
					std::for_each(retval_list.begin(),retval_list.end(),[](return_type &r) -> void {r.m_container.clear();});
				};
std::cout << "going for final\n";
				try {
					final_merge(retval,retval_list,n_threads);
					// Here the estimate will be correct if the initial estimate (corrected for load factor)
					// is at least equal to the final real bucket count.
std::cout << "final: " << retval.m_container.size() << " vs " << final_estimate << '\n';
					trace_estimates(retval.m_container.size(),final_estimate);
				} catch (...) {
					// Do the cleanup before re-throwing, as we use mutable iterators in final_merge.
					cleanup();
					// Clear also retval: it could have bogus number of elements if errors arise in the final merge.
					retval.m_container.clear();
					throw;
				}
// std::cout << "Elapsed time for summation: " << (double)(boost::posix_time::microsec_clock::local_time() - time0).total_microseconds() / 1000 << '\n';
				// Clean up the temporary series.
				cleanup();
				return retval;
			}
		}
	private:
		typedef decltype(std::declval<return_type>().m_container.bucket_count()) bucket_size_type;
		static void trace_estimates(const bucket_size_type &real_size, const bucket_size_type &estimate)
		{
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
		template <typename RetvalList, typename Size>
		static void final_merge(return_type &retval, RetvalList &retval_list, const Size &n_threads)
		{
			piranha_assert(n_threads > 1u);
			piranha_assert(retval.m_container.bucket_count());
			// Misc for exception handling.
			mutex e_mutex;
			std::vector<exception_ptr> e_vector;
			// We never need more capacity than the number of threads.
			e_vector.reserve(n_threads);
			if (unlikely(e_vector.capacity() != n_threads)) {
				piranha_throw(std::bad_alloc,0);
			}
			typedef typename std::vector<std::pair<bucket_size_type,decltype(retval.m_container._m_begin())>> container_type;
			typedef typename container_type::size_type size_type;
			// First, let's fill a vector assigning each term of each element in retval_list to a bucket in retval.
			const size_type size = static_cast<size_type>(
				std::accumulate(retval_list.begin(),retval_list.end(),integer(0),[](const integer &n, const return_type &r) {return n + r.size();}));
			container_type idx(size);
			thread_group tg1;
			size_type i = 0u;
			piranha_assert(retval_list.size() <= n_threads);
			for (auto r_it = retval_list.begin(); r_it != retval_list.end(); ++r_it) {
				auto f = [&idx,&retval,&e_mutex,&e_vector,i,r_it]() -> void {
					try {
						thread_management::binder b;
						const auto it_f = r_it->m_container._m_end();
						auto tmp_i = i;
						for (auto it = r_it->m_container._m_begin(); it != it_f; ++it, ++tmp_i) {
							piranha_assert(tmp_i < idx.size());
							idx[tmp_i] = std::make_pair(retval.m_container._bucket(*it),it);
						}
					} catch (...) {
						lock_guard<mutex>::type lock(e_mutex);
						e_vector.push_back(current_exception());
					}
				};
				tg1.create_thread(f);
				i += r_it->size();
			}
			tg1.join_all();
			if (unlikely(e_vector.size())) {
				piranha::rethrow_exception(e_vector[0u]);
			}
			// Reset exception vector.
			e_vector.clear();
			e_vector.reserve(n_threads);
			if (unlikely(e_vector.capacity() != n_threads)) {
				piranha_throw(std::bad_alloc,0);
			}
			thread_group tg2;
			mutex m;
			std::vector<integer> new_sizes;
			new_sizes.reserve(n_threads);
			if (unlikely(new_sizes.capacity() != n_threads)) {
				piranha_throw(std::bad_alloc,0);
			}
			const bucket_size_type bucket_count = retval.m_container.bucket_count(), block_size = bucket_count / n_threads;
			for (Size n = 0u; n < n_threads; ++n) {
				// These are the bucket indices allowed to the current thread.
				const bucket_size_type start = n * block_size, end = (n == n_threads - 1u) ? bucket_count : (n + 1u) * block_size;
				auto f = [start,end,&size,&retval,&idx,&m,&new_sizes,&e_mutex,&e_vector]() -> void {
					try {
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
					} catch (...) {
						lock_guard<mutex>::type lock(e_mutex);
						e_vector.push_back(current_exception());
					}
				};
				tg2.create_thread(f);
			}
			tg2.join_all();
			if (unlikely(e_vector.size())) {
				piranha::rethrow_exception(e_vector[0u]);
			}
			// Final update of retval's size.
			const auto new_size = std::accumulate(new_sizes.begin(),new_sizes.end(),integer(0));
std::cout << "new size is " << new_size << '\n';
			piranha_assert(new_size + retval.m_container.size() >= 0);
			retval.m_container._update_size(static_cast<decltype(retval.m_container.size())>(new_size + retval.m_container.size()));
			// Take care of rehashing, if needed.
			if (unlikely(retval.m_container.load_factor() > retval.m_container.max_load_factor())) {
				retval.m_container.rehash(
					boost::numeric_cast<bucket_size_type>(std::ceil(retval.size() / retval.m_container.max_load_factor()))
				);
			}
		}
		struct plain_functor
		{
			typedef typename std::vector<term_type1 const *>::size_type size_type;
			explicit plain_functor(term_type1 const **ptr1, const size_type &s1,
				term_type2 const **ptr2, const size_type &s2,
				return_type &retval):
				m_ptr1(ptr1),m_s1(s1),m_ptr2(ptr2),m_s2(s2),m_retval(retval)
			{}
			void operator()(const size_type &i, const size_type &j) const
			{
				piranha_assert(i < m_s1 && j < m_s2);
				(m_ptr1[i])->multiply(m_tmp,*(m_ptr2[j]),m_retval.m_symbol_set);
				series_multiplier::insert_impl(m_retval,m_tmp);
			}
			term_type1 const **					m_ptr1;
			const size_type						m_s1;
			term_type2 const **					m_ptr2;
			const size_type						m_s2;
			return_type						&m_retval;
			mutable typename term_type1::multiplication_result_type	m_tmp;
		};
		// Functor tasked to prepare return value(s) with estimated bucket sizes (if
		// it is worth to perform such analysis).
		template <typename Functor, typename Size>
		static std::pair<bool,bucket_size_type> rehasher(const Functor &f, return_type &r, const Size &s1, const Size &s2)
		{
			// NOTE: hard-coded value of 100000 for minimm number of terms multiplications.
			if (s2 && s1 >= 100000u / s2) {
				// NOTE: here we could have (very unlikely) some overflow or memory error in the computation
				// of the estimate or in rehashing. In such a case, just ignore the rehashing, clean
				// up retval just to be sure, and proceed.
				try {
					auto size = estimate_final_series_size(f,s1,s2,r);
					r.m_container.rehash(boost::numeric_cast<decltype(size)>(std::ceil(size / r.m_container.max_load_factor())));
					return std::make_pair(true,size);
				} catch (...) {
					r.m_container.clear();
					return std::make_pair(false,bucket_size_type(0u));
				}
			}
			return std::make_pair(false,bucket_size_type(0u));
		}
		template <typename Functor, typename Size>
		static bucket_size_type estimate_final_series_size(const Functor &f, const Size &size1, const Size &size2, return_type &retval)
		{
			// If one of the two series is empty, just return 0.
			if (unlikely(!size1 || !size2)) {
				return 0u;
			}
			// NOTE: Hard-coded number of trials = 10.
			// NOTE: here consider that in case of extremely sparse series with few terms (e.g., next to the lower limit
			// for which this function is called) this will incur in noticeable overhead, since we will need many term-by-term
			// before encountering the first duplicate.
			const auto ntrials = 10u;
			// NOTE: Hard-coded value for the estimation multiplier.
			// NOTE: This value should be tuned for performance/memory usage tradeoffs.
			const auto multiplier = 4;
			// Size of the multiplication result
			// Vectors of indices.
			std::vector<Size> v_idx1, v_idx2;
			for (Size i = 0u; i < size1; ++i) {
				v_idx1.push_back(i);
			}
			for (Size i = 0u; i < size2; ++i) {
				v_idx2.push_back(i);
			}
			// Maxium number of random multiplications before which a duplicate term must be generated.
			const Size max_M = static_cast<Size>(((integer(size1) * size2) / multiplier).sqrt());
			// Random number engine and generator.
			std::mt19937 engine;
			typedef typename std::iterator_traits<typename std::vector<Size>::iterator>::difference_type diff_type;
			typedef std::uniform_int_distribution<diff_type> dist_type;
			auto rng = [&engine](const diff_type &n) {return dist_type(diff_type(0),n - diff_type(1))(engine);};
			// Init mean.
			integer mean(0);
			// Go with the trials.
			// NOTE: This could be easily parallelised, but not sure if it is worth.
			for (auto n = 0u; n < ntrials; ++n) {
				// Randomise.
				std::random_shuffle(v_idx1.begin(),v_idx1.end(),rng);
				std::random_shuffle(v_idx2.begin(),v_idx2.end(),rng);
				Size count = 0u;
				auto it1 = v_idx1.begin(), it2 = v_idx2.begin();
				while (count < max_M) {
					if (it1 == v_idx1.end()) {
						// Each time we wrap around the first series,
						// wrap around also the second and rotate it.
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
					if (retval.size() != count + 1u) {
						break;
					}
					// Increase cycle variables.
					++count;
					++it1;
					++it2;
				}
				mean += count;
				// Reset retval.
				retval.m_container.clear();
			}
			mean /= ntrials;
			const integer M = mean * mean * multiplier;
			return static_cast<bucket_size_type>(M);
		}
		template <typename Functor, typename Size>
		static void blocked_multiplication(const Functor &f, const Size &size1, const Size &size2)
		{
			// NOTE: hard-coded block size of 256.
			static_assert(std::is_unsigned<Size>::value && boost::integer_traits<Size>::const_max >= 256u, "Invalid size type.");
			const Size bsize1 = 256u, nblocks1 = size1 / bsize1, bsize2 = bsize1, nblocks2 = size2 / bsize2;
			// Start and end of last (possibly irregular) blocks.
			const Size i_ir_start = nblocks1 * bsize1, i_ir_end = size1;
			const Size j_ir_start = nblocks2 * bsize2, j_ir_end = size2;
			for (Size n1 = 0u; n1 < nblocks1; ++n1) {
				const Size i_start = n1 * bsize1, i_end = i_start + bsize1;
				// regulars1 * regulars2
				for (Size n2 = 0u; n2 < nblocks2; ++n2) {
					const Size j_start = n2 * bsize2, j_end = j_start + bsize2;
					for (Size i = i_start; i < i_end; ++i) {
						for (Size j = j_start; j < j_end; ++j) {
							f(i,j);
						}
					}
				}
				// regulars1 * rem2
				for (Size i = i_start; i < i_end; ++i) {
					for (Size j = j_ir_start; j < j_ir_end; ++j) {
						f(i,j);
					}
				}
			}
			// rem1 * regulars2
			for (Size n2 = 0u; n2 < nblocks2; ++n2) {
				const Size j_start = n2 * bsize2, j_end = j_start + bsize2;
				for (Size i = i_ir_start; i < i_ir_end; ++i) {
					for (Size j = j_start; j < j_end; ++j) {
						f(i,j);
					}
				}
			}
			// rem1 * rem2.
			for (Size i = i_ir_start; i < i_ir_end; ++i) {
				for (Size j = j_ir_start; j < j_ir_end; ++j) {
					f(i,j);
				}
			}
		}
		template <typename Tuple, std::size_t N = 0, typename Enable2 = void>
		struct inserter
		{
			static void run(Tuple &t, return_type &retval)
			{
				retval.insert(std::get<N>(t));
				inserter<Tuple,N + static_cast<std::size_t>(1)>::run(t,retval);
			}
		};
		template <typename Tuple, std::size_t N>
		struct inserter<Tuple,N,typename std::enable_if<N == std::tuple_size<Tuple>::value>::type>
		{
			template <typename Term>
			static void run(Tuple &, return_type &)
			{}
		};
		template <typename Term, typename... Args>
		static void insert_impl(return_type &retval,std::tuple<Args...> &mult_res)
		{
			inserter<std::tuple<Args...>>::run(mult_res,retval);
		}
		static void insert_impl(return_type &retval, typename Series1::term_type &mult_res)
		{
			retval.insert(mult_res);
		}
	private:
		const Series1				&m_s1;
		const Series2				&m_s2;
		mutable std::vector<term_type1 const *>	m_v1;
		mutable std::vector<term_type2 const *>	m_v2;
};

}

#endif
