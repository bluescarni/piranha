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

#ifndef PIRANHA_ARRAY_HASH_SET_HPP
#define PIRANHA_ARRAY_HASH_SET_HPP

#include <boost/iterator/iterator_facade.hpp>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "small_vector.hpp"
#include "thread_pool.hpp"
#include "type_traits.hpp"

namespace piranha
{

// TODO:
// - optimisation for the begin() iterator,
// - check again about the mod implementation,
// - in the dtor checks, do we still want the shutdown() logic after we rework symbol?
//   are we still acessing potentially global variables?
// - in the dtor checks, remember to change the load_factor() logic if we make max
//   load factor a soft limit (i.e., it could go past the limit while using
//   the low level interface in poly multiplication).

template <typename T, typename Hash = std::hash<T>, typename Pred = std::equal_to<T>>
class array_hash_set
{
		PIRANHA_TT_CHECK(is_container_element,T);
		PIRANHA_TT_CHECK(is_hash_function_object,Hash,T);
		PIRANHA_TT_CHECK(is_equality_function_object,Pred,T);
		// A few useful typedefs.
		using bucket_type = small_vector<T,std::integral_constant<std::size_t,1u>>;
		using ptr_type = bucket_type *;
		using allocator_type = std::allocator<bucket_type>;
	public:
		/// Size type.
		using size_type = std::size_t;
		/// Functor type for the calculation of hash values.
		using hasher = Hash;
		/// Functor type for comparing the items in the set.
		using key_equal = Pred;
		/// Key type.
		using key_type = T;
	private:
		// Pack these together, to exploit likely EBCO.
		using pack_type = std::tuple<ptr_type,hasher,key_equal,allocator_type>;
		// A few handy accessors.
		ptr_type &ptr()
		{
			return std::get<0u>(m_pack);
		}
		const ptr_type &ptr() const
		{
			return std::get<0u>(m_pack);
		}
		const hasher &hash() const
		{
			return std::get<1u>(m_pack);
		}
		const key_equal &k_equal() const
		{
			return std::get<2u>(m_pack);
		}
		allocator_type &allocator()
		{
			return std::get<3u>(m_pack);
		}
		const allocator_type &allocator() const
		{
			return std::get<3u>(m_pack);
		}
		// Definition of the iterator type.
		// NOTE: re: std::iterator inheritance. It's netiher encouraged not discouraged according to this:
		// http://stackoverflow.com/questions/6471019/can-should-i-inherit-from-stl-iterator
		// I think we can just keep on using boost iterator_facade for the time being.
		template <typename Key>
		class iterator_impl: public boost::iterator_facade<iterator_impl<Key>,Key,boost::forward_traversal_tag>
		{
				friend class array_hash_set;
				// Pointer to the hash set.
				using set_type = typename std::conditional<std::is_const<Key>::value,array_hash_set const,array_hash_set>::type;
				// The local bucket iterator.
				using it_type = typename std::conditional<std::is_const<Key>::value,typename bucket_type::const_iterator,
					typename bucket_type::iterator>::type;
			public:
				// NOTE: here the local iterator is a naked pointer, which we value initialise:
				// a zero-initialisation takes place in this case, which is the same as writing m_it(0).
				iterator_impl():m_set(nullptr),m_idx(0u),m_it{} {}
				explicit iterator_impl(set_type *set, const size_type &idx, it_type it):
					m_set(set),m_idx(idx),m_it(it) {}
			private:
				friend class boost::iterator_core_access;
				void increment()
				{
					piranha_assert(m_set);
					// Get a reference to the internal set container.
					auto container = m_set->ptr();
					// Assert that the current iterator is valid.
					piranha_assert(m_idx < m_set->bucket_count());
					piranha_assert(!container[m_idx].empty());
					piranha_assert(m_it != container[m_idx].end());
					++m_it;
					if (m_it == container[m_idx].end()) {
						const size_type container_size = m_set->bucket_count();
						while (true) {
							++m_idx;
							if (m_idx == container_size) {
								// NOTE: this will represent the end of the set: m_idx == container_size
								// and local iterator zero-constructed.
								m_it = it_type{};
								return;
							} else if (!container[m_idx].empty()) {
								m_it = container[m_idx].begin();
								return;
							}
						}
					}
				}
				bool equal(const iterator_impl &other) const
				{
					piranha_assert(m_set && other.m_set);
					// NOTE: comparing iterators from different containers is UB
					// in the standard library.
					piranha_assert(m_set == other.m_set);
					return (m_idx == other.m_idx && m_it == other.m_it);
				}
				Key &dereference() const
				{
					piranha_assert(m_set && m_idx < m_set->bucket_count() &&
						m_it != m_set->ptr()[m_idx].end());
					return *m_it;
				}
			private:
				set_type	*m_set;
				size_type	m_idx;
				it_type		m_it;
		};
		// The number of available nonzero sizes: it will be the number of bits in the size type. Possible nonzero sizes will be in
		// the [2 ** 0, 2 ** (n-1)] range.
		static const size_type m_n_nonzero_sizes = static_cast<size_type>(std::numeric_limits<size_type>::digits);
		// Get log2 of set size at least equal to hint. To be used only when hint is not zero.
		static size_type get_log2_from_hint(const size_type &hint)
		{
			piranha_assert(hint);
			for (size_type i = 0u; i < m_n_nonzero_sizes; ++i) {
				if ((size_type(1u) << i) >= hint) {
					return i;
				}
			}
			piranha_throw(std::bad_alloc,);
		}
		// Initialisation from number of buckets.
		void init_from_n_buckets(const size_type &n_buckets, unsigned n_threads)
		{
			piranha_assert(!ptr() && !m_log2_size && !m_n_elements);
			if (unlikely(!n_threads)) {
				piranha_throw(std::invalid_argument,"the number of threads must be strictly positive");
			}
			// Proceed to actual construction only if the requested number of buckets is nonzero.
			if (!n_buckets) {
				return;
			}
			const size_type log2_size = get_log2_from_hint(n_buckets);
			const size_type size = static_cast<size_type>(size_type(1u) << log2_size);
			auto new_ptr = allocator().allocate(size);
			// NOTE: std::allocator will throw if allocation fails, so in principle
			// this will never be reached. Let's keep it here for future allocator
			// experiments.
			if (unlikely(!new_ptr)) {
				piranha_throw(std::bad_alloc,);
			}
			if (n_threads == 1u) {
				// Default-construct the elements of the array.
				// NOTE: this is a noexcept operation, no need to account for rolling back.
				for (size_type i = 0u; i < size; ++i) {
					allocator().construct(&new_ptr[i]);
				}
			} else {
				// Sync variables.
				using crs_type = std::vector<std::pair<size_type,size_type>>;
				crs_type constructed_ranges(static_cast<typename crs_type::size_type>(n_threads),
					std::make_pair(size_type(0u),size_type(0u)));
				if (unlikely(constructed_ranges.size() != n_threads)) {
					piranha_throw(std::bad_alloc,);
				}
				// Thread function.
				auto thread_function = [this,new_ptr,&constructed_ranges](const size_type &start, const size_type &end, const unsigned &thread_idx) {
					for (size_type i = start; i != end; ++i) {
						this->allocator().construct(&new_ptr[i]);
					}
					constructed_ranges[thread_idx] = std::make_pair(start,end);
				};
				// Work per thread.
				const auto wpt = size / n_threads;
				future_list<decltype(thread_pool::enqueue(0u,thread_function,0u,0u,0u))> f_list;
				try {
					for (unsigned i = 0u; i < n_threads; ++i) {
						const auto start = static_cast<size_type>(wpt * i),
							end = static_cast<size_type>((i == n_threads - 1u) ? size : wpt * (i + 1u));
						f_list.push_back(thread_pool::enqueue(i,thread_function,start,end,i));
					}
					f_list.wait_all();
					// NOTE: no need to get_all() here, as we know no exceptions will be generated inside thread_func.
				} catch (...) {
					// NOTE: everything in thread_func is noexcept, if we are here the exception was thrown by enqueue or push_back.
					// Wait for everything to wind down.
					f_list.wait_all();
					// Destroy what was constructed.
					for (const auto &r: constructed_ranges) {
						for (size_type i = r.first; i != r.second; ++i) {
							allocator().destroy(&new_ptr[i]);
						}
					}
					// Deallocate before re-throwing.
					allocator().deallocate(new_ptr,size);
					throw;
				}
			}
			// Assign the members.
			ptr() = new_ptr;
			m_log2_size = log2_size;
		}
		// Destruction utils.
		// Run a consistency check on the set, will return false if something is wrong.
		bool sanity_check() const
		{
			// Ignore sanity checks on shutdown.
			if (environment::shutdown()) {
				return true;
			}
			size_type count = 0u;
			for (size_type i = 0u; i < bucket_count(); ++i) {
				for (auto it = ptr()[i].begin(); it != ptr()[i].end(); ++it) {
					if (_bucket(*it) != i) {
						return false;
					}
					++count;
				}
			}
			if (count != m_n_elements) {
				return false;
			}
			// m_log2_size must not be equal to or greater than the number of bits of size_type.
			if (m_log2_size >= unsigned(std::numeric_limits<size_type>::digits)) {
				return false;
			}
			// The container pointer must be consistent with the other members.
			if (!ptr() && (m_log2_size || m_n_elements)) {
				return false;
			}
			// Check size is consistent with number of iterator traversals.
			count = 0u;
			for (auto it = begin(); it != end(); ++it, ++count) {}
			if (count != m_n_elements) {
				return false;
			}
			// Check load factor is not exceeded.
			if (load_factor() > max_load_factor()) {
				return false;
			}
			return true;
		}
		// Destroy all elements and deallocate m_container.
		void destroy_and_deallocate()
		{
			// Proceed to destroy all elements and deallocate only if the set is actually storing something.
			if (ptr()) {
				const size_type size = size_type(1u) << m_log2_size;
				for (size_type i = 0u; i < size; ++i) {
					allocator().destroy(&(ptr()[i]));
				}
				allocator().deallocate(ptr(),size);
			} else {
				piranha_assert(!m_log2_size && !m_n_elements);
			}
		}
	public:
		/// Iterator type.
		/**
		 * A read-only forward iterator.
		 */
		using iterator = iterator_impl<key_type const>;
	private:
		// Static checks on the iterator type.
		PIRANHA_TT_CHECK(is_forward_iterator,iterator);
	public:
		/// Const iterator type.
		/**
		 * Equivalent to the iterator type.
		 */
		using const_iterator = iterator;
		/// Local iterator.
		/**
		 * Const iterator that can be used to iterate through a single bucket.
		 */
		using local_iterator = typename bucket_type::const_iterator;
		/// Default constructor.
		/**
		 * If not specified, it will default-initialise the hasher and the equality predicate. The resulting
		 * set will be empty.
		 *
		 * @param[in] h hasher functor.
		 * @param[in] k equality predicate.
		 *
		 * @throws unspecified any exception thrown by the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		array_hash_set(const hasher &h = hasher{}, const key_equal &k = key_equal{}):
			m_pack(nullptr,h,k,allocator_type{}),m_log2_size(0u),m_n_elements(0u)
		{}
		/**
		 * Will construct a set whose number of buckets is at least equal to \p n_buckets. If \p n_threads is not 1,
		 * then the first \p n_threads threads from piranha::thread_pool will be used concurrently for the initialisation
		 * of the table.
		 *
		 * @param[in] n_buckets desired number of buckets.
		 * @param[in] h hasher functor.
		 * @param[in] k equality predicate.
		 * @param[in] n_threads number of threads to use during initialisation.
		 *
		 * @throws std::bad_alloc if the desired number of buckets is greater than an implementation-defined maximum, or in case
		 * of memory errors.
		 * @throws std::invalid_argument if \p n_threads is zero.
		 * @throws unspecified any exception thrown by:
		 * - the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>,
		 * - piranha::thread_pool::enqueue() or piranha::future_list::push_back(), if \p n_threads is not 1.
		 */
		explicit array_hash_set(const size_type &n_buckets, const hasher &h = hasher{}, const key_equal &k = key_equal{}, unsigned n_threads = 1u):
			m_pack(nullptr,h,k,allocator_type{}),m_log2_size(0u),m_n_elements(0u)
		{
			init_from_n_buckets(n_buckets,n_threads);
		}
		/// Copy constructor.
		/**
		 * The hasher and the equality predicate will be copied too.
		 *
		 * @param[in] other piranha::array_hash_set that will be copied into \p this.
		 *
		 * @throws unspecified any exception thrown by memory allocation errors,
		 * the copy constructor of the stored type, <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		array_hash_set(const array_hash_set &other):
			m_pack(nullptr,other.m_hasher,other.m_key_equal,other.m_allocator),m_log2_size(0u),
			m_n_elements(0u)
		{
			// Proceed to actual copy only if other has some content.
			if (other.ptr()) {
				const size_type size = size_type(1u) << other.m_log2_size;
				auto new_ptr = allocator().allocate(size);
				if (unlikely(!new_ptr)) {
					piranha_throw(std::bad_alloc,);
				}
				size_type i = 0u;
				try {
					// Copy-construct the elements of the array.
					for (; i < size; ++i) {
						allocator().construct(&new_ptr[i],other.ptr()[i]);
					}
				} catch (...) {
					// Unwind the construction and deallocate, before re-throwing.
					for (size_type j = 0u; j < i; ++j) {
						allocator().destroy(&new_ptr[j]);
					}
					allocator().deallocate(new_ptr,size);
					throw;
				}
				// Assign the members.
				ptr() = new_ptr;
				m_log2_size = other.m_log2_size;
				m_n_elements = other.m_n_elements;
			} else {
				piranha_assert(!other.m_log2_size && !other.m_n_elements);
			}
		}
		/// Destructor.
		/**
		 * No side effects.
		 */
		~array_hash_set()
		{
			piranha_assert(sanity_check());
			destroy_and_deallocate();
		}
		/// Load factor.
		/**
		 * @return <tt>(double)size() / bucket_count()</tt>, or 0 if the set is empty.
		 */
		double load_factor() const
		{
			const auto b_count = bucket_count();
			return (b_count) ? static_cast<double>(size()) / static_cast<double>(b_count) : 0.;
		}
		/// Maximum load factor.
		/**
		 * @return the maximum load factor allowed before a resize.
		 */
		double max_load_factor() const
		{
			// Maximum load factor hard-coded to 1.
			// NOTE: if this is ever made configurable, it should never be allowed to go to zero.
			return 1.;
		}
		/// Number of elements contained in the set.
		/**
		 * @return number of elements in the set.
		 */
		size_type size() const
		{
			return m_n_elements;
		}
		/// Const begin iterator.
		/**
		 * @return array_hash_set::const_iterator to the first element of the set, or end() if the set is empty.
		 */
		const_iterator begin() const
		{
			// NOTE: this could take a while in case of an empty set with lots of buckets. Take a shortcut
			// taking into account the number of elements in the set - if zero, go directly to end()?
			const_iterator retval;
			retval.m_set = this;
			size_type idx = 0u;
			const auto b_count = bucket_count();
			for (; idx < b_count; ++idx) {
				if (!ptr()[idx].empty()) {
					break;
				}
			}
			retval.m_idx = idx;
			// If we are not at the end, assign proper iterator.
			if (idx != b_count) {
				retval.m_it =ptr()[idx].begin();
			}
			return retval;
		}
		/// Const end iterator.
		/**
		 * @return array_hash_set::const_iterator to the position past the last element of the set.
		 */
		const_iterator end() const
		{
			// NOTE: this is consistent with what the iterator traversal algorithm
			// returns when it reaches the end of the container.
			return const_iterator(this,bucket_count(),local_iterator{});
		}
		/// Begin iterator.
		/**
		 * @return array_hash_set::iterator to the first element of the set, or end() if the set is empty.
		 */
		iterator begin()
		{
			return static_cast<array_hash_set const *>(this)->begin();
		}
		/// End iterator.
		/**
		 * @return array_hash_set::iterator to the position past the last element of the set.
		 */
		iterator end()
		{
			return static_cast<array_hash_set const *>(this)->end();
		}
		/// Number of buckets.
		/**
		 * @return number of buckets in the set.
		 */
		size_type bucket_count() const
		{
			return (ptr()) ? (size_type(1u) << m_log2_size) : size_type(0u);
		}
		/** @name Low-level interface
		 * Low-level methods and types.
		 */
		//@{
		/// Index of destination bucket from hash value.
		/**
		 * Note that this method will not check if the number of buckets is zero.
		 *
		 * @param[in] hash input hash value.
		 *
		 * @return index of the destination bucket for an object with hash value \p hash.
		 */
		size_type _bucket_from_hash(const std::size_t &hash) const
		{
			piranha_assert(bucket_count());
			return static_cast<size_type>(hash % (size_type(1u) << m_log2_size));
		}
		/// Index of destination bucket (low-level).
		/**
		 * Equivalent to bucket(), with the exception that this method will not check
		 * if the number of buckets is zero.
		 *
		 * @param[in] k input argument.
		 *
		 * @return index of the destination bucket for \p k.
		 *
		 * @throws unspecified any exception thrown by the call operator of the hasher.
		 */
		size_type _bucket(const key_type &k) const
		{
			return _bucket_from_hash(hasher()(k));
		}
		//@}
	private:
		pack_type	m_pack;
		size_type	m_log2_size;
		size_type	m_n_elements;
};

template <typename T, typename Hash, typename Pred>
const typename array_hash_set<T,Hash,Pred>::size_type array_hash_set<T,Hash,Pred>::m_n_nonzero_sizes;

}

#endif
