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
#include <initializer_list>
#include <limits>
#include <map>
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
#include "serialization.hpp"
#include "small_vector.hpp"
#include "thread_pool.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Array hash set.
/**
 * Hash set class with interface similar to \p std::unordered_set. The main points of difference with respect to
 * \p std::unordered_set are the following:
 *
 * - the exception safety guarantee is weaker (see below),
 * - iterators and iterator invalidation: after a rehash operation, all iterators will be invalidated and existing
 *   references/pointers to the elements will also be invalid; after an insertion/erase operation, all existing iterators, pointers
 *   and references to the elements in the destination bucket will be invalid.
 *
 * The implementation employs a separate chaining strategy consisting of an array of buckets, each one a piranha::small_vector.
 *
 * An additional set of low-level methods is provided: such methods are suitable for use in high-performance and multi-threaded contexts,
 * and, if misused, could lead to data corruption and other unpredictable errors.
 *
 * Note that for performance reasons the implementation employs table sizes that are powers of two. Hence, particular care should be taken
 * that the hash function does not exhibit commensurabilities with powers of 2.
 *
 * ## Type requirements ##
 *
 * - \p T must satisfy piranha::is_container_element,
 * - \p Hash must satisfy piranha::is_hash_function_object,
 * - \p Pred must satisfy piranha::is_equality_function_object.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the strong exception safety guarantee for all operations apart from methods involving insertion,
 * which provide the basic guarantee (after a failed insertion, the table will be left in an unspecified but valid state).
 *
 * ## Move semantics ##
 *
 * Move construction and move assignment will leave the moved-from object equivalent to an empty set whose hasher and
 * equality predicate have been moved-from.
 *
 * ## Serialization ##
 *
 * This class supports serialization if the contained type supports it. Note that the hasher and the comparator
 * are not serialised and they are recreated from scratch upon deserialization.
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
 /* Some TODOs:
 * \todo tests for low-level methods
 * \todo see if it is possible to rework max_load_factor() to return an unsigned instead of double. The unsigned is the max load factor in percentile: 50 means 0.5, etc.
 * \todo memory handling: the usage of the allocator object should be more standard, i.e., use the pointer and reference typedefs defined within, replace
 * positional new with construct even in the list implementation. Then it can be made a template parameter with default = std::allocator.
 * \todo: use of new: we should probably replace new with new, in case new is overloaded -> also, check all occurrences of root new, it is used as well
 * in static_vector for instance.
 * \todo inline the first bucket, with the idea of avoiding memory allocations when the series consist of a single element (useful for instance
 * when iterating over the series with the fat iterator).
 * - optimisation for the begin() iterator,
 * - check again about the mod implementation,
 * - in the dtor checks, do we still want the shutdown() logic after we rework symbol?
 *   are we still acessing potentially global variables?
 * - in the dtor checks, remember to change the load_factor() logic if we make max
 *   load factor a soft limit (i.e., it could go past the limit while using
 *   the low level interface in poly multiplication).
 */
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
		// Enabler for insert().
		template <typename U>
		using insert_enabler = typename std::enable_if<std::is_same<key_type,typename std::decay<U>::type>::value,int>::type;
		// Enabler for unique insert.
		template <typename U>
		using unique_insert_enabler = typename std::enable_if<std::is_same<T,typename std::decay<U>::type>::value,int>::type;
		// Serialization support.
		friend class boost::serialization::access;
		template <class Archive>
		void save(Archive &ar, unsigned int) const
		{
			// Save the number of elements first.
			ar & m_n_elements;
			// Save the elements one by one.
			const auto it_f = end();
			for (auto it = begin(); it != it_f; ++it) {
				ar & (*it);
			}
		}
		template <class Archive>
		void load(Archive &ar, unsigned int)
		{
			// Erase this and work on an empty one. In case of exceptions,
			// we should have an hash set in unspecified but consistent state.
			// We could also enforce strong exception safety by working on a temporary
			// set and then swapping that in, but that would need more memory - and we
			// care about memory for very large series.
			*this = array_hash_set();
			// Recover the number of elements.
			size_type n_elements;
			ar & n_elements;
			// Recover the elements one by one.
			for (size_type i = 0u; i < n_elements; ++i) {
				key_type k;
				ar & k;
				const auto ret_ins = insert(std::move(k));
				// Check that there was no duplicate.
				(void)ret_ins;
				piranha_assert(ret_ins.second);
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
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
		 * of the set.
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
			m_pack(nullptr,other.hash(),other.k_equal(),other.allocator()),m_log2_size(0u),
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
		/// Move constructor.
		/**
		 * After the move, \p other will have zero buckets and zero elements, and its hasher and equality predicate
		 * will have been used to move-construct their counterparts in \p this.
		 *
		 * @param[in] other set to be moved.
		 */
		array_hash_set(array_hash_set &&other) noexcept : m_pack(other.ptr(),std::move(other.hash()),std::move(other.k_equal()),
			std::move(other.allocator())),m_log2_size(other.m_log2_size),m_n_elements(other.m_n_elements)
		{
			// Clear out the other one.
			other.ptr() = nullptr;
			other.m_log2_size = 0u;
			other.m_n_elements = 0u;
		}
		/// Constructor from range.
		/**
		 * Create a set from a range.
		 *
		 * @param[in] begin begin of range.
		 * @param[in] end end of range.
		 * @param[in] n_buckets number of initial buckets.
		 * @param[in] h hash functor.
		 * @param[in] k key equality predicate.
		 *
		 * @throws std::bad_alloc if the desired number of buckets is greater than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>, or arising from
		 * calling insert() on the elements of the range.
		 */
		template <typename InputIterator>
		explicit array_hash_set(const InputIterator &begin, const InputIterator &end, const size_type &n_buckets = 0u,
			const hasher &h = hasher{}, const key_equal &k = key_equal{}):
			m_pack(nullptr,h,k,allocator_type{}),m_log2_size(0u),m_n_elements(0u)
		{
			init_from_n_buckets(n_buckets,1u);
			for (auto it = begin; it != end; ++it) {
				insert(*it);
			}
		}
		/// Constructor from initializer list.
		/**
		 * Will insert() all the elements of the initializer list, ignoring the return value of the operation.
		 * Hash functor and equality predicate will be default-constructed.
		 *
		 * @param[in] list initializer list of elements to be inserted.
		 *
		 * @throws std::bad_alloc if the desired number of buckets is greater than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by either insert() or of the default constructor of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		template <typename U>
		explicit array_hash_set(std::initializer_list<U> list):
			m_pack(nullptr,hasher{},key_equal{},allocator_type{}),m_log2_size(0u),m_n_elements(0u)
		{
			// We do not care here for possible truncation of list.size(), as this is only an optimization.
			init_from_n_buckets(static_cast<size_type>(list.size()),1u);
			for (const auto &x: list) {
				insert(x);
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
		/// Copy assignment operator.
		/**
		 * @param[in] other assignment argument.
		 *
		 * @return reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		array_hash_set &operator=(const array_hash_set &other)
		{
			if (likely(this != &other)) {
				array_hash_set tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other set to be moved into \p this.
		 *
		 * @return reference to \p this.
		 */
		array_hash_set &operator=(array_hash_set &&other) noexcept
		{
			if (likely(this != &other)) {
				destroy_and_deallocate();
				m_pack = std::move(other.m_pack);
				m_log2_size = other.m_log2_size;
				m_n_elements = other.m_n_elements;
				// Zero out other.
				other.ptr() = nullptr;
				other.m_log2_size = 0u;
				other.m_n_elements = 0u;
			}
			return *this;
		}
		/// Remove all elements.
		/**
		 * After this call, size() and bucket_count() will both return zero.
		 */
		void clear()
		{
			destroy_and_deallocate();
			// Reset the members.
			ptr() = nullptr;
			m_log2_size = 0u;
			m_n_elements = 0u;
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
		/// Test for empty set.
		/**
		 * @return \p true if size() returns 0, \p false otherwise.
		 */
		bool empty() const
		{
			return !size();
		}
		/// Index of destination bucket.
		/**
		 * Index to which \p k would belong, were it to be inserted into the set. The index of the
		 * destination bucket is the hash value reduced modulo the bucket count.
		 *
		 * @param[in] k input argument.
		 *
		 * @return index of the destination bucket for \p k.
		 *
		 * @throws piranha::zero_division_error if bucket_count() returns zero.
		 * @throws unspecified any exception thrown by _bucket().
		 */
		size_type bucket(const key_type &k) const
		{
			if (unlikely(!bucket_count())) {
				piranha_throw(zero_division_error,"cannot calculate bucket index in an empty set");
			}
			return _bucket(k);
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 *
		 * @return array_hash_set::const_iterator to <tt>k</tt>'s position in the set, or end() if \p k is not in the set.
		 *
		 * @throws unspecified any exception thrown by _find() or by _bucket().
		 */
		const_iterator find(const key_type &k) const
		{
			if (unlikely(!bucket_count())) {
				return end();
			}
			return _find(k,_bucket(k));
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 *
		 * @return array_hash_set::iterator to <tt>k</tt>'s position in the set, or end() if \p k is not in the set.
		 *
		 * @throws unspecified any exception thrown by _find().
		 */
		iterator find(const key_type &k)
		{
			return static_cast<const array_hash_set *>(this)->find(k);
		}
		/// Insert element.
		/**
		 * \note
		 * This template is activated only if \p T and \p U are the same type, aside from cv qualifications and references.
		 *
		 * If no other key equivalent to \p k exists in the set, the insertion is successful and returns the <tt>(it,true)</tt>
		 * pair - where \p it is the position in the set into which the object has been inserted. Otherwise, the return value
		 * will be <tt>(it,false)</tt> - where \p it is the position of the existing equivalent object.
		 *
		 * @param[in] k object that will be inserted into the set.
		 *
		 * @return <tt>(array_hash_set::iterator,bool)</tt> pair containing an iterator to the newly-inserted object (or its existing
		 * equivalent) and the result of the operation.
		 *
		 * @throws unspecified any exception thrown by:
		 * - array_hash_set::key_type's copy constructor,
		 * - _find(),
		 * - _bucket().
		 * @throws std::overflow_error if a successful insertion would result in size() exceeding the maximum
		 * value representable by type piranha::array_hash_set::size_type.
		 * @throws std::bad_alloc if the operation results in a resize of the set past an implementation-defined
		 * maximum number of buckets.
		 */
		template <typename U, insert_enabler<U> = 0>
		std::pair<iterator,bool> insert(U &&k)
		{
			auto b_count = bucket_count();
			// Handle the case of a set with no buckets.
			if (unlikely(!b_count)) {
				_increase_size();
				// Update the bucket count.
				b_count = 1u;
			}
			// Try to locate the element.
			auto bucket_idx = _bucket(k);
			const auto it = _find(k,bucket_idx);
			if (it != end()) {
				// Item already present, exit.
				return std::make_pair(it,false);
			}
			if (unlikely(m_n_elements == std::numeric_limits<size_type>::max())) {
				piranha_throw(std::overflow_error,"maximum number of elements reached");
			}
			// Item is new. Handle the case in which we need to rehash because of load factor.
			if (unlikely(static_cast<double>(m_n_elements + size_type(1u)) / static_cast<double>(b_count) > max_load_factor())) {
				_increase_size();
				// We need a new bucket index in case of a rehash.
				bucket_idx = _bucket(k);
			}
			const auto it_retval = _unique_insert(std::forward<U>(k),bucket_idx);
			++m_n_elements;
			return std::make_pair(it_retval,true);
		}
		/// Erase element.
		/**
		 * Erase the element to which \p it points. \p it must be a valid iterator
		 * pointing to an element of the set.
		 *
		 * Erasing an element invalidates all iterators pointing to elements in the same bucket
		 * as the erased element.
		 *
		 * After the operation has taken place, the size() of the set will be decreased by one.
		 *
		 * @param[in] it iterator to the element of the set to be removed.
		 *
		 * @return iterator pointing to the element following \p it pior to the element being erased, or end() if
		 * no such element exists.
		 */
		iterator erase(const_iterator it)
		{
			piranha_assert(!empty());
			const auto b_it = _erase(it);
			iterator retval;
			retval.m_set = this;
			const auto b_count = bucket_count();
			// Travel to the next iterator if necessary.
			if (b_it == ptr()[it.m_idx].end()) {
				size_type idx = it.m_idx + 1u;
				// Advance to the first non-empty bucket if necessary,
				// without going past the end of the set.
				for (; idx < b_count; ++idx) {
					if (!ptr()[idx].empty()) {
						break;
					}
				}
				retval.m_idx = idx;
				// If we are not at the end, assign proper iterator.
				if (idx != b_count) {
					retval.m_it = ptr()[idx].begin();
				}
			} else {
				retval.m_idx = it.m_idx;
				retval.m_it = b_it;
			}
			piranha_assert(m_n_elements);
			--m_n_elements;
			return retval;
		}
		/// Rehash set.
		/**
		 * Change the number of buckets in the set to at least \p new_size. No rehash is performed
		 * if rehashing would lead to exceeding the maximum load factor. If \p n_threads is not 1,
		 * then the first \p n_threads threads from piranha::thread_pool will be used concurrently during
		 * the rehash operation.
		 *
		 * @param[in] new_size new desired number of buckets.
		 * @param[in] n_threads number of threads to use.
		 *
		 * @throws std::invalid_argument if \p n_threads is zero.
		 * @throws unspecified any exception thrown by the constructor from number of buckets,
		 * _unique_insert() or _bucket().
		 */
		void rehash(const size_type &new_size, unsigned n_threads = 1u)
		{
			if (unlikely(!n_threads)) {
				piranha_throw(std::invalid_argument,"the number of threads must be strictly positive");
			}
			// If rehash is requested to zero, do something only if there are no items stored in the set.
			if (!new_size) {
				if (!size()) {
					clear();
				}
				return;
			}
			// Do nothing if rehashing to the new size would lead to exceeding the max load factor.
			if (static_cast<double>(size()) / static_cast<double>(new_size) > max_load_factor()) {
				return;
			}
			// Create a new set with needed amount of buckets.
			array_hash_set new_set(new_size,hash(),k_equal(),n_threads);
			try {
				const auto it_f = _m_end();
				for (auto it = _m_begin(); it != it_f; ++it) {
					const auto new_idx = new_set._bucket(*it);
					new_set._unique_insert(std::move(*it),new_idx);
				}
			} catch (...) {
				// Clear up both this and the new set upon any kind of error.
				clear();
				new_set.clear();
				throw;
			}
			// Retain the number of elements.
			new_set.m_n_elements = m_n_elements;
			// Clear the old set.
			clear();
			// Assign the new set.
			*this = std::move(new_set);
		}
		/// Get information on the sparsity of the set.
		/**
		 * @return an <tt>std::map<size_type,size_type></tt> in which the key is the number of elements
		 * stored in a bucket and the mapped type the number of buckets containing those many elements.
		 *
		 * @throws unspecified any exception thrown by memory errors in standard containers.
		 */
		std::map<size_type,size_type> evaluate_sparsity() const
		{
			const auto it_f = ptr() + bucket_count();
			std::map<size_type,size_type> retval;
			size_type counter;
			for (auto it = ptr(); it != it_f; ++it) {
				counter = 0u;
				for (auto l_it = it->begin(); l_it != it->end(); ++l_it) {
					++counter;
				}
				++retval[counter];
			}
			return retval;
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
		/// Increase bucket count.
		/**
		 * Increase the number of buckets to the next implementation-defined value.
		 *
		 * @throws std::bad_alloc if the operation results in a resize of the set past an implementation-defined
		 * maximum number of buckets.
		 * @throws unspecified any exception thrown by rehash().
		 */
		void _increase_size()
		{
			if (unlikely(m_log2_size >= m_n_nonzero_sizes - 1u)) {
				piranha_throw(std::bad_alloc,);
			}
			// We must take care here: if the set has zero buckets,
			// the next log2_size is 0u. Otherwise increase current log2_size.
			piranha_assert(ptr() || (!ptr() && !m_log2_size));
			const auto new_log2_size = (ptr()) ? (m_log2_size + 1u) : 0u;
			// Rehash to the new size.
			rehash(size_type(1u) << new_log2_size);
		}
		/// Insert unique element.
		/**
		 * \note
		 * This template method is activated only if piranha::array_hash_set::key_type and \p U are the same type, aside from cv qualifications and references.
		 *
		 * The parameter \p bucket_idx is the index of the destination bucket for \p k and, for a
		 * set with a nonzero number of buckets, must be equal to the output
		 * of bucket() before the insertion.
		 *
		 * This method will not check if a key equivalent to \p k already exists in the set, it will not
		 * update the number of elements present in the set after the insertion, it will not resize
		 * the set in case the maximum load factor is exceeded, nor it will check
		 * if the value of \p bucket_idx is correct.
		 *
		 * @param[in] k object that will be inserted into the set.
		 * @param[in] bucket_idx destination bucket for \p k.
		 *
		 * @return iterator pointing to the newly-inserted element.
		 *
		 * @throws unspecified any exception thrown by the copy constructor of piranha::array_hash_set::key_type or by memory allocation
		 * errors.
		 */
		template <typename U, unique_insert_enabler<U> = 0>
		iterator _unique_insert(U &&k, const size_type &bucket_idx)
		{
			// Assert that key is not present already in the set.
			piranha_assert(find(std::forward<U>(k)) == end());
			// Assert bucket index is correct.
			piranha_assert(bucket_idx == _bucket(k));
			auto &bucket = ptr()[bucket_idx];
			bucket.push_back(std::forward<U>(k));
			// NOTE: here bucket.begin() is fine as we just inserted one element,
			// same with bucket.size().
			return iterator(this,bucket_idx,bucket.begin() + (bucket.size() - 1u));
		}
		/// Mutable iterator.
		/**
		 * This iterator type provides non-const access to the elements of the set. Please note that modifications
		 * to an existing element of the set might invalidate the relation between the element and its position in the set.
		 * After such modifications of one or more elements, the only valid operation is array_hash_set::clear() (destruction of the
		 * set before calling array_hash_set::clear() will lead to assertion failures in debug mode).
		 */
		using _m_iterator = iterator_impl<key_type>;
		/// Mutable begin iterator.
		/**
		 * @return array_hash_set::_m_iterator to the beginning of the set.
		 */
		_m_iterator _m_begin()
		{
			// NOTE: same possible optimisation as in begin().
			const auto b_count = bucket_count();
			_m_iterator retval;
			retval.m_set = this;
			size_type idx = 0u;
			for (; idx < b_count; ++idx) {
				if (!ptr()[idx].empty()) {
					break;
				}
			}
			retval.m_idx = idx;
			// If we are not at the end, assign proper iterator.
			if (idx != b_count) {
				retval.m_it = ptr()[idx].begin();
			}
			return retval;
		}
		/// Mutable end iterator.
		/**
		 * @return array_hash_set::_m_iterator to the end of the set.
		 */
		_m_iterator _m_end()
		{
			return _m_iterator(this,bucket_count(),typename bucket_type::iterator{});
		}
		/// Find element (low-level).
		/**
		 * Locate element in the set. The parameter \p bucket_idx is the index of the destination bucket for \p k and, for
		 * a set with a nonzero number of buckets, must be equal to the output
		 * of bucket() before the insertion. This method will not check if the value of \p bucket_idx is correct.
		 *
		 * @param[in] k element to be located.
		 * @param[in] bucket_idx index of the destination bucket for \p k.
		 *
		 * @return array_hash_set::iterator to <tt>k</tt>'s position in the set, or end() if \p k is not in the set.
		 *
		 * @throws unspecified any exception thrown by calling the equality predicate.
		 */
		const_iterator _find(const key_type &k, const size_type &bucket_idx) const
		{
			// Assert bucket index is correct.
			piranha_assert(bucket_idx == _bucket(k) && bucket_idx < bucket_count());
			const auto &b = ptr()[bucket_idx];
			const auto it_f = b.end();
			const_iterator retval(end());
			for (auto it = b.begin(); it != it_f; ++it) {
				if (k_equal()(*it,k)) {
					retval.m_idx = bucket_idx;
					retval.m_it = it;
					break;
				}
			}
			return retval;
		}
		/// Erase element.
		/**
		 * Erase the element to which \p it points. \p it must be a valid iterator
		 * pointing to an element of the set.
		 *
		 * Erasing an element invalidates all iterators pointing to elements in the same bucket
		 * as the erased element.
		 *
		 * This method will not update the number of elements in the set, nor it will try to access elements
		 * outside the bucket to which \p it refers.
		 *
		 * @param[in] it iterator to the element of the set to be removed.
		 *
		 * @return local iterator pointing to the element following \p it pior to the element being erased, or local end() if
		 * no such element exists.
		 */
		local_iterator _erase(const_iterator it)
		{
			// Verify the iterator is valid.
			piranha_assert(it.m_set == this);
			piranha_assert(it.m_idx < bucket_count());
			piranha_assert(!ptr()[it.m_idx].empty());
			piranha_assert(it.m_it != ptr()[it.m_idx].end());
			auto &bucket = ptr()[it.m_idx];
			return bucket.erase(it.m_it);
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
