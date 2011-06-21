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

#ifndef PIRANHA_SERIES_MULTIPLIER_H
#define PIRANHA_SERIES_MULTIPLIER_H

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <list>
#include <new> // For bad_alloc.
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "concepts/multipliable_term.hpp"
#include "concepts/series.hpp"
#include "config.hpp"
#include "echelon_descriptor.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "thread_group.hpp"
#include "thread_management.hpp"
#include "threading.hpp"

namespace piranha
{

/// Default series multiplier.
/**
 * This class is used within piranha::base_series::multiply_by_series() to multiply two series as follows:
 * 
 * - an instance of series multiplier is created using two series (possibly of different types) as construction arguments,
 * - operator()() is called with a generic piranha::echelon_descriptor as argument, returning an instance
 *   of \p Series1 as result of the multiplication of the two series used for construction.
 * 
 * Any specialisation of this class must respect the protocol described above (i.e., construction from series
 * instances and operator()() to compute the result).
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
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will have no effect on an existing object.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo series multiplier concept?
 * \todo in the heuristic to decide single vs multithread we should take into account if coefficient is series or not, and maybe provide
 * means via template specialisation to customise the behaviour for different types of coefficients.
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
		/// Alias for the type of the result of the multiplication.
		typedef decltype(std::declval<Series1>().multiply_by_series(
			std::declval<Series1>(),std::declval<echelon_descriptor<term_type1>>())) return_type;
	public:
		/// Constructor.
		/**
		 * @param[in] s1 first series.
		 * @param[in] s2 second series.
		 */
		explicit series_multiplier(const Series1 &s1, const Series2 &s2) : m_s1(s1),m_s2(s2)
		{}
		/// Compute result of series multiplication.
		/**
		 * This method will call execute() with a \p Functor type that uses the <tt>multiply()</tt> method
		 * of the first series' term type to produce the results of term-by-term multiplications, and
		 * piranha::base_series::insert() to insert such results in the return value.
		 * 
		 * @param[in] ed piranha::echelon_descriptor that will be used to build (via repeated insertions)
		 * the result of the series multiplication.
		 * 
		 * @return the result of multiplying the two series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::base_series::insert(),
		 * - the <tt>multiply()</tt> method of the term type of \p Series1,
		 * - execute().
		 */
		template <typename Term>
		return_type operator()(const echelon_descriptor<Term> &ed) const
		{
			return execute<plain_functor<Term>>(ed);
		}
	protected:
		/// Low-level implementation of series multiplication.
		/**
		 * The multiplication algorithm proceeds as follows:
		 * 
		 * - if one of the two series is empty, a default-constructed instance of \p return_type is returned;
		 * - a heuristic determines whether to enable multi-threaded mode or not;
		 * - in single-threaded mode:
		 *   - an instance of \p Functor is created and its <tt>operator()</tt> is run iteratively over the terms
		 *     of the two series to construct the return value;
		 * - in multi-threaded mode:
		 *   - the first series is subdivided into segments and the same process described for single-threaded mode is run in parallel,
		 *     storing the multiple resulting series in a list;
		 *   - the series in the result list are merged into a single series via piranha::base_series::insert().
		 * 
		 * The protocol expected by an instance of type \p Functor is the following:
		 * 
		 * - it must be constructible from two vectors of const pointers to the terms in the input series, an echelon
		 *   descriptor of the type of \p ed, and an instance of type \p return_type that will be used to
		 *   accumulate the terms during multiplication;
		 * - it must be provided with an <tt>operator()()</tt>, taking two unsigned integers (e.g., \p i and \p j) as input
		 *   parameters and returning void. A call of this operator will multiply the <tt>i</tt>-th term of the first series
		 *   by the <tt>j</tt>-th term of the second series (as they appear in the pointers vectors),
		 *   and insert the result into the the instance of \p return_type used for construction.
		 * 
		 * Note that the parameters passed to the constructor exist outside the \p Functor. In particular,
		 * the \p return_type instance used for construction will then be used to create the return value (and
		 * thus \p Functor is expected to use a reference or a pointer to such object in its operations).
		 * Instances of \p Functor are created sequentially, and they are
		 * allowed to mutate the vectors of terms pointers.
		 * 
		 * @param[in] ed piranha::echelon_descriptor that will be passed to \p Functor to perform term-by-term multiplications.
		 * 
		 * @return the result of multiplying the first series by the second series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - memory allocation errors in standard containers,
		 * - \p boost::numeric_cast (in case of out-of-range convertions from one integral type to another),
		 * - the cast operator of piranha::integer to integral types,
		 * - the constructor and call operator of \p Functor,
		 * - errors in threading primitives,
		 * - piranha::base_series::insert().
		 */
		template <typename Functor, typename Term>
		return_type execute(const echelon_descriptor<Term> &ed) const
		{
			// Do not do anything if one of the two series is empty.
			if (unlikely(m_s1.empty() || m_s2.empty())) {
				return return_type{};
			}
			// Cache the pointers.
			// NOTE: here maybe it is possible to improve performance by using std::array when number of
			// terms is small.
			std::vector<term_type1 const *> v1;
			std::vector<term_type2 const *> v2;
			v1.reserve(m_s1.size());
			v2.reserve(m_s2.size());
			// Fill in the vectors of pointers.
			std::back_insert_iterator<decltype(v1)> bii1(v1);
			std::transform(m_s1.m_container.begin(),m_s1.m_container.end(),bii1,[](const term_type1 &t) {return &t;});
			std::back_insert_iterator<decltype(v2)> bii2(v2);
			std::transform(m_s2.m_container.begin(),m_s2.m_container.end(),bii2,[](const term_type2 &t) {return &t;});
			// This is the size type that will be used throughout the calculations.
			typedef decltype(v1.size()) size_type;
			const auto size1 = v1.size(), size2 = v2.size();
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
std::cout << "using " << n_threads << " threads\n";
			// Go single-thread if heurisitcs says so or if we are already in a different thread from the main one.
			if (likely(n_threads == 1u || runtime_info::get_main_thread_id() != this_thread::get_id())) {
				return_type retval;
				Functor f(v1,v2,ed,retval);
				rehasher(f,retval,size1,size2);
				blocked_multiplication(f,size_type(0u),size1,size_type(0u),size2);
				return retval;
			} else {
// boost::posix_time::ptime time0 = boost::posix_time::microsec_clock::local_time();
				// Tools to handle exceptions thrown in the threads.
				std::vector<exception_ptr> exceptions;
				exceptions.reserve(n_threads);
				if (exceptions.capacity() != n_threads) {
					throw std::bad_alloc();
				}
				mutex exceptions_mutex;
				// Build the return values and the multiplication functors.
				std::list<return_type> retval_list;
				std::list<Functor> functor_list;
				const auto block_size = size1 / n_threads;
				for (size_type i = 0u; i < n_threads; ++i) {
					retval_list.push_back(return_type{});
					functor_list.push_back(Functor(v1,v2,ed,retval_list.back()));
				}
				thread_group tg;
				condition_variable c;
				mutex m;
				auto f_it = functor_list.begin();
				auto r_it = retval_list.begin();
				size_type cur_n = 0u;
				for (size_type i = 0u; i < n_threads; ++i, ++f_it, ++r_it) {
					// Last thread needs a different size from block_size.
					const size_type s1 = (i == n_threads - 1u) ? (size1 - i * block_size) : block_size;
					// Functor for use in the thread.
					auto f = [&v1,&v2,f_it,r_it,i,block_size,s1,size2,&c,&m,&cur_n,&exceptions,&exceptions_mutex]() -> void {
						try {
							const auto tmp_i = i;
							thread_management::binder binder;
							series_multiplier::rehasher(*f_it,*r_it,s1,size2,&c,&m,&cur_n,&tmp_i);
// std::cout << "bsize : " << r_it->m_container.bucket_count() << '\n';
							series_multiplier::blocked_multiplication(*f_it,
								i * block_size,s1,size_type(0u),size2);
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
				auto final_estimate = esitmate_final_series_size(Functor(v1,v2,ed,retval),size1,size2,retval);
				// We want to make sure that final_estimate contains at least 1 element, so that we can use faster low-level
				// methods in hash_set.
				if (unlikely(!final_estimate)) {
					final_estimate = 1u;
				}
// std::cout << "Final estimate: " <<  final_estimate << '\n';
				// Try to see if a series already has enough buckets.
				auto it = std::find_if(retval_list.begin(),retval_list.end(),[&final_estimate](const return_type &r) {return r.m_container.bucket_count() >= final_estimate;});
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
					final_merge(retval,retval_list,n_threads,ed);
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
		template <typename RetvalList, typename Size, typename Term>
		static void final_merge(return_type &retval, RetvalList &retval_list, const Size &n_threads, const echelon_descriptor<Term> &ed)
		{
			piranha_assert(n_threads > 1u);
			piranha_assert(retval.m_container.bucket_count());
			// Misc for exception handling.
			mutex e_mutex;
			std::vector<exception_ptr> e_vector;
			// We never need more capacity than the number of threads.
			e_vector.reserve(n_threads);
			if (unlikely(e_vector.capacity() != n_threads)) {
				throw std::bad_alloc();
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
				throw std::bad_alloc();
			}
			thread_group tg2;
			mutex m;
			std::vector<integer> new_sizes;
			new_sizes.reserve(n_threads);
			if (unlikely(new_sizes.capacity() != n_threads)) {
				throw std::bad_alloc();
			}
			// Vector of terms for which insertion failed.
			const bucket_size_type bucket_count = retval.m_container.bucket_count(), block_size = bucket_count / n_threads;
			for (Size n = 0u; n < n_threads; ++n) {
				// The are the bucket indices allowed to the current thread.
				const bucket_size_type start = n * block_size, end = (n == n_threads - 1u) ? bucket_count : (n + 1u) * block_size;
				auto f = [start,end,&size,&retval,&ed,&idx,&m,&new_sizes,&e_mutex,&e_vector]() -> void {
					try {
						thread_management::binder b;
						size_type count_plus = 0u, count_minus = 0u;
						for (size_type i = 0u; i < size; ++i) {
							const auto &bucket_idx = idx[i].first;
							auto &term_it = idx[i].second;
							if (bucket_idx >= start && bucket_idx < end) {
								// Reconstruct part of base_series insertion, without the update in the number of elements.
								// NOTE: compatibility and ignorability do not matter, as terms being inserted come from base_series
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
									piranha_assert(!it->is_ignorable(ed) && it->is_compatible(ed));
									// Cleanup function.
									auto cleanup = [&]() -> void {
										if (unlikely(!it->is_compatible(ed) || it->is_ignorable(ed))) {
											retval.m_container._erase(it);
											// After term is erased, update count.
											piranha_assert(count_minus < boost::integer_traits<size_type>::const_max);
											++count_minus;
										}
									};
									try {
										// The term exists already, update it.
										retval.template insertion_cf_arithmetics<true>(it,std::move(*term_it),ed);
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
			if (unlikely(retval.m_container.load_factor() > retval.m_container.get_max_load_factor())) {
				retval.m_container.rehash(
					static_cast<bucket_size_type>(integer(std::ceil(retval.size() / retval.m_container.get_max_load_factor())))
				);
			}
		}
		template <typename Term>
		struct plain_functor
		{
			explicit plain_functor(const std::vector<term_type1 const *> &v1, const std::vector<term_type2 const *> &v2,
				const echelon_descriptor<Term> &ed, return_type &retval):
				m_v1(v1),m_v2(v2),m_ed(ed),m_retval(retval)
			{}
			template <typename Size>
			void operator()(const Size &i, const Size &j) const
			{
				piranha_assert(i < m_v1.size() && j < m_v2.size());
				(m_v1[i])->multiply(m_tmp,*(m_v2[j]),m_ed);
				series_multiplier::insert_impl(m_retval,m_tmp,m_ed);
			}
			const std::vector<term_type1 const *>			&m_v1;
			const std::vector<term_type2 const *>			&m_v2;
			const echelon_descriptor<Term>				&m_ed;
			return_type						&m_retval;
			mutable typename term_type1::multiplication_result_type	m_tmp;
		};
		// Functor tasked to prepare return value(s) with estimated bucket sizes (if
		// it is worth to perform such analysis).
		template <typename Functor, typename Size>
		static void rehasher(const Functor &f, return_type &r, const Size &s1, const Size &s2,
			condition_variable *c = piranha_nullptr, mutex *m = piranha_nullptr, Size *cur_n = piranha_nullptr,
			const Size *n = piranha_nullptr)
		{
			piranha_assert((c && m && cur_n && n) || (!c && !m && !cur_n && !n));
			// NOTE: hard-coded value of 100 for minimum size of input series.
			// NOTE: always do the analysis in multi-thread mode, otherwise
			// the condition variable will wait on threads that might never get there (when
			// the subdivision of work produces one block of size > 100, and all the other smaller).
			if (c || (s1 > 100u && s2 > 100u)) {
				// NOTE: here we could have (very unlikely) some overflow or memory error in the computation
				// of the estimate or in rehashing. In such a case, just ignore the rehashing, clean
				// up retval just to be sure, and proceed.
				try {
					auto size = esitmate_final_series_size(f,s1,s2,r,c,m,cur_n,n);
					r.m_container.rehash(static_cast<decltype(size)>((size * integer(r.m_container.get_max_load_factor() * 100)) / 100));
				} catch (...) {
					r.m_container.clear();
				}
			}
		}
		template <typename Functor, typename Size>
		static bucket_size_type esitmate_final_series_size(const Functor &f, const Size &size1, const Size &size2, return_type &retval,
			condition_variable *c = piranha_nullptr, mutex *m = piranha_nullptr, Size *cur_n = piranha_nullptr,
			const Size *n = piranha_nullptr)
		{
			piranha_assert((c && m && cur_n && n) || (!c && !m && !cur_n && !n));
			// NOTE: Hard-coded number of trials = 10.
			const auto ntrials = 10u;
			std::vector<std::vector<Size>> v_idx1(ntrials), v_idx2(ntrials);
			// Functor to fill the vectors of indices and randomize them.
			auto randomizer = [&ntrials,&v_idx1,&v_idx2,&size1,&size2]() -> void {
				for (auto i = 0u; i < ntrials; ++i) {
					if (i) {
						v_idx1[i] = v_idx1[i - 1u];
						v_idx2[i] = v_idx2[i - 1u];
						std::random_shuffle(v_idx1[i].begin(),v_idx1[i].end());
						std::random_shuffle(v_idx2[i].begin(),v_idx2[i].end());
					} else {
						v_idx1[i].reserve(size1);
						v_idx2[i].reserve(size2);
						for (Size j = 0u; j < size1; ++j) {
							v_idx1[i].push_back(j);
						}
						for (Size j = 0u; j < size2; ++j) {
							v_idx2[i].push_back(j);
						}
						std::random_shuffle(v_idx1[i].begin(),v_idx1[i].end());
						std::random_shuffle(v_idx2[i].begin(),v_idx2[i].end());
					}
				}
			};
			if (c) {
				unique_lock<mutex>::type lock(*m);
				while (*cur_n != *n) {
					c->wait(lock);
				}
				randomizer();
				++*cur_n;
				c->notify_all();
			} else {
				randomizer();
			}
			integer mean(0);
			// This could be easily parallelised, but not sure if it is worth.
			for (auto n = 0u; n < ntrials; ++n) {
				const auto &idx1 = v_idx1[n], &idx2 = v_idx2[n];
				Size i = 0u;
				for (; i < std::min<Size>(size1,size2); ++i) {
					// NOTE: by accessing idx1 and idx2 using an instance of type Size we are
					// not absolutely sure that it will work in face of types with different ranges.
					// The worst that can happen is that i is truncated to idx1/2's size type, so
					// it seems not much of a big deal - worst that can happen is underestimate
					// of final series size.
					f(idx1[i],idx2[i]);
					if (retval.size() != i + 1u) {
						break;
					}
				}
				mean += i;
				// Reset retval.
				retval.m_container.clear();
			}
			mean /= ntrials;
			// NOTE: heuristic from experiments.
			const integer M = (mean * mean * 6) / 3;
// std::cout << "M = " << M << '\n';
			return static_cast<bucket_size_type>(M);
		}
		template <typename Functor, typename Size>
		static void blocked_multiplication(const Functor &f, const Size &start1, const Size &size1,
			const Size &start2, const Size &size2)
		{
			// NOTE: hard-coded block size of 256.
			static_assert(std::is_unsigned<Size>::value && boost::integer_traits<Size>::const_max >= 256u, "Invalid size type.");
			const Size bsize1 = 256u, nblocks1 = size1 / bsize1, bsize2 = bsize1, nblocks2 = size2 / bsize2;
			// Start and end of last (possibly irregular) blocks.
			const Size i_ir_start = start1 + nblocks1 * bsize1, i_ir_end = start1 + size1;
			const Size j_ir_start = start2 + nblocks2 * bsize2, j_ir_end = start2 + size2;
			for (Size n1 = 0u; n1 < nblocks1; ++n1) {
				const Size i_start = start1 + n1 * bsize1, i_end = i_start + bsize1;
				// regulars1 * regulars2
				for (Size n2 = 0u; n2 < nblocks2; ++n2) {
					const Size j_start = start2 + n2 * bsize2, j_end = j_start + bsize2;
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
				const Size j_start = start2 + n2 * bsize2, j_end = j_start + bsize2;
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
			template <typename Term>
			static void run(Tuple &t, return_type &retval, const echelon_descriptor<Term> &ed)
			{
				retval.insert(std::get<N>(t),ed);
				inserter<Tuple,N + static_cast<std::size_t>(1)>::run(t,retval,ed);
			}
		};
		template <typename Tuple, std::size_t N>
		struct inserter<Tuple,N,typename std::enable_if<N == std::tuple_size<Tuple>::value>::type>
		{
			template <typename Term>
			static void run(Tuple &, return_type &, const echelon_descriptor<Term> &)
			{}
		};
		template <typename Term, typename... Args>
		static void insert_impl(return_type &retval,std::tuple<Args...> &mult_res, const echelon_descriptor<Term> &ed)
		{
			inserter<std::tuple<Args...>>::run(mult_res,retval,ed);
		}
		template <typename Term>
		static void insert_impl(return_type &retval, typename Series1::term_type &mult_res, const echelon_descriptor<Term> &ed)
		{
			retval.insert(mult_res,ed);
		}
	private:
		const Series1 &m_s1;
		const Series2 &m_s2;
};

}

#endif
