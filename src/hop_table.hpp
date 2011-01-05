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
#include <boost/aligned_storage.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <functional>
#include <unordered_set>
#include <utility>

#include "config.hpp"
#include "cvector.hpp"
#include "mf_int.hpp"

namespace piranha
{

template <typename T, typename Hash = std::hash<T>, typename Pred = std::equal_to<T>>
class hop_table
{
		struct hop_bucket
		{
			typedef typename boost::aligned_storage<sizeof(T),boost::alignment_of<T>::value>::type storage_type;
			static const mf_uint max_shift = mf_int_traits::nbits - static_cast<mf_uint>(1);
			static const mf_uint highest_bit = static_cast<mf_uint>(1) << max_shift;
			hop_bucket():m_occupied(false),m_bitset(0),m_storage() {}
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
		typedef Hash hasher;
		typedef Pred key_equal;
		typedef T key_type;
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
		typedef iterator_impl iterator;
		typedef iterator const_iterator;
		hop_table(const hasher &h = hasher(), const key_equal &k = key_equal()):
			m_container(1543),m_hasher(h),m_key_equal(k),m_size(0) {}
		~hop_table()
		{
			piranha_assert(m_container.size());
			piranha_assert(sanity_check());
		}
		const_iterator begin() const
		{
			const_iterator retval(this,0);
			piranha_assert(m_container.size());
			// Go to the first occupied bucket.
			if (!m_container[0].m_occupied) {
				retval.increment();
			}
			return retval;
		}
		const_iterator end() const
		{
			return const_iterator(this,m_container.size());
		}
		iterator begin()
		{
			return static_cast<hop_table const *>(this)->begin();
		}
		iterator end()
		{
			return static_cast<hop_table const *>(this)->end();
		}
		size_type size() const
		{
			return m_size;
		}
		size_type bucket(const key_type &k) const
		{
			return m_hasher(k) % m_container.size();
		}
		const_iterator find(const key_type &k) const
		{
			piranha_assert(m_container.size());
			const size_type container_size = m_container.size(), bucket_idx = bucket(k);
			const hop_bucket &b = m_container[bucket_idx];
			// Detect if the virtual bucket is empty.
			if (b.none()) {
std::cout << "empty bitset\n";
				return end();
			}
			size_type next_idx = bucket_idx;
			// Walk through the virtual bucket's entries.
			for (mf_uint i = 0; i < mf_int_traits::nbits; ++i, ++next_idx) {
				// Do not try to examine buckets past the end.
				if (next_idx == container_size) {
					break;
				}
				// Start from the highest bit, descending.
				piranha_assert(!b.test(i) || m_container[next_idx].m_occupied);
				if (b.test(i) && m_key_equal(*m_container[next_idx].ptr(),k))
				{
					return const_iterator(this,next_idx);
				}
			}
			return end();
		}
		template <typename... Args>
		std::pair<iterator,bool> emplace(Args && ... params)
		{
			piranha_assert(m_container.size());
			key_type k(std::forward<Args>(params)...);
			static_assert(mf_int_traits::nbits > 0,"Invalid number of bits.");
			const size_type container_size = m_container.size(), bucket_idx = bucket(k);
std::cout << "original bucket index: " << bucket_idx << '\n';
			if (!m_container[bucket_idx].m_occupied) {
std::cout << "found on 1st shot: " << bucket_idx << '\n';
				piranha_assert(!m_container[bucket_idx].test(0));
				new ((void *)&m_container[bucket_idx].m_storage) key_type(std::move(k));
				m_container[bucket_idx].m_occupied = true;
				m_container[bucket_idx].set(0);
				++m_size;
				return std::make_pair(iterator(this,bucket_idx),true);
			}
			size_type alt_idx = bucket_idx + 1;
			// Start the linear probe.
			for (; alt_idx < container_size; ++alt_idx) {
				if (!m_container[alt_idx].m_occupied) {
					break;
				}
			}
			if (alt_idx == container_size) {
				// No free slot was found, need to resize.
				// TODO
std::cout << "no free buckets\n";
				std::exit(1);
			}
std::cout << "found after linear probe\n";
			while (alt_idx - bucket_idx >= mf_int_traits::nbits) {
std::cout << "need to do the hopscotch dance\n";
				const size_type orig_idx = alt_idx;
				// First let's try to move as back as possible.
				alt_idx -= hop_bucket::max_shift;
				int msb = mf_int_traits::msb(m_container[alt_idx].m_bitset), min_bit_pos = 1;
				piranha_assert(msb != 0);
				while (msb < min_bit_pos && alt_idx < orig_idx) {
std::cout << "bling bling\n";
					++alt_idx;
					++min_bit_pos;
					msb = mf_int_traits::msb(m_container[alt_idx].m_bitset);
				}
				if (alt_idx == orig_idx) {
					// No free slot was found, need to resize.
					// TODO
std::cout << "no free buckets\n";
					std::exit(1);
				}
				piranha_assert(msb > 0);
				const size_type next_idx = alt_idx + (hop_bucket::max_shift - static_cast<unsigned>(msb));
				piranha_assert(next_idx < orig_idx);
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
std::cout << "gonna write into: " << alt_idx << '\n';
			// The available slot is within the destination virtual bucket.
			piranha_assert(!m_container[alt_idx].m_occupied);
			piranha_assert(!m_container[bucket_idx].test(alt_idx - bucket_idx));
			new ((void *)&m_container[alt_idx].m_storage) key_type(std::move(k));
			m_container[alt_idx].m_occupied = true;
			m_container[bucket_idx].set(alt_idx - bucket_idx);
			++m_size;
			return std::make_pair(iterator(this,alt_idx),true);
		}
	private:
		// Run a consistency check on the table, will return false if something is wrong.
		bool sanity_check() const
		{
			size_type count = 0;
			for (size_type i = 0; i < m_container.size(); ++i) {
				for (mf_uint j = 0; j < std::min<mf_uint>(mf_int_traits::nbits,m_container.size() - i); ++j) {
					if (m_container[i].test(j)) {
						if (!m_container[i + j].m_occupied) {
std::cout << "not occupied!!\n";
							return false;
						}
						if (bucket(*m_container[i + j].ptr()) != i) {
std::cout << "not hashed good!\n";
							return false;
						}
					}
				}
				if (m_container[i].m_occupied) {
					++count;
				}
			}
			if (count != m_size) {
std::cout << "inconsistent size!\n";
				return false;
			}
			return true;
		}
	private:
		container_type	m_container;
		const hasher	m_hasher;
		const key_equal	m_key_equal;
		size_type	m_size;
};

}

#endif
