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

#ifndef PIRANHA_HASH_SET_HPP
#define PIRANHA_HASH_SET_HPP

#include <algorithm>
#include <array>
#include <boost/integer_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <type_traits>
#include <vector>

#include "config.hpp"
#include "debug_access.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "thread_pool.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Hash set.
/**
 * Hash set class with interface similar to \p std::unordered_set. The main points of difference with respect to
 * \p std::unordered_set are the following:
 * 
 * - the exception safety guarantee is weaker (see below),
 * - iterators and iterator invalidation: after a rehash operation, all iterators will be invalidated and existing
 *   references/pointers to the elements will also be invalid; after an insertion/erase operation, all existing iterators, pointers
 *   and references to the elements in the destination bucket will be invalid.
 * 
 * The implementation employs a separate chaining strategy consisting of an array of buckets, each one a singly linked list with the first node
 * stored directly within the array (so that the first insertion in a bucket does not require any heap allocation).
 * 
 * An additional set of low-level methods is provided: such methods are suitable for use in high-performance and multi-threaded contexts,
 * and, if misused, could lead to data corruption and other unpredictable errors.
 * 
 * Note that for performance reasons the implementation employs table sizes that are powers of two. Hence, particular care should be taken
 * that the hash function does not exhibit commensurabilities with powers of 2.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must satisfy piranha::is_container_element,
 * - \p Hash must satisfy piranha::is_hash_function_object,
 * - \p Pred must satisfy piranha::is_equality_function_object.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations apart from the insertion methods,
 * which provide the following guarantee: the table will be either left in the same state as it was before the attempted
 * insertion or it will be emptied.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object equivalent to an empty set whose hasher and
 * equality predicate have been moved-from.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo tests for low-level methods
 * \todo better increase_size with recycling of dynamically-allocated nodes
 * \todo see if it is possible to rework max_load_factor() to return an unsigned instead of double. The unsigned is the max load factor in percentile: 50 means 0.5, etc.
 * \todo see if we can reduce the number of branches in the find algorithm (e.g., when traversing the list) -> this should be a general review of the internal linked list
 * implementation.
 * \todo memory handling: the usage of the allocator object should be more standard, i.e., use the pointer and reference typedefs defined within, replace
 * positional new with construct even in the list implementation. Then it can be made a template parameter with default = std::allocator.
 * \todo: use of new: we should probably replace new with new, in case new is overloaded -> also, check all occurrences of root new, it is used as well
 * in static_vector for instance.
 * \todo store functors in a tuple to get advantage of EBMO.
 * \todo inline the first bucket, with the idea of avoiding memory allocations when the series consist of a single element (useful for instance
 * when iterating over the series with the fat iterator).
 */
template <typename T, typename Hash = std::hash<T>, typename Pred = std::equal_to<T>>
class hash_set
{
		PIRANHA_TT_CHECK(is_container_element,T);
		PIRANHA_TT_CHECK(is_hash_function_object,Hash,T);
		PIRANHA_TT_CHECK(is_equality_function_object,Pred,T);
		// Make friend with debug access class.
		template <typename U>
		friend class debug_access;
		// Node class for bucket element.
		struct node
		{
			typedef typename std::aligned_storage<sizeof(T),alignof(T)>::type storage_type;
			node() noexcept : m_next(nullptr) {}
			// Erase all other ctors/assignments, we do not want to
			// copy around this object as m_storage is not what it is
			// (and often could be uninitialized, which would lead to UB if used).
			node(const node &) = delete;
			node(node &&) = delete;
			node &operator=(const node &) = delete;
			node &operator=(node &&) = delete;
			// NOTE: the idea here is that we use these helpers only *after* the object has been constructed,
			// for constructing the object we must always access m_storage directly. The chain of two casts
			// seems to be the only standard-conforming way of getting out a pointer to T.
			// http://stackoverflow.com/questions/1082378/what-is-the-basic-use-of-aligned-storage
			// http://stackoverflow.com/questions/13466556/aligned-storage-and-strict-aliasing
			// http://en.cppreference.com/w/cpp/types/aligned_storage
			// A couple of further notes:
			// - pointer casting to/from void * is ok (4.10.2 and 5.2.9.13, via static_cast);
			// - the placement new operator (which we use to construct the object into the storage)
			//   is guaranteed to construct the object at the beginning of the storage, and taking
			//   a void * out of the storage will return the address of the object stored within;
			// - the lifetime of the original POD storage_type ends when we reuse its storage via
			//   placement new.
			// This page contains some more info about storage reuse:
			// http://en.cppreference.com/w/cpp/language/lifetime#Storage_reuse
			const T *ptr() const
			{
				piranha_assert(m_next);
				return static_cast<const T *>(static_cast<const void *>(&m_storage));
			}
			T *ptr()
			{
				piranha_assert(m_next);
				return static_cast<T *>(static_cast<void *>(&m_storage));
			}
			storage_type	m_storage;
			node		*m_next;
		};
		// List constituting the bucket.
		// NOTE: in this list implementation the m_next pointer is used as a flag to signal if the current node
		// stores an item: the pointer is not null if it does contain something. The value of m_next pointer in a node is set to a constant
		// &terminator node if it is the last node of the list. I.e., the terminator is end() in all cases
		// except when the list is empty (in that case the m_node member is end()).
		struct list
		{
			template <typename U>
			class iterator_impl: public boost::iterator_facade<iterator_impl<U>,U,boost::forward_traversal_tag>
			{
					typedef typename std::conditional<std::is_const<U>::value,node const *,node *>::type ptr_type;
					template <typename V>
					friend class iterator_impl;
				public:
					iterator_impl():m_ptr(nullptr) {}
					explicit iterator_impl(ptr_type ptr):m_ptr(ptr) {}
					// Constructor from other iterator type.
					template <typename V>
					iterator_impl(const iterator_impl<V> &other,
						typename std::enable_if<std::is_convertible<typename iterator_impl<V>::ptr_type,ptr_type>::value>::type * = nullptr):
						m_ptr(other.m_ptr)
					{}
				private:
					friend class boost::iterator_core_access;
					void increment()
					{
						piranha_assert(m_ptr && m_ptr->m_next);
						m_ptr = m_ptr->m_next;
					}
					template <typename V>
					bool equal(const iterator_impl<V> &other) const
					{
						return m_ptr == other.m_ptr;
					}
					U &dereference() const
					{
						piranha_assert(m_ptr && m_ptr->m_next);
						return *m_ptr->ptr();
					}
				public:
					ptr_type m_ptr;
			};
			typedef iterator_impl<T> iterator;
			typedef iterator_impl<T const> const_iterator;
			// Static checks on the iterator types.
			PIRANHA_TT_CHECK(is_forward_iterator,iterator);
			PIRANHA_TT_CHECK(is_forward_iterator,const_iterator);
			list() noexcept : m_node() {}
			list(list &&other) noexcept : m_node()
			{
				steal_from_rvalue(std::move(other));
			}
			list(const list &other):m_node()
			{
				try {
					auto cur = &m_node;
					auto other_cur = &other.m_node;
					while (other_cur->m_next) {
						if (cur->m_next) {
							// This assert means we are operating on the last element
							// of the list, as we are doing back-insertions.
							piranha_assert(cur->m_next == &terminator);
							// Create a new node with content equal to other_cur
							// and linking forward to the terminator.
							std::unique_ptr<node> new_node(::new node());
							::new ((void *)&new_node->m_storage) T(*other_cur->ptr());
							new_node->m_next = &terminator;
							// Link the new node.
							cur->m_next = new_node.release();
							cur = cur->m_next;
						} else {
							// This means this is the first node.
							::new ((void *)&cur->m_storage) T(*other_cur->ptr());
							cur->m_next = &terminator;
						}
						other_cur = other_cur->m_next;
					}
				} catch (...) {
					destroy();
					throw;
				}
			}
			list &operator=(list &&other)
			{
				if (likely(this != &other)) {
					// Destroy the content of this.
					destroy();
					steal_from_rvalue(std::move(other));
				}
				return *this;
			}
			list &operator=(const list &other)
			{
				if (likely(this != &other)) {
					auto tmp = other;
					*this = std::move(tmp);
				}
				return *this;
			}
			~list()
			{
				destroy();
			}
			void steal_from_rvalue(list &&other) noexcept
			{
				piranha_assert(empty());
				// Do something only if there is content in the other.
				if (other.m_node.m_next) {
					// Move construct current first node with first node of other.
					::new ((void *)&m_node.m_storage) T(std::move(*other.m_node.ptr()));
					// Link remaining content of other into this.
					m_node.m_next = other.m_node.m_next;
					// Destroy first node of other.
					other.m_node.ptr()->~T();
					other.m_node.m_next = nullptr;
				}
				piranha_assert(other.empty());
			}
			template <typename U>
			node *insert(U &&item, typename std::enable_if<std::is_same<T,typename std::decay<U>::type>::value>::type * = nullptr)
			{
				// NOTE: optimize with likely/unlikely?
				if (m_node.m_next) {
					// Create the new node and forward-link it to the second node.
					std::unique_ptr<node> new_node(::new node());
					::new ((void *)&new_node->m_storage) T(std::forward<U>(item));
					new_node->m_next = m_node.m_next;
					// Link first node to the new node.
					m_node.m_next = new_node.release();
					return m_node.m_next;
				} else {
					::new ((void *)&m_node.m_storage) T(std::forward<U>(item));
					m_node.m_next = &terminator;
					return &m_node;
				}
			}
			iterator begin()
			{
				return iterator(&m_node);
			}
			iterator end()
			{
				return iterator((m_node.m_next) ? &terminator : &m_node);
			}
			const_iterator begin() const
			{
				return const_iterator(&m_node);
			}
			const_iterator end() const
			{
				return const_iterator((m_node.m_next) ? &terminator : &m_node);
			}
			bool empty() const
			{
				return !m_node.m_next;
			}
			void destroy()
			{
				node *cur = &m_node;
				while (cur->m_next) {
					// Store the current value temporarily.
					auto old = cur;
					// Assign the next.
					cur = cur->m_next;
					// Destroy the old payload and erase connections.
					old->ptr()->~T();
					old->m_next = nullptr;
					// If the old node was not the initial one, delete it.
					if (old != &m_node) {
						::delete old;
					}
				}
				// After destruction, the list should be equivalent to a default-constructed one.
				piranha_assert(empty());
			}
			static node	terminator;
			node		m_node;
		};
		// Allocator type.
		// NOTE: if we move allocator choice in public interface we need to document the exception behaviour of the allocator. Also, check the validity
		// of the assumptions on the type returned by allocate(): must it be a pointer or just convertible to pointer?
		// NOTE: for std::allocator, pointer is guaranteed to be "T *":
		// http://en.cppreference.com/w/cpp/memory/allocator
		typedef std::allocator<list> allocator_type;
		// The container is a pointer to an array of lists.
		typedef list *container_type;
	public:
		/// Functor type for the calculation of hash values.
		typedef Hash hasher;
		/// Functor type for comparing the items in the set.
		typedef Pred key_equal;
		/// Key type.
		typedef T key_type;
		/// Size type.
		/**
		 * Alias for \p std::size_t.
		 */
		typedef std::size_t size_type;
	private:
		template <typename Key>
		class iterator_impl: public boost::iterator_facade<iterator_impl<Key>,Key,boost::forward_traversal_tag>
		{
				friend class hash_set;
				typedef typename std::conditional<std::is_const<Key>::value,hash_set const,hash_set>::type set_type;
				typedef typename std::conditional<std::is_const<Key>::value,typename list::const_iterator,typename list::iterator>::type it_type;
			public:
				iterator_impl():m_set(nullptr),m_idx(0u),m_it() {}
				explicit iterator_impl(set_type *set, const size_type &idx, it_type it):
					m_set(set),m_idx(idx),m_it(it) {}
			private:
				friend class boost::iterator_core_access;
				void increment()
				{
					piranha_assert(m_set);
					auto &container = m_set->m_container;
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
					return (m_set == other.m_set && m_idx == other.m_idx && m_it == other.m_it);
				}
				Key &dereference() const
				{
					piranha_assert(m_set && m_idx < m_set->bucket_count() &&
						m_it != m_set->m_container[m_idx].end());
					return *m_it;
				}
			private:
				set_type	*m_set;
				size_type	m_idx;
				it_type		m_it;
		};
		void init_from_n_buckets(const size_type &n_buckets, unsigned n_threads)
		{
			piranha_assert(!m_container && !m_log2_size && !m_n_elements);
			if (unlikely(!n_threads)) {
				piranha_throw(std::invalid_argument,"the number of threads must be strictly positive");
			}
			// Proceed to actual construction only if the requested number of buckets is nonzero.
			if (!n_buckets) {
				return;
			}
			const size_type log2_size = get_log2_from_hint(n_buckets);
			const size_type size = size_type(1u) << log2_size;
			auto new_ptr = m_allocator.allocate(size);
			if (unlikely(!new_ptr)) {
				piranha_throw(std::bad_alloc,);
			}
			if (n_threads == 1u) {
				// Default-construct the elements of the array.
				// NOTE: this is a noexcept operation, no need to account for rolling back.
				for (size_type i = 0u; i < size; ++i) {
					m_allocator.construct(&new_ptr[i]);
				}
			} else {
				// Sync variables.
				using crs_type = std::vector<std::pair<size_type,size_type>>;
				crs_type constructed_ranges(static_cast<typename crs_type::size_type>(n_threads),std::make_pair(0u,0u));
				if (unlikely(constructed_ranges.size() != n_threads)) {
					piranha_throw(std::bad_alloc,);
				}
				// Thread function.
				auto thread_function = [this,new_ptr,&constructed_ranges](const size_type &start, const size_type &end, const unsigned &thread_idx) {
					for (size_type i = start; i != end; ++i) {
						this->m_allocator.construct(&new_ptr[i]);
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
							m_allocator.destroy(&new_ptr[i]);
						}
					}
					// Deallocate before re-throwing.
					m_allocator.deallocate(new_ptr,size);
					throw;
				}
			}
			// Assign the members.
			m_container = new_ptr;
			m_log2_size = log2_size;
		}
		// Destroy all elements and deallocate m_container.
		void destroy_and_deallocate()
		{
			// Proceed to destroy all elements and deallocate only if the table is actually storing something.
			if (m_container) {
				const size_type size = size_type(1u) << m_log2_size;
				for (size_type i = 0u; i < size; ++i) {
					m_allocator.destroy(&m_container[i]);
				}
				m_allocator.deallocate(m_container,size);
			} else {
				piranha_assert(!m_log2_size && !m_n_elements);
			}
		}
	public:
		/// Iterator type.
		/**
		 * A read-only forward iterator.
		 */
		typedef iterator_impl<key_type const> iterator;
	private:
		// Static checks on the iterator type.
		PIRANHA_TT_CHECK(is_forward_iterator,iterator);
	public:
		/// Const iterator type.
		/**
		 * Equivalent to the iterator type.
		 */
		typedef iterator const_iterator;
		/// Local iterator.
		/**
		 * Const iterator that can be used to iterate through a single bucket.
		 */
		typedef typename list::const_iterator local_iterator;
		/// Default constructor.
		/**
		 * If not specified, it will default-initialise the hasher and the equality predicate. The resulting
		 * hash set will be empty.
		 * 
		 * @param[in] h hasher functor.
		 * @param[in] k equality predicate.
		 * 
		 * @throws unspecified any exception thrown by the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hash_set(const hasher &h = hasher(), const key_equal &k = key_equal()):
			m_container(nullptr),m_log2_size(0u),m_hasher(h),m_key_equal(k),m_n_elements(0u),m_allocator() {}
		/// Constructor from number of buckets.
		/**
		 * Will construct a table whose number of buckets is at least equal to \p n_buckets. If \p n_threads is not 1,
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
		explicit hash_set(const size_type &n_buckets, const hasher &h = hasher(), const key_equal &k = key_equal(), unsigned n_threads = 1u):
			m_container(nullptr),m_log2_size(0u),m_hasher(h),m_key_equal(k),m_n_elements(0u),m_allocator()
		{
			init_from_n_buckets(n_buckets,n_threads);
		}
		/// Copy constructor.
		/**
		 * @param[in] other piranha::hash_set that will be copied into \p this.
		 * 
		 * @throws unspecified any exception thrown by memory allocation errors,
		 * the copy constructor of the stored type, <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hash_set(const hash_set &other):
			m_container(nullptr),m_log2_size(0u),m_hasher(other.m_hasher),
			m_key_equal(other.m_key_equal),m_n_elements(0u),m_allocator(other.m_allocator)
		{
			// Proceed to actual copy only if other has some content.
			if (other.m_container) {
				const size_type size = size_type(1u) << other.m_log2_size;
				auto new_ptr = m_allocator.allocate(size);
				if (unlikely(!new_ptr)) {
					piranha_throw(std::bad_alloc,);
				}
				size_type i = 0u;
				try {
					// Copy-construct the elements of the array.
					for (; i < size; ++i) {
						m_allocator.construct(&new_ptr[i],other.m_container[i]);
					}
				} catch (...) {
					// Unwind the construction and deallocate, before re-throwing.
					for (size_type j = 0u; j < i; ++j) {
						m_allocator.destroy(&new_ptr[j]);
					}
					m_allocator.deallocate(new_ptr,size);
					throw;
				}
				// Assign the members.
				m_container = new_ptr;
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
		 * @param[in] other table to be moved.
		 */
		hash_set(hash_set &&other) noexcept(true) : m_container(other.m_container),m_log2_size(other.m_log2_size),
			m_hasher(std::move(other.m_hasher)),m_key_equal(std::move(other.m_key_equal)),m_n_elements(other.m_n_elements),
			m_allocator(std::move(other.m_allocator))
		{
			// Clear out the other one.
			other.m_container = nullptr;
			other.m_log2_size = 0u;
			other.m_n_elements = 0u;
		}
		/// Constructor from range.
		/**
		 * Create a table with a copy of a range.
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
		explicit hash_set(const InputIterator &begin, const InputIterator &end, const size_type &n_buckets = 0u,
			const hasher &h = hasher(), const key_equal &k = key_equal()):
			m_container(nullptr),m_log2_size(0u),m_hasher(h),m_key_equal(k),m_n_elements(0u),m_allocator()
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
		explicit hash_set(std::initializer_list<U> list):
			m_container(nullptr),m_log2_size(0u),m_hasher(),m_key_equal(),m_n_elements(0u),m_allocator()
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
		~hash_set()
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
		hash_set &operator=(const hash_set &other)
		{
			if (likely(this != &other)) {
				hash_set tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other table to be moved into \p this.
		 * 
		 * @return reference to \p this.
		 */
		hash_set &operator=(hash_set &&other) noexcept(true)
		{
			if (likely(this != &other)) {
				destroy_and_deallocate();
				m_container = other.m_container;
				m_log2_size = other.m_log2_size;
				m_hasher = std::move(other.m_hasher);
				m_key_equal = std::move(other.m_key_equal);
				m_n_elements = other.m_n_elements;
				m_allocator = std::move(other.m_allocator);
				// Zero out other.
				other.m_container = nullptr;
				other.m_log2_size = 0u;
				other.m_n_elements = 0u;
			}
			return *this;
		}
		/// Const begin iterator.
		/**
		 * @return hash_set::const_iterator to the first element of the table, or end() if the table is empty.
		 */
		const_iterator begin() const
		{
			// NOTE: this could take a while in case of an empty table with lots of buckets. Take a shortcut
			// taking into account the number of elements in the table - if zero, go directly to end()?
			const_iterator retval;
			retval.m_set = this;
			size_type idx = 0u;
			const auto b_count = bucket_count();
			for (; idx < b_count; ++idx) {
				if (!m_container[idx].empty()) {
					break;
				}
			}
			retval.m_idx = idx;
			// If we are not at the end, assign proper iterator.
			if (idx != b_count) {
				retval.m_it = m_container[idx].begin();
			}
			return retval;
		}
		/// Const end iterator.
		/**
		 * @return hash_set::const_iterator to the position past the last element of the table.
		 */
		const_iterator end() const
		{
			return const_iterator(this,bucket_count(),local_iterator{});
		}
		/// Begin iterator.
		/**
		 * @return hash_set::iterator to the first element of the table, or end() if the table is empty.
		 */
		iterator begin()
		{
			return static_cast<hash_set const *>(this)->begin();
		}
		/// End iterator.
		/**
		 * @return hash_set::iterator to the position past the last element of the table.
		 */
		iterator end()
		{
			return static_cast<hash_set const *>(this)->end();
		}
		/// Number of elements contained in the table.
		/**
		 * @return number of elements in the table.
		 */
		size_type size() const
		{
			return m_n_elements;
		}
		/// Test for empty table.
		/**
		 * @return \p true if size() returns 0, \p false otherwise.
		 */
		bool empty() const
		{
			return !size();
		}
		/// Number of buckets.
		/**
		 * @return number of buckets in the table.
		 */
		size_type bucket_count() const
		{
			return (m_container) ? (size_type(1u) << m_log2_size) : size_type(0u);
		}
		/// Load factor.
		/**
		 * @return <tt>(double)size() / bucket_count()</tt>, or 0 if the table is empty.
		 */
		double load_factor() const
		{
			const auto b_count = bucket_count();
			return (b_count) ? static_cast<double>(size()) / static_cast<double>(b_count) : 0.;
		}
		/// Index of destination bucket.
		/**
		 * Index to which \p k would belong, were it to be inserted into the table. The index of the
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
				piranha_throw(zero_division_error,"cannot calculate bucket index in an empty table");
			}
			return _bucket(k);
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 * 
		 * @return hash_set::const_iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 * 
		 * @throws unspecified any exception thrown by _find().
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
		 * @return hash_set::iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 * 
		 * @throws unspecified any exception thrown by _find().
		 */
		iterator find(const key_type &k)
		{
			return static_cast<const hash_set *>(this)->find(k);
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
		/// Insert element.
		/**
		 * This template is activated only if \p T and \p U are the same type, aside from cv qualifications and references.
		 * If no other key equivalent to \p k exists in the table, the insertion is successful and returns the <tt>(it,true)</tt>
		 * pair - where \p it is the position in the table into which the object has been inserted. Otherwise, the return value
		 * will be <tt>(it,false)</tt> - where \p it is the position of the existing equivalent object.
		 * 
		 * @param[in] k object that will be inserted into the table.
		 * 
		 * @return <tt>(hash_set::iterator,bool)</tt> pair containing an iterator to the newly-inserted object (or its existing
		 * equivalent) and the result of the operation.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - hash_set::key_type's copy constructor,
		 * - _find().
		 * @throws std::overflow_error if a successful insertion would result in size() exceeding the maximum
		 * value representable by type piranha::hash_set::size_type.
		 * @throws std::bad_alloc if the operation results in a resize of the table past an implementation-defined
		 * maximum number of buckets.
		 */
		template <typename U>
		std::pair<iterator,bool> insert(U &&k, typename std::enable_if<std::is_same<T,typename std::decay<U>::type>::value>::type * = nullptr)
		{
			auto b_count = bucket_count();
			// Handle the case of a table with no buckets.
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
			if (unlikely(m_n_elements == boost::integer_traits<size_type>::const_max)) {
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
		 * pointing to an element of the table.
		 * 
		 * Erasing an element invalidates all iterators pointing to elements in the same bucket
		 * as the erased element.
		 * 
		 * After the operation has taken place, the size() of the table will be decreased by one.
		 * 
		 * @param[in] it iterator to the element of the table to be removed.
		 * 
		 * @return iterator pointing to the element following \p it pior to the element being erased, or end() if
		 * no such element exists.
		 */
		iterator erase(const iterator &it)
		{
			piranha_assert(!empty());
			const auto b_it = _erase(it);
			iterator retval;
			retval.m_set = this;
			const auto b_count = bucket_count();
			// Travel to the next iterator if necessary.
			if (b_it == m_container[it.m_idx].end()) {
				size_type idx = it.m_idx + 1u;
				// Advance to the first non-empty bucket if necessary,
				// without going past the end of the table.
				for (; idx < b_count; ++idx) {
					if (!m_container[idx].empty()) {
						break;
					}
				}
				retval.m_idx = idx;
				// If we are not at the end, assign proper iterator.
				if (idx != b_count) {
					retval.m_it = m_container[idx].begin();
				}
			} else {
				retval.m_idx = it.m_idx;
				retval.m_it = b_it;
			}
			piranha_assert(m_n_elements);
			--m_n_elements;
			return retval;
		}
		/// Remove all elements.
		/**
		 * After this call, size() and bucket_count() will both return zero.
		 */
		void clear()
		{
			destroy_and_deallocate();
			// Reset the members.
			m_container = nullptr;
			m_log2_size = 0u;
			m_n_elements = 0u;
		}
		/// Swap content.
		/**
		 * Will use \p std::swap to swap hasher and equality predicate.
		 * 
		 * @param[in] other swap argument.
		 * 
		 * @throws unspecified any exception thrown by swapping hasher or equality predicate via \p std::swap.
		 */
		void swap(hash_set &other)
		{
			std::swap(m_container,other.m_container);
			std::swap(m_log2_size,other.m_log2_size);
			std::swap(m_hasher,other.m_hasher);
			std::swap(m_key_equal,other.m_key_equal);
			std::swap(m_n_elements,other.m_n_elements);
			std::swap(m_allocator,other.m_allocator);
		}
		/// Rehash table.
		/**
		 * Change the number of buckets in the table to at least \p new_size. No rehash is performed
		 * if rehashing would lead to exceeding the maximum load factor. If \p n_threads is not 1,
		 * then the first \p n_threads threads from piranha::thread_pool will be used concurrently during
		 * the rehash operation.
		 * 
		 * @param[in] new_size new desired number of buckets.
		 * @param[in] n_threads number of threads to use.
		 * 
		 * @throws std::invalid_argument if \p n_threads is zero.
		 * @throws unspecified any exception thrown by the constructor from number of buckets
		 * or by _unique_insert().
		 */
		void rehash(const size_type &new_size, unsigned n_threads = 1u)
		{
			if (unlikely(!n_threads)) {
				piranha_throw(std::invalid_argument,"the number of threads must be strictly positive");
			}
			// If rehash is requested to zero, do something only if there are no items stored in the table.
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
			// Create a new table with needed amount of buckets.
			hash_set new_table(new_size,m_hasher,m_key_equal,n_threads);
			try {
				const auto it_f = _m_end();
				for (auto it = _m_begin(); it != it_f; ++it) {
					const auto new_idx = new_table._bucket(*it);
					new_table._unique_insert(std::move(*it),new_idx);
				}
			} catch (...) {
				// Clear up both this and the new table upon any kind of error.
				clear();
				new_table.clear();
				throw;
			}
			// Retain the number of elements.
			new_table.m_n_elements = m_n_elements;
			// Clear the old table.
			clear();
			// Assign the new table.
			*this = std::move(new_table);
		}
		/// Get information on the sparsity of the table.
		/**
		 * @return an <tt>std::map<size_type,size_type></tt> in which the key is the number of elements
		 * stored in a bucket and the mapped type the number of buckets containing those many elements.
		 * 
		 * @throws unspecified any exception thrown by memory errors in standard containers.
		 */
		std::map<size_type,size_type> evaluate_sparsity() const
		{
			const auto it_f = m_container + bucket_count();
			std::map<size_type,size_type> retval;
			size_type counter;
			for (auto it = m_container; it != it_f; ++it) {
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
		/// Mutable iterator.
		/**
		 * This iterator type provides non-const access to the elements of the table. Please note that modifications
		 * to an existing element of the table might invalidate the relation between the element and its position in the table.
		 * After such modifications of one or more elements, the only valid operation is hash_set::clear() (destruction of the
		 * table before calling hash_set::clear() will lead to assertion failures in debug mode).
		 */
		typedef iterator_impl<key_type> _m_iterator;
		/// Mutable begin iterator.
		/**
		 * @return hash_set::_m_iterator to the beginning of the table.
		 */
		_m_iterator _m_begin()
		{
			// NOTE: this could take a while in case of an empty table with lots of buckets. Take a shortcut
			// taking into account the number of elements in the table - if zero, go directly to end()?
			const auto b_count = bucket_count();
			_m_iterator retval;
			retval.m_set = this;
			size_type idx = 0u;
			for (; idx < b_count; ++idx) {
				if (!m_container[idx].empty()) {
					break;
				}
			}
			retval.m_idx = idx;
			// If we are not at the end, assign proper iterator.
			if (idx != b_count) {
				retval.m_it = m_container[idx].begin();
			}
			return retval;
		}
		/// Mutable end iterator.
		/**
		 * @return hash_set::_m_iterator to the end of the table.
		 */
		_m_iterator _m_end()
		{
			return _m_iterator(this,bucket_count(),typename list::iterator{});
		}
		/// Insert unique element (low-level).
		/**
		 * This template is activated only if \p T and \p U are the same type, aside from cv qualifications and references.
		 * The parameter \p bucket_idx is the index of the destination bucket for \p k and, for a
		 * table with a nonzero number of buckets, must be equal to the output
		 * of bucket() before the insertion.
		 * 
		 * This method will not check if a key equivalent to \p k already exists in the table, it will not
		 * update the number of elements present in the table after the insertion, it will not resize
		 * the table in case the maximum load factor is exceeded, nor it will check
		 * if the value of \p bucket_idx is correct.
		 * 
		 * @param[in] k object that will be inserted into the table.
		 * @param[in] bucket_idx destination bucket for \p k.
		 * 
		 * @return iterator pointing to the newly-inserted element.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of hash_set::key_type or by memory allocation
		 * errors.
		 */
		template <typename U>
		iterator _unique_insert(U &&k, const size_type &bucket_idx,
			typename std::enable_if<std::is_same<T,typename std::decay<U>::type>::value>::type * = nullptr)
		{
			// Assert that key is not present already in the table.
			piranha_assert(find(std::forward<U>(k)) == end());
			// Assert bucket index is correct.
			piranha_assert(bucket_idx == _bucket(k));
			auto ptr = m_container[bucket_idx].insert(std::forward<U>(k));
			return iterator(this,bucket_idx,local_iterator(ptr));
		}
		/// Find element (low-level).
		/**
		 * Locate element in the table. The parameter \p bucket_idx is the index of the destination bucket for \p k and, for 
		 * a table with a nonzero number of buckets, must be equal to the output
		 * of bucket() before the insertion. This method will not check if the value of \p bucket_idx is correct.
		 * 
		 * @param[in] k element to be located.
		 * @param[in] bucket_idx index of the destination bucket for \p k.
		 * 
		 * @return hash_set::iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 */
		const_iterator _find(const key_type &k, const size_type &bucket_idx) const noexcept
		{
			// Assert bucket index is correct.
			piranha_assert(bucket_idx == _bucket(k) && bucket_idx < bucket_count());
			const auto &b = m_container[bucket_idx];
			const auto it_f = b.end();
			const_iterator retval(end());
			for (auto it = b.begin(); it != it_f; ++it) {
				if (m_key_equal(*it,k)) {
					retval.m_idx = bucket_idx;
					retval.m_it = it;
					break;
				}
			}
			return retval;
		}
		/// Index of destination bucket from hash value.
		/**
		 * Note that this method will not check if the number of buckets is zero.
		 * 
		 * @param[in] hash input hash value.
		 * 
		 * @return index of the destination bucket for an object with hash value \p hash.
		 */
		size_type _bucket_from_hash(const std::size_t &hash) const noexcept
		{
			piranha_assert(bucket_count());
			return hash % (size_type(1u) << m_log2_size);
		}
		/// Index of destination bucket (low-level).
		/**
		 * Equivalent to bucket(), with the exception that this method will not check
		 * if the number of buckets is zero.
		 * 
		 * @param[in] k input argument.
		 * 
		 * @return index of the destination bucket for \p k.
		 */
		size_type _bucket(const key_type &k) const noexcept
		{
			return _bucket_from_hash(m_hasher(k));
		}
		/// Force update of the number of elements.
		/**
		 * After this call, size() will return \p new_size regardless of the true number of elements in the table.
		 * 
		 * @param[in] new_size new table size.
		 */
		void _update_size(const size_type &new_size) noexcept
		{
			m_n_elements = new_size;
		}
		/// Increase bucket count.
		/**
		 * Increase the number of buckets to the next implementation-defined value.
		 * 
		 * @throws std::bad_alloc if the operation results in a resize of the table past an implementation-defined
		 * maximum number of buckets.
		 * @throws unspecified any exception thrown by rehash().
		 */
		void _increase_size()
		{
			if (unlikely(m_log2_size >= m_n_nonzero_sizes - 1u)) {
				piranha_throw(std::bad_alloc,);
			}
			// We must take care here: if the table has zero buckets,
			// the next log2_size is 0u. Otherwise increase current log2_size.
			piranha_assert(m_container || (!m_container && !m_log2_size));
			const auto new_log2_size = (m_container) ? (m_log2_size + 1u) : 0u;
			// Rehash to the new size.
			rehash(size_type(1u) << new_log2_size);
		}
		/// Const reference to list in bucket.
		/**
		 * @param[in] idx index of the bucket whose list will be returned.
		 * 
		 * @return a const reference to the list of items contained in the bucket positioned
		 * at index \p idx.
		 */
		const list &_get_bucket_list(const size_type &idx) const noexcept
		{
			piranha_assert(idx < bucket_count());
			return m_container[idx];
		}
		/// Erase element.
		/**
		 * Erase the element to which \p it points. \p it must be a valid iterator
		 * pointing to an element of the table.
		 * 
		 * Erasing an element invalidates all iterators pointing to elements in the same bucket
		 * as the erased element.
		 * 
		 * This method will not update the number of elements in the table, nor it will try to access elements
		 * outside the bucket to which \p it refers.
		 * 
		 * @param[in] it iterator to the element of the table to be removed.
		 * 
		 * @return local iterator pointing to the element following \p it pior to the element being erased, or local end() if
		 * no such element exists.
		 */
		local_iterator _erase(const iterator &it) noexcept
		{
			// Verify the iterator is valid.
			piranha_assert(it.m_set == this);
			piranha_assert(it.m_idx < bucket_count());
			piranha_assert(!m_container[it.m_idx].empty());
			piranha_assert(it.m_it != m_container[it.m_idx].end());
			auto &bucket = m_container[it.m_idx];
			// If the pointed-to element is the first one in the bucket, we need special care.
			if (&*it == &*bucket.m_node.ptr()) {
				// Destroy the payload.
				bucket.m_node.ptr()->~T();
				if (bucket.m_node.m_next == &bucket.terminator) {
					// Special handling if this was the only element.
					bucket.m_node.m_next = nullptr;
					return bucket.end();
				} else {
					// Store the link in the second element.
					auto tmp = bucket.m_node.m_next->m_next;
					// Move-construct from the second element, and then destroy it.
					::new ((void *)&bucket.m_node.m_storage) T(std::move(*bucket.m_node.m_next->ptr()));
					bucket.m_node.m_next->ptr()->~T();
					::delete bucket.m_node.m_next;
					// Establish the new link.
					bucket.m_node.m_next = tmp;
					return bucket.begin();
				}
			} else {
				auto b_it = bucket.begin();
				auto prev_b_it = b_it;
				const auto b_it_f = bucket.end();
				++b_it;
				for (; b_it != b_it_f; ++b_it, ++prev_b_it) {
					if (&*b_it == &*it) {
						// Assign to the previous element the next link of the current one.
						prev_b_it.m_ptr->m_next = b_it.m_ptr->m_next;
						// Delete the current one.
						b_it.m_ptr->ptr()->~T();
						::delete b_it.m_ptr;
						break;
					};
				}
				// We never want to go through the whole list, it means the element
				// to which 'it' refers is not here: assert that the iterator we just
				// erased was not end() - i.e., it was pointing to something.
				piranha_assert(b_it.m_ptr);
				// See if the erased iterator was the last one of the list.
				const auto tmp = prev_b_it;
				++prev_b_it;
				if (prev_b_it == b_it_f) {
					// Iterator is the last one, return local end().
					return b_it_f;
				} else {
					// Iterator is not the last one, return it.
					return tmp;
				}
			}
		}
		//@}
	private:
		// Run a consistency check on the table, will return false if something is wrong.
		bool sanity_check() const
		{
			// Ignore sanity checks on shutdown.
			if (environment::shutdown()) {
				return true;
			}
			size_type count = 0u;
			for (size_type i = 0u; i < bucket_count(); ++i) {
				for (auto it = m_container[i].begin(); it != m_container[i].end(); ++it) {
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
			if (!m_container && (m_log2_size || m_n_elements)) {
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
		// The number of available nonzero sizes will be the number of bits in the size type. Possible nonzero sizes will be in
		// the [2 ** 0, 2 ** (n-1)] range.
		static const size_type m_n_nonzero_sizes = static_cast<size_type>(std::numeric_limits<size_type>::digits);
		// Get log2 of table size at least equal to hint. To be used only when hint is not zero.
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
	private:
		container_type	m_container;
		size_type	m_log2_size;
		hasher		m_hasher;
		key_equal	m_key_equal;
		size_type	m_n_elements;
		allocator_type	m_allocator;
};

template <typename T, typename Hash, typename Pred>
typename hash_set<T,Hash,Pred>::node hash_set<T,Hash,Pred>::list::terminator;

template <typename T, typename Hash, typename Pred>
const typename hash_set<T,Hash,Pred>::size_type hash_set<T,Hash,Pred>::m_n_nonzero_sizes;

}

#endif
