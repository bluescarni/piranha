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
#include <atomic>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <future>
#include <iterator>
#include <limits>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/atomic_utils.hpp"
#include "detail/gcd.hpp"
#include "detail/series_fwd.hpp"
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

// TODO test with empty series.
// TODO document the swapping.
// TODO static checks on the functors.
// TODO test the skip.
// \todo optimization in case one series has 1 term with unitary key and both series same type: multiply directly coefficients.
// \todo think about the possibility of caching optimizations. For instance: merge the arguments of series coefficients, avoiding n ** 2 merge
// operations during multiplication.
template <typename Series>
class base_series_multiplier: private detail::base_series_multiplier_impl<Series>
{
		PIRANHA_TT_CHECK(is_series,Series);
	public:
		/// Alias for a vector of const pointers to series terms.
		using v_ptr = std::vector<typename Series::term_type const *>;
		/// TODO...
		using size_type = typename v_ptr::size_type;
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
		 * @param[in] s1 first series.
		 * @param[in] s2 second series.
		 *
		 * @throws std::invalid_argument if the symbol sets of \p s1 and \p s2 differ.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers.
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
		template <typename MultFunctor, typename SkipFunctor = no_skip>
		void blocked_multiplication(const MultFunctor &mf,
			const size_type &start1, const size_type &end1,
			const size_type &start2, const size_type &end2,
			const SkipFunctor &sf = no_skip{}) const
		{
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
		// TODO: ignore functor
		// TODO: document exception behaviour.
		// TODO: document it always returns 1 at least.
		template <std::size_t MultArity, typename MultFunctor>
		bucket_size_type estimate_final_series_size(Series &tmp, const MultFunctor &mf) const
		{
			// Make sure tmp is cleared in any case.
			series_clearer sc(tmp);
			// Local shortcut.
			constexpr std::size_t result_size = MultArity;
			static_assert(result_size != 0u,"Invalid multiplication arity in series multiplier.");
			// Cache these.
			const size_type size1 = m_v1.size(), size2 = m_v2.size();
			// If one of the two series is empty, just return 0.
			if (unlikely(!size1 || !size2)) {
				return 1u;
			}
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
			// Init counter.
			integer total(0);
			// Go with the trials.
			// NOTE: This could be easily parallelised, but not sure if it is worth it.
			for (auto n = 0u; n < ntrials; ++n) {
				// Randomise.
				std::shuffle(v_idx1.begin(),v_idx1.end(),engine);
				std::shuffle(v_idx2.begin(),v_idx2.end(),engine);
				size_type count = 0u;
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
					// Check for unlikely overflow when increasing count.
					if (unlikely(result_size > std::numeric_limits<size_type>::max() ||
						count > std::numeric_limits<size_type>::max() - result_size))
					{
						piranha_throw(std::overflow_error,"overflow error");
					}
					if (tmp.size() != count + result_size) {
						break;
					}
					// Increase cycle variables.
					count = static_cast<size_type>(count + result_size);
					++it1;
					++it2;
				}
				total += count;
				// Reset tmp.
				tmp._container().clear();
			}
			const auto mean = total / ntrials;
			auto retval = mean * mean * multiplier;
			// Return always at least one.
			if (retval.sign() == 0) {
				return 1u;
			} else {
				return static_cast<bucket_size_type>(retval);
			}
		}
		template <bool FastMode>
		struct plain_multiplier
		{
			using term_type = typename Series::term_type;
			using key_type = typename term_type::key_type;
			using it_type = decltype(std::declval<Series &>()._container().end());
			static constexpr std::size_t m_arity = key_type::multiply_arity;
			explicit plain_multiplier(const std::vector<term_type const *>	&v1, const std::vector<term_type const *> &v2,
				Series &retval):m_v1(v1),m_v2(v2),m_retval(retval),m_c_end(retval._container().end())
			{}
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
			mutable std::array<term_type,m_arity>	m_tmp_t;
			const std::vector<term_type const *>	&m_v1;
			const std::vector<term_type const *>	&m_v2;
			Series					&m_retval;
			const it_type				m_c_end;
		};
		static void sanitize_series(Series &retval, unsigned n_threads)
		{
			using term_type = typename Series::term_type;
			if (unlikely(n_threads == 0u)) {
				piranha_throw(std::invalid_argument,"invalid number of threads");
			}
			auto &container = retval._container();
			const auto &args = retval.get_symbol_set();
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
			piranha_assert(b_count);
			// Adjust the number of threads if they are more than the bucket count.
			const unsigned nt = (n_threads <= b_count) ? n_threads : static_cast<unsigned>(b_count);
			std::mutex m;
			integer global_count(0);
			auto eraser = [b_count,&container,&m,&args,&global_count](const bucket_size_type &start, const bucket_size_type &end) {
				piranha_assert(start < end && end <= b_count);
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
				for (unsigned i = 0u; i < nt; ++i) {
					const auto start = static_cast<bucket_size_type>((b_count / nt) * i),
						end = static_cast<bucket_size_type>((i == nt - 1u) ? b_count : (b_count / nt) * (i + 1u));
					f_list.push_back(thread_pool::enqueue(i,eraser,start,end));
				}
				// First let's wait for everything to finish.
				f_list.wait_all();
				// Then, let's handle the exceptions.
				f_list.get_all();
			} catch (...) {
				f_list.wait_all();
				// Clean up and re-throw.
				container.clear();
				throw;
			}
			// Final update of the total count.
			container._update_size(static_cast<bucket_size_type>(global_count));
		}
		// TODO enabler.
		Series plain_multiplication() const
		{
			// Shortcuts.
			using term_type = typename Series::term_type;
			using cf_type = typename term_type::cf_type;
			using key_type = typename term_type::key_type;
			constexpr std::size_t m_arity = key_type::multiply_arity;
			PIRANHA_TT_CHECK(key_is_multipliable,cf_type,key_type);
			// Do not do anything if one of the two series is empty.
			if (unlikely(m_v1.empty() || m_v2.empty())) {
				// NOTE: requirement is ok, a series must be def-ctible.
				return Series{};
			}
			const size_type size1 = m_v1.size(), size2 = m_v2.size();
			piranha_assert(size1 && size2);
			// Error out if the possible size of the output series is too large. This is to protect against
			// overflows in insertion counts in mt mode.
			if (unlikely(integer(size1) * size2 * m_arity > std::numeric_limits<bucket_size_type>::max())) {
				piranha_throw(std::overflow_error,"possible overflow in series size");
			}
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
std::cout << "estimate: " << est << '\n';
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
					blocked_multiplication(plain_multiplier<true>(m_v1,m_v2,retval),0u,size1,0u,size2);
					sanitize_series(retval,n_threads);
std::cout << retval._container().bucket_count() << ',' << retval._container().size() << '\n';
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
					auto tf = [i,this,block_size,n_threads,&sl_array,&retval]()
					{
						// Used to store the result of term multiplication.
						std::array<term_type,key_type::multiply_arity> tmp_t;
						// End of retval container (thread-safe).
						const auto c_end = retval._container().end();
						// Block functor,
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
						this->blocked_multiplication(f,static_cast<size_type>(i * block_size),e1,0u,this->m_v2.size());
					};
					f_list.push_back(thread_pool::enqueue(static_cast<unsigned>(i),tf));
				}
				f_list.wait_all();
				f_list.get_all();
			} catch (...) {
				f_list.wait_all();
				// Clean up retval as it might be in an inconsistent state.
				retval._container().clear();
				throw;
			}
			try {
				sanitize_series(retval,n_threads);
			} catch (...) {
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
