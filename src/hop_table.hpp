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

#ifndef PIRANHA_HOP_TABLE_HPP
#define PIRANHA_HOP_TABLE_HPP

#include <algorithm>
#include <array>
#include <boost/aligned_storage.hpp>
#include <boost/integer_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/utility.hpp>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <new>
#include <unordered_set>
#include <utility>
#include <type_traits>

#include "config.hpp"
#include "cvector.hpp"
#include "exceptions.hpp"
#include "mf_int.hpp"


#include <iostream> // TODO: remove!!!!

namespace piranha
{

/// Hopscotch hash table.
/**
 * Hash table class based on hopscotch hashing.
 * 
 * @see http://en.wikipedia.org/wiki/Hopscotch_hashing
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo bucket type with no-op destructor for T with trivial dtor.
 * \todo try using pointers during table resize.
 */
template <typename T, typename Hash = std::hash<T>, typename Pred = std::equal_to<T>>
class hop_table
{
		struct hop_bucket
		{
			typedef typename boost::aligned_storage<sizeof(T),boost::alignment_of<T>::value>::type storage_type;
			static const mf_uint max_shift = mf_int_traits::nbits - static_cast<mf_uint>(1);
			static const mf_uint highest_bit = static_cast<mf_uint>(1) << max_shift;
			hop_bucket():m_occupied(false),m_bitset(0),m_storage() {}
			hop_bucket(const hop_bucket &other):m_occupied(false),m_bitset(other.m_bitset),m_storage()
			{
// std::cout << "copy bucket\n";
				if (other.m_occupied) {
// std::cout << "copy blup\n";
					new ((void *)&m_storage) T(*other.ptr());
					m_occupied = true;
				}
			}
			hop_bucket(hop_bucket &&other):m_occupied(false),m_bitset(other.m_bitset),m_storage()
			{
// std::cout << "move bucket\n";
				if (other.m_occupied) {
// std::cout << "move blup\n";
					new ((void *)&m_storage) T(std::move(*other.ptr()));
					m_occupied = true;
				}
			}
			hop_bucket &operator=(const hop_bucket &) = delete;
			hop_bucket &operator=(hop_bucket &&) = delete;
			T *ptr()
			{
				piranha_assert(m_occupied);
				return static_cast<T *>(static_cast<void *>(&m_storage));
			}
			const T *ptr() const
			{
				piranha_assert(m_occupied);
				return static_cast<const T *>(static_cast<const void *>(&m_storage));
			}
			bool none() const
			{
				return !m_bitset;
			}
			bool test(const mf_uint &idx) const
			{
				piranha_assert(idx < mf_int_traits::nbits);
				return m_bitset & (highest_bit >> idx);
			}
			void set(const mf_uint &idx)
			{
				piranha_assert(idx < mf_int_traits::nbits);
				m_bitset |= (highest_bit >> idx);
			}
			void toggle(const mf_uint &idx)
			{
				piranha_assert(idx < mf_int_traits::nbits);
				m_bitset ^= (highest_bit >> idx);
			}
			~hop_bucket()
			{
				if (m_occupied) {
					ptr()->~T();
				}
			}
			bool		m_occupied;
			mf_uint		m_bitset;
			storage_type	m_storage;
		};
		typedef piranha::cvector<hop_bucket> container_type;
	public:
		/// Functor type for the calculation of hash values.
		typedef Hash hasher;
		/// Functor type for comparing the items in the table.
		typedef Pred key_equal;
		/// Key type.
		typedef T key_type;
		/// Size type.
		typedef typename container_type::size_type size_type;
	private:
		class iterator_impl: public boost::iterator_facade<iterator_impl,key_type const,boost::forward_traversal_tag>
		{
				friend class hop_table;
			public:
				iterator_impl():m_table(piranha_nullptr),m_idx(0) {}
				explicit iterator_impl(const hop_table *table, const size_type &idx):m_table(table),m_idx(idx) {}
			private:
				friend class boost::iterator_core_access;
				void increment()
				{
					piranha_assert(m_table);
					const size_type container_size = m_table->m_container.size();
					do {
						++m_idx;
					} while (m_idx < container_size && !m_table->m_container[m_idx].m_occupied);
				}
				bool equal(const iterator_impl &other) const
				{
					piranha_assert(m_table && other.m_table);
					return (m_table == other.m_table && m_idx == other.m_idx);
				}
				const key_type &dereference() const
				{
					piranha_assert(m_table && m_idx < m_table->m_container.size() && m_table->m_container[m_idx].m_occupied);
					return *m_table->m_container[m_idx].ptr();
				}
			private:
				const hop_table	*m_table;
				size_type	m_idx;
		};
	public:
		/// Iterator type.
		typedef iterator_impl iterator;
		/// Const iterator type.
		/**
		 * Equivalent to the iterator type.
		 */
		typedef iterator const_iterator;
		/// Default constructor.
		/**
		 * If not specified, it will default-initialise the hasher and the equality predicate.
		 * 
		 * @param[in] h hasher functor.
		 * @param[in] k equality predicate.
		 * 
		 * @throws unspecified any exception thrown by the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hop_table(const hasher &h = hasher(), const key_equal &k = key_equal()):
			m_container(),m_hasher(h),m_key_equal(k),m_n_elements(0) {}
		/// Constructor from number of buckets.
		/**
		 * Will construct a table whose number of buckets is at least equal to \p n_buckets.
		 * 
		 * @param[in] n_buckets desired number of buckets.
		 * @param[in] h hasher functor.
		 * @param[in] k equality predicate.
		 * 
		 * @throws std::bad_alloc if the desired number of buckets is greater than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		explicit hop_table(const size_type &n_buckets, const hasher &h = hasher(), const key_equal &k = key_equal()):
			m_container(get_size_from_hint(n_buckets)),m_hasher(h),m_key_equal(k),m_n_elements(0) {}
		/// Copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructors of piranha::cvector, <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hop_table(const hop_table &) = default;
		/// Move constructor.
		/**
		 * After the move, \p other will have zero buckets and zero elements, and its hasher and equality predicate
		 * will have been used to copy-construct their counterparts in \p this.
		 * 
		 * @param[in] other table to be moved.
		 * 
		 * @throws unspecified any exception thrown by the copy constructors of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hop_table(hop_table &&other): m_container(),m_hasher(other.m_hasher),
			m_key_equal(other.m_key_equal),m_n_elements(other.m_n_elements)
		{
			// Assign it here, so that if hasher or equality throw in the initialiser list
			// we won't have fucked other.
			m_container = std::move(other.m_container);
			// Mark the other as empty, as other's cvector will be empty.
			other.m_n_elements = 0;
		}
		/// Destructor.
		/**
		 * No side effects.
		 */
		~hop_table()
		{
			piranha_assert(sanity_check());
		}
		/// Const begin iterator.
		/**
		 * @return hop_table::const_iterator to the first element of the table, or end() if the table is empty.
		 */
		const_iterator begin() const
		{
			const_iterator retval(this,0);
			// Go to the first occupied bucket.
			if (m_container.size() && !m_container[0].m_occupied) {
				retval.increment();
			}
			return retval;
		}
		/// Const end iterator.
		/**
		 * @return hop_table::const_iterator to the position past the last element of the table.
		 */
		const_iterator end() const
		{
			return const_iterator(this,m_container.size());
		}
		/// Begin iterator.
		/**
		 * @return hop_table::iterator to the first element of the table, or end() if the table is empty.
		 */
		iterator begin()
		{
			return static_cast<hop_table const *>(this)->begin();
		}
		/// End iterator.
		/**
		 * @return hop_table::iterator to the position past the last element of the table.
		 */
		iterator end()
		{
			return static_cast<hop_table const *>(this)->end();
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
		 * @return true if size() returns 0, false otherwise.
		 */
		bool empty() const
		{
			return !size();
		}
		/// Number of buckets.
		/**
		 * @return number of buckets in the table.
		 */
		size_type n_buckets() const
		{
			return m_container.size();
		}
		/// Index of first destination bucket.
		/**
		 * Index of the first bucket that would be taken into consideration during the insertion operation of \p k.
		 * Note that in hopscotch hashing it is not possible in general to establish beforehand the bucket into which
		 * \p k would effectively be placed without attempting an insertion operation.
		 * 
		 * @param[in] k input argument.
		 * 
		 * @return index of the first destination bucket for \p k.
		 * 
		 * @throws piranha::zero_division_error if size() is zero.
		 */
		size_type bucket(const key_type &k) const
		{
			if (unlikely(!m_container.size())) {
				piranha_throw(zero_division_error,"cannot calculate bucket index in an empty table");
			}
			return bucket_impl(k);
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 * 
		 * @return hop_table::const_iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 */
		const_iterator find(const key_type &k) const
		{
			if (unlikely(!m_container.size())) {
				return end();
			}
			return find_impl(k,bucket_impl(k));
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 * 
		 * @return hop_table::iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 */
		iterator find(const key_type &k)
		{
			return static_cast<const hop_table *>(this)->find(k);
		}
		/// Insert element.
		/**
		 * This template is activated only if \p U is implicitly convertible to \p T.
		 * If no other key equivalent to \p k exists in the table, the insertion is successful and returns the <tt>(it,true)</tt>
		 * pair - where \p it is the position in the table into which the object has been inserted. Otherwise, the return value
		 * will be <tt>(it,false)</tt> - where \p it is the position of the existing equivalent object.
		 * 
		 * @param[in] k object that will be inserted into the table.
		 * 
		 * @return <tt>(hop_table::iterator,bool)</tt> pair containing an iterator to the newly-inserted object (or its existing
		 * equivalent) and the result of the operation.
		 * 
		 * @throws unspecified any exception thrown by hop_table::key_type's move or copy constructors.
		 * @throws std::bad_alloc if the operation results in a resize of the table past an implementation-defined
		 * maximum number of buckets.
		 */
		template <typename U>
		std::pair<iterator,bool> insert(U &&k, typename boost::enable_if<std::is_convertible<U,T>>::type * = piranha_nullptr)
		{
			if (unlikely(!m_container.size())) {
				increase_size();
			}
			const auto bucket_idx = bucket_impl(k);
			const auto it = find_impl(k,bucket_idx);
			if (it != end()) {
				return std::make_pair(it,false);
			}
			auto ue_retval = _unique_insert(std::forward<U>(k),bucket_idx);
			while (unlikely(!ue_retval.second)) {
				increase_size();
				ue_retval = _unique_insert(std::forward<U>(k),bucket_impl(k));
			}
			++m_n_elements;
			return std::make_pair(ue_retval.first,true);
		}
		/// Insert unique element (low-level).
		/**
		 * This template is activated only if \p U is implicitly convertible to \p T.
		 * Insert \p k into the table.
		 * The parameter \p bucket_idx is the first-choice bucket for \p k and, for a non-empty table, must be equal to the output
		 * of bucket() before the insertion.
		 * This method will not check if a key equivalent to \p k already exists in the table, it will not
		 * update the number of elements present in the table after a successful insertion, nor it will check
		 * if the value of \p bucket_idx is correct.
		 * 
		 * If \p k can be inserted in the table without any resize operation, the insertion is successful and returns the <tt>(it,true)</tt>
		 * pair - where \p it is the position in the table into which the object has been inserted. Otherwise, the return value
		 * will be <tt>(end(),false)</tt>.
		 * 
		 * @param[in] k object that will be inserted into the table.
		 * @param[in] bucket_idx first-choice bucket for \p k.
		 * 
		 * @return <tt>(hop_table::iterator,bool)</tt> pair containing an iterator to the newly-inserted object (or
		 * end()) and the result of the operation.
		 * 
		 * @throws unspecified any exception thrown by hop_table::key_type's move or copy constructors.
		 */
		template <typename U>
		std::pair<iterator,bool> _unique_insert(U &&k, const size_type &bucket_idx, typename boost::enable_if<std::is_convertible<U,T>>::type * = piranha_nullptr)
		{
			const size_type container_size = m_container.size();
			if (unlikely(!container_size)) {
				// No free slot was found, need to resize.
// std::cout << "no free buckets\n";
				return std::make_pair(end(),false);
			}
			piranha_assert(bucket_idx == bucket_impl(k));
// std::cout << "original bucket index: " << bucket_idx << '\n';
			if (!m_container[bucket_idx].m_occupied) {
// std::cout << "found on 1st shot: " << bucket_idx << '\n';
				piranha_assert(!m_container[bucket_idx].test(0));
				new ((void *)&m_container[bucket_idx].m_storage) key_type(std::forward<U>(k));
				m_container[bucket_idx].m_occupied = true;
				m_container[bucket_idx].set(0);
				return std::make_pair(iterator(this,bucket_idx),true);
			}
			size_type alt_idx = bucket_idx + static_cast<size_type>(1);
			// Start the linear probe.
			for (; alt_idx < container_size; ++alt_idx) {
				if (!m_container[alt_idx].m_occupied) {
					break;
				}
			}
			if (alt_idx == container_size) {
				// No free slot was found, need to resize.
// std::cout << "no free buckets\n";
				return std::make_pair(end(),false);
			}
// std::cout << "found after linear probe\n";
			while (alt_idx - bucket_idx >= mf_int_traits::nbits) {
// std::cout << "need to do the hopscotch dance\n";
				const size_type orig_idx = alt_idx;
				// First let's try to move as back as possible.
				alt_idx -= hop_bucket::max_shift;
				int msb = mf_int_traits::msb(m_container[alt_idx].m_bitset), min_bit_pos = 1;
				piranha_assert(msb != 0);
				while (msb < min_bit_pos && alt_idx < orig_idx) {
// std::cout << "bling bling\n";
					++alt_idx;
					++min_bit_pos;
					msb = mf_int_traits::msb(m_container[alt_idx].m_bitset);
				}
				if (alt_idx == orig_idx) {
					// No free slot was found, need to resize.
// std::cout << "no free buckets\n";
					return std::make_pair(end(),false);
				}
				piranha_assert(msb > 0);
				piranha_assert(hop_bucket::max_shift >= static_cast<unsigned>(msb));
				const size_type next_idx = alt_idx + (hop_bucket::max_shift - static_cast<unsigned>(msb));
				piranha_assert(next_idx < orig_idx && next_idx >= alt_idx && orig_idx >= alt_idx);
				piranha_assert(m_container[alt_idx].test(next_idx - alt_idx));
				piranha_assert(!m_container[alt_idx].test(orig_idx - alt_idx));
				// Move the content of the target bucket to the empty slot.
				new ((void *)&m_container[orig_idx].m_storage)
					key_type(std::move(*m_container[next_idx].ptr()));
				// Destroy the object in the target bucket.
				m_container[next_idx].ptr()->~key_type();
				// Set the flags.
				m_container[orig_idx].m_occupied = true;
				m_container[next_idx].m_occupied = false;
				m_container[alt_idx].toggle(next_idx - alt_idx);
				m_container[alt_idx].toggle(orig_idx - alt_idx);
				piranha_assert(!m_container[alt_idx].test(next_idx - alt_idx));
				piranha_assert(m_container[alt_idx].test(orig_idx - alt_idx));
				// Set the new alt_idx.
				alt_idx = next_idx;
			}
// std::cout << "gonna write into: " << alt_idx << '\n';
			// The available slot is within the destination virtual bucket.
			piranha_assert(!m_container[alt_idx].m_occupied);
			piranha_assert(!m_container[bucket_idx].test(alt_idx - bucket_idx));
			new ((void *)&m_container[alt_idx].m_storage) key_type(std::forward<U>(k));
			m_container[alt_idx].m_occupied = true;
			m_container[bucket_idx].set(alt_idx - bucket_idx);
			return std::make_pair(iterator(this,alt_idx),true);
		}
	private:
		size_type bucket_impl(const key_type &k) const
		{
			piranha_assert(m_container.size());
			return m_hasher(k) % m_container.size();
		}
		const_iterator find_impl(const key_type &k, const size_type &bucket_idx) const
		{
			const size_type container_size = m_container.size();
			piranha_assert(container_size && bucket_idx == bucket_impl(k));
			const hop_bucket &b = m_container[bucket_idx];
			// Detect if the virtual bucket is empty.
			if (b.none()) {
// std::cout << "empty bitset\n";
				return end();
			}
			size_type next_idx = bucket_idx;
			// Walk through the virtual bucket's entries.
			for (mf_uint i = 0; i < mf_int_traits::nbits; ++i, ++next_idx) {
				// Do not try to examine buckets past the end.
				if (next_idx == container_size) {
					break;
				}
				piranha_assert(!b.test(i) || m_container[next_idx].m_occupied);
				if (b.test(i) && m_key_equal(*m_container[next_idx].ptr(),k))
				{
					return const_iterator(this,next_idx);
				}
			}
			return end();
		}
		// Run a consistency check on the table, will return false if something is wrong.
		bool sanity_check() const
		{
			size_type count = 0;
			for (size_type i = 0; i < m_container.size(); ++i) {
				for (mf_uint j = 0; j < std::min<mf_uint>(mf_int_traits::nbits,m_container.size() - i); ++j) {
					if (m_container[i].test(j)) {
						if (!m_container[i + j].m_occupied) {
// std::cout << "not occupied!!\n";
							return false;
						}
						if (bucket_impl(*m_container[i + j].ptr()) != i) {
// std::cout << "not hashed good!\n";
							return false;
						}
					}
				}
				if (m_container[i].m_occupied) {
					++count;
				}
			}
			if (count != m_n_elements) {
// std::cout << "inconsistent size!\n";
				return false;
			}
			if (m_container.size() != table_sizes[get_size_index()]) {
// std::cout << "inconsistent size index!\n";
				return false;
			}
			// Check size is consistent with number of iterator traversals.
			count = 0;
			for (auto it = begin(); it != end(); ++it, ++count) {}
			if (count != m_n_elements) {
// std::cout << "inconsistent number of iterator traversals!\n";
				return false;
			}
			return true;
		}
		static const std::size_t n_available_sizes =
#if defined(PIRANHA_64BIT_MODE)
			41;
#else
			33;
#endif
		typedef std::array<size_type,n_available_sizes> table_sizes_type;
		static const table_sizes_type table_sizes;
		// Increase table size at least to the next available size.
		void increase_size()
		{
// std::cout << "resize requested!!!\n";
			auto cur_size_index = get_size_index();
			if (unlikely(cur_size_index == n_available_sizes - static_cast<decltype(cur_size_index)>(1))) {
				throw std::bad_alloc();
			}
			++cur_size_index;
			std::list<hop_table> temp_tables;
			temp_tables.push_back(hop_table(table_sizes[cur_size_index],m_hasher,m_key_equal));
			try {
				decltype(_unique_insert(key_type(),0)) result;
				for (auto it = m_container.begin(); it != m_container.end(); ++it) {
					if (it->m_occupied) {
						do {
							result = temp_tables.back()._unique_insert(std::move(*(it->ptr())),temp_tables.back().bucket_impl(*(it->ptr())));
							temp_tables.back().m_n_elements += static_cast<size_type>(result.second);
							if (unlikely(!result.second)) {
std::cout << "ZOMGMOMGOMOGMGOOM\n";
								if (unlikely(cur_size_index == n_available_sizes - static_cast<decltype(cur_size_index)>(1))) {
									throw std::bad_alloc();
								}
								++cur_size_index;
								temp_tables.push_back(hop_table(table_sizes[cur_size_index],m_hasher,m_key_equal));
							}
						} while (unlikely(!result.second));
					}
				}
				piranha_assert(temp_tables.size() >= 1);
				while (unlikely(temp_tables.size() > 1)) {
					// Get the table before the last one.
					auto table_it = temp_tables.end();
					--(--table_it);
					for (auto it = table_it->m_container.begin(); it != table_it->m_container.end(); ++it) {
						if (it->m_occupied) {
							do {
								result = temp_tables.back()._unique_insert(std::move(*(it->ptr())),temp_tables.back().bucket_impl(*(it->ptr())));
								temp_tables.back().m_n_elements += static_cast<size_type>(result.second);
								if (unlikely(!result.second)) {
									if (unlikely(cur_size_index == n_available_sizes - static_cast<decltype(cur_size_index)>(1))) {
										throw std::bad_alloc();
									}
									++cur_size_index;
									temp_tables.push_back(hop_table(table_sizes[cur_size_index],m_hasher,m_key_equal));
								}
							} while (unlikely(!result.second));
						}
					}
					// The table before the last one was emptied, remove it.
					temp_tables.erase(table_it);
				}
			} catch (...) {
				// In face of exceptions, zero out the table and re-throw.
				m_container = container_type();
				m_n_elements = 0;
				throw;
			}
			piranha_assert(temp_tables.front().m_n_elements == m_n_elements);
			// Grab the payload from the temp table.
			m_container = std::move(temp_tables.front().m_container);
			temp_tables.front().m_n_elements = 0;
		}
		// Return the index in the table_sizes array of the current table size.
		std::size_t get_size_index() const
		{
			auto range = std::equal_range(table_sizes.begin(),table_sizes.end(),m_container.size());
			// Paranoia check.
			typedef typename std::iterator_traits<typename table_sizes_type::iterator>::difference_type difference_type;
			static_assert(
				static_cast<typename std::make_unsigned<difference_type>::type>(boost::integer_traits<difference_type>::const_max) >=
				n_available_sizes,"Overflow error."
			);
			piranha_assert(range.second - range.first == 1);
			return std::distance(table_sizes.begin(),range.first);
		}
		// Get table size at least equal to hint.
		static size_type get_size_from_hint(const size_type &hint)
		{
			auto it = std::lower_bound(table_sizes.begin(),table_sizes.end(),hint);
			if (it == table_sizes.end()) {
				throw std::bad_alloc();
			}
			piranha_assert(*it >= hint);
std::cout << "hint was: " << hint << ", size is: " << *it << '\n';
			return *it;
		}
	private:
		container_type	m_container;
		const hasher	m_hasher;
		const key_equal	m_key_equal;
		size_type	m_n_elements;
};

template <typename T, typename Hash, typename Pred>
const typename hop_table<T,Hash,Pred>::table_sizes_type hop_table<T,Hash,Pred>::table_sizes = { {
	0ull,
	1ull,
	3ull,
	5ull,
	11ull,
	23ull,
	53ull,
	97ull,
	193ull,
	389ull,
	769ull,
	1543ull,
	3079ull,
	6151ull,
	12289ull,
	24593ull,
	49157ull,
	98317ull,
	196613ull,
	393241ull,
	786433ull,
	1572869ull,
	3145739ull,
	6291469ull,
	12582917ull,
	25165843ull,
	50331653ull,
	100663319ull,
	201326611ull,
	402653189ull,
	805306457ull,
	1610612741ull,
	3221225473ull
#if defined(PIRANHA_64BIT_MODE)
	,
	6442450939ull,
	12884901893ull,
	25769803799ull,
	51539607551ull,
	103079215111ull,
	206158430209ull,
	412316860441ull,
	824633720831ull
#endif
} };

}

#endif
