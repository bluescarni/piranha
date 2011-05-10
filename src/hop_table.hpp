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
#include <boost/integer_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <new>
#include <unordered_set>
#include <utility>
#include <type_traits>

#include "concepts/container_element.hpp"
#include "config.hpp"
#include "cvector.hpp"
#include "debug_access.hpp"
#include "exceptions.hpp"
#include "mf_int.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Hopscotch hash table.
/**
 * \image html hopscotch.gif "Hopscotch hashing."
 * \image latex hopscotch.png "Hopscotch hashing." width=5cm
 * 
 * Hash table class based on hopscotch hashing. The interface is similar to \p std::unordered_set.
 * 
 * @see http://en.wikipedia.org/wiki/Hopscotch_hashing
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must be a model of piranha::concept::ContainerElement. \p Hash and \p Pred must model the
 * concepts in the standard C++ library for the corresponding types of \p std::unordered_set.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations apart from the insertion methods,
 * which provide the following guarantee: the table will be either left in the same state as it was before the attempted
 * insertion or it will be emptied.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object equivalent to an empty table whose hasher and
 * equality predicate have been moved-from.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo concept assert for hash and pred. Require non-throwing swap, comparison and hashing operators and modify docs accordingly.
 * \todo tests for low-level methods
 * \todo try forcing 32-bit bitmap insted of mf_int
 * \todo see if find() can be sped up by using msb() instead of linear search
 */
template <typename T, typename Hash = std::hash<T>, typename Pred = std::equal_to<T>>
class hop_table
{
		BOOST_CONCEPT_ASSERT((concept::ContainerElement<T>));
		// Make friend with debug access class.
		template <typename U>
		friend class debug_access;
		template <typename U>
		struct base_generic_hop_bucket
		{
			// NOTE: here the bitset is read from msb to lsb. lsb is reserved for the flag
			// that signals if the bucket is occupied, leaving a size of nbits - 1 for the virtual
			// bucket.
			typedef typename std::aligned_storage<sizeof(U),alignof(U)>::type storage_type;
			static const mf_uint n_eff_bits = mf_int_traits::nbits - static_cast<mf_uint>(1);
			static const mf_uint max_shift = n_eff_bits - static_cast<mf_uint>(1);
			static const mf_uint highest_bit = static_cast<mf_uint>(1) << n_eff_bits;
			U *ptr()
			{
				piranha_assert(test_occupied());
				return static_cast<U *>(static_cast<void *>(&m_storage));
			}
			const U *ptr() const
			{
				piranha_assert(test_occupied());
				return static_cast<const U *>(static_cast<const void *>(&m_storage));
			}
			bool none() const
			{
				// Suppress the lsb as it is used for the occupied flag.
				return !(m_bitset >> static_cast<unsigned>(1));
			}
			bool test(const mf_uint &idx) const
			{
				piranha_assert(idx < n_eff_bits);
				return m_bitset & (highest_bit >> idx);
			}
			void set(const mf_uint &idx)
			{
				piranha_assert(idx < n_eff_bits);
				m_bitset |= (highest_bit >> idx);
			}
			void toggle(const mf_uint &idx)
			{
				piranha_assert(idx < n_eff_bits);
				m_bitset ^= (highest_bit >> idx);
			}
			bool test_occupied() const
			{
				return (m_bitset & static_cast<mf_uint>(1));
			}
			void set_occupied()
			{
				m_bitset |= static_cast<mf_uint>(1);
			}
			void toggle_occupied()
			{
				m_bitset ^= static_cast<mf_uint>(1);
			}
			storage_type	m_storage;
			mf_uint		m_bitset;
		};
		template <typename U, typename Enable = void>
		struct generic_hop_bucket: base_generic_hop_bucket<U>
		{
			// NOTE: no need to init the storage space, it will be inited
			// when constructing the objects in-place.
			generic_hop_bucket()
			{
				this->m_bitset = 0;
			}
			generic_hop_bucket(const generic_hop_bucket &other)
			{
				this->m_bitset = other.m_bitset;
				if (this->test_occupied()) {
					::new ((void *)&this->m_storage) U(*other.ptr());
				}
			}
			// NOTE: this should be called only in the default constructor of cvector,
			// so assert that other is empty.
			generic_hop_bucket(generic_hop_bucket &&other) piranha_noexcept_spec(true)
			{
				this->m_bitset = other.m_bitset;
				piranha_assert(!other.m_bitset);
			}
			// This should never be called, define it only to satisfy concept.
			generic_hop_bucket &operator=(generic_hop_bucket &&) piranha_noexcept_spec(true)
			{
				piranha_assert(false);
			}
			// Delete unused operators.
			generic_hop_bucket &operator=(const generic_hop_bucket &) = delete;
			~generic_hop_bucket() piranha_noexcept_spec(true)
			{
				if (this->test_occupied()) {
					this->ptr()->~U();
				}
			}
		};
		// NOTE: the rationale here is that for trivially-copyable and trivially-destructible
		// objects, the bucket can behave like a POD type, which lets optimizations in cvector kick in.
		template <typename U>
		struct generic_hop_bucket<U, typename std::enable_if<
			std::has_trivial_destructor<U>::value && std::has_trivial_copy_constructor<U>::value
			>::type>: base_generic_hop_bucket<U>
		{};
		typedef generic_hop_bucket<T> hop_bucket;
		// The numerical value here is kind of empirical/rule of thumb.
		typedef piranha::cvector<hop_bucket,5000000> container_type;
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
		template <typename Key>
		class iterator_impl: public boost::iterator_facade<iterator_impl<Key>,Key,boost::forward_traversal_tag>
		{
				friend class hop_table;
				typedef typename std::conditional<std::is_const<Key>::value,hop_table const,hop_table>::type table_type;
			public:
				iterator_impl():m_table(piranha_nullptr),m_idx(0) {}
				explicit iterator_impl(table_type *table, const size_type &idx):m_table(table),m_idx(idx) {}
			private:
				friend class boost::iterator_core_access;
				void increment()
				{
					piranha_assert(m_table);
					const size_type container_size = m_table->m_container.size();
					do {
						++m_idx;
					} while (m_idx < container_size && !m_table->m_container[m_idx].test_occupied());
				}
				bool equal(const iterator_impl &other) const
				{
					piranha_assert(m_table && other.m_table);
					return (m_table == other.m_table && m_idx == other.m_idx);
				}
				Key &dereference() const
				{
					piranha_assert(m_table && m_idx < m_table->m_container.size() && m_table->m_container[m_idx].test_occupied());
					return *m_table->m_container[m_idx].ptr();
				}
			private:
				table_type	*m_table;
				size_type	m_idx;
		};
	public:
		/// Iterator type.
		typedef iterator_impl<key_type const> iterator;
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
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructors of piranha::cvector, <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hop_table(const hop_table &) = default;
		/// Move constructor.
		/**
		 * After the move, \p other will have zero buckets and zero elements, and its hasher and equality predicate
		 * will have been used to move-construct their counterparts in \p this.
		 * 
		 * @param[in] other table to be moved.
		 */
		hop_table(hop_table &&other) piranha_noexcept_spec(true) : m_container(std::move(other.m_container)),m_hasher(std::move(other.m_hasher)),
			m_key_equal(std::move(other.m_key_equal)),m_n_elements(std::move(other.m_n_elements))
		{
			// Mark the other as empty, as other's cvector will be empty.
			other.m_n_elements = 0;
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
		explicit hop_table(const InputIterator &begin, const InputIterator &end, const size_type &n_buckets = 0,
			const hasher &h = hasher(), const key_equal &k = key_equal()):
			m_container(get_size_from_hint(n_buckets)),m_hasher(h),m_key_equal(k),m_n_elements(0)
		{
			for (auto it = begin; it != end; ++it) {
				insert(*it);
			}
		}
		/// Constructor from initializer list.
		/**
		 * Will insert() all the elements of the initializer list, ignoring the return value of the operation.
		 * Hash functor and equality predicate will be default-constructed. \p U must be implicitly convertible
		 * to hop_table::key_type.
		 * 
		 * @param[in] list initializer list of elements to be inserted.
		 * 
		 * @throws std::bad_alloc if the desired number of buckets is greater than an implementation-defined maximum.
		 * @throws boost::bad_numeric_cast if converting from the type of <tt>list.size()</tt> to hop_table::size_type results in a positive overflow.
		 * @throws unspecified any exception thrown by either insert() or of the default constructor of <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		template <typename U>
		hop_table(std::initializer_list<U> list):m_container(get_size_from_hint(boost::numeric_cast<size_type>(list.size()))),m_hasher(),m_key_equal(),m_n_elements(0)
		{
			for (auto it = list.begin(); it != list.end(); ++it) {
				insert(*it);
			}
		}
		/// Destructor.
		/**
		 * No side effects.
		 */
		~hop_table() piranha_noexcept_spec(true)
		{
			piranha_assert(sanity_check());
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructors of piranha::cvector, <tt>Hash</tt> or <tt>Pred</tt>.
		 */
		hop_table &operator=(const hop_table &other)
		{
			if (likely(this != &other)) {
				hop_table tmp(other);
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
		hop_table &operator=(hop_table &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				m_hasher = std::move(other.m_hasher);
				m_key_equal = std::move(other.m_key_equal);
				m_n_elements = std::move(other.m_n_elements);
				m_container = std::move(other.m_container);
				// Zero out other.
				other.m_n_elements = 0;
			}
			return *this;
		}
		/// Const begin iterator.
		/**
		 * @return hop_table::const_iterator to the first element of the table, or end() if the table is empty.
		 */
		const_iterator begin() const
		{
			const_iterator retval(this,0);
			// Go to the first occupied bucket.
			if (m_container.size() && !m_container[0].test_occupied()) {
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
		size_type n_buckets() const
		{
			return m_container.size();
		}
		/// Load factor.
		/**
		 * @return <tt>size() / n_buckets()</tt>.
		 * 
		 * @throws piranha::zero_division_error if the table is empty.
		 */
		double load_factor() const
		{
			if (unlikely(!m_container.size())) {
				piranha_throw(zero_division_error,"number of buckets is zero");
			}
			return static_cast<double>(m_n_elements) / m_container.size();
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
		 * @throws piranha::zero_division_error if n_buckets() returns zero.
		 * @throws unspecified any exception thrown by _bucket().
		 */
		size_type bucket(const key_type &k) const
		{
			if (unlikely(!m_container.size())) {
				piranha_throw(zero_division_error,"cannot calculate bucket index in an empty table");
			}
			return _bucket(k);
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 * 
		 * @return hop_table::const_iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 * 
		 * @throws unspecified any exception thrown by _find().
		 */
		const_iterator find(const key_type &k) const
		{
			if (unlikely(!m_container.size())) {
				return end();
			}
			return _find(k,_bucket(k));
		}
		/// Find element.
		/**
		 * @param[in] k element to be located.
		 * 
		 * @return hop_table::iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 * 
		 * @throws unspecified any exception thrown by _find().
		 */
		iterator find(const key_type &k)
		{
			return static_cast<const hop_table *>(this)->find(k);
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
		 * @return <tt>(hop_table::iterator,bool)</tt> pair containing an iterator to the newly-inserted object (or its existing
		 * equivalent) and the result of the operation.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - hop_table::key_type's copy constructor,
		 * - _find().
		 * @throws std::bad_alloc if the operation results in a resize of the table past an implementation-defined
		 * maximum number of buckets.
		 */
		template <typename U>
		std::pair<iterator,bool> insert(U &&k, typename std::enable_if<std::is_same<T,typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			if (unlikely(!m_container.size())) {
				increase_size();
			}
			const auto bucket_idx = _bucket(k), it = _find(k,bucket_idx);
			if (it != end()) {
				return std::make_pair(it,false);
			}
			auto ue_retval = _unique_insert(std::forward<U>(k),bucket_idx);
			while (unlikely(!ue_retval.second)) {
				increase_size();
				ue_retval = _unique_insert(std::forward<U>(k),_bucket(k));
			}
			++m_n_elements;
			return std::make_pair(ue_retval.first,true);
		}
		/// Erase element.
		/**
		 * Erase the element to which \p it points. \p it must be a valid iterator
		 * pointing to an element of the table.
		 * 
		 * Erasing an element does not invalidate any iterator, apart from the one pointing
		 * to the erased element.
		 * 
		 * @param[in] it iterator to the element of the table to be removed.
		 * 
		 * @throws unspecified any exception thrown by _bucket().
		 */
		void erase(const iterator &it)
		{
			piranha_assert(!empty() && it.m_table == this && it.m_idx < m_container.size());
			piranha_assert(m_container[it.m_idx].test_occupied());
			// Find the original destination bucket.
			const auto bucket_idx = _bucket(*it);
			piranha_assert(it.m_idx >= bucket_idx && it.m_idx - bucket_idx < hop_bucket::n_eff_bits);
			// Destroy the object stored in the iterator position.
			m_container[it.m_idx].ptr()->~key_type();
			m_container[it.m_idx].toggle_occupied();
			// Flip the bucket flag.
			m_container[bucket_idx].toggle(it.m_idx - bucket_idx);
			// Update the number of elements.
			piranha_assert(m_n_elements);
			--m_n_elements;
		}
		/// Remove all elements.
		/**
		 * After this call, size() and n_buckets() will both return zero.
		 */
		void clear()
		{
			m_container = container_type();
			m_n_elements = 0;
		}
		/// Swap content.
		/**
		 * Will use \p std::swap to swap hasher and equality predicate.
		 * 
		 * @param[in] other swap argument.
		 * 
		 * @throws unspecified any exception thrown by swapping hasher or equality predicate via \p std::swap.
		 */
		void swap(hop_table &other)
		{
			m_container.swap(other.m_container);
			std::swap(m_hasher,other.m_hasher);
			std::swap(m_key_equal,other.m_key_equal);
			std::swap<size_type>(m_n_elements,other.m_n_elements);
		}
		/** @name Low-level interface
		 * Low-level methods and types.
		 */
		//@{
		/// Mutable iterator.
		/**
		 * This iterator type provides non-const access to the elements of the table. Please note that modifications
		 * to an existing element of the table might invalidate the relation between the element and its position in the table.
		 * After such modifications of one or more elements, the only valid operation is hop_table::clear() (destruction of the
		 * table before calling hop_table::clear() will lead to assertion failures in debug mode).
		 */
		typedef iterator_impl<key_type> _m_iterator;
		/// Mutable begin iterator.
		/**
		 * @return hop_table::_m_iterator to the beginning of the table.
		 */
		_m_iterator _m_begin()
		{
			_m_iterator retval(this,0);
			if (m_container.size() && !m_container[0].test_occupied()) {
				retval.increment();
			}
			return retval;
		}
		/// Mutable end iterator.
		/**
		 * @return hop_table::_m_iterator to the end of the table.
		 */
		_m_iterator _m_end()
		{
			return _m_iterator(this,m_container.size());
		}
		/// Insert unique element (low-level).
		/**
		 * This template is activated only if \p T and \p U are the same type, aside from cv qualifications and references.
		 * The parameter \p bucket_idx is the first-choice bucket for \p k and, for a
		 * table with a positive number of buckets, must be equal to the output
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
		 * @throws unspecified any exception thrown by hop_table::key_type's copy constructor.
		 */
		template <typename U>
		std::pair<iterator,bool> _unique_insert(U &&k, const size_type &bucket_idx,
			typename std::enable_if<std::is_same<T,typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			// Assert that key is not present already in the table.
			piranha_assert(find(std::forward<U>(k)) == end());
			const size_type container_size = m_container.size();
			if (unlikely(!container_size)) {
				// No free slot was found, need to resize.
				return std::make_pair(end(),false);
			}
			piranha_assert(bucket_idx == _bucket(k));
			if (!m_container[bucket_idx].test_occupied()) {
				piranha_assert(!m_container[bucket_idx].test(0));
				::new ((void *)&m_container[bucket_idx].m_storage) key_type(std::forward<U>(k));
				m_container[bucket_idx].set_occupied();
				m_container[bucket_idx].set(0);
				return std::make_pair(iterator(this,bucket_idx),true);
			}
			size_type alt_idx = bucket_idx + static_cast<size_type>(1);
			// Start the linear probe.
			for (; alt_idx < container_size; ++alt_idx) {
				if (!m_container[alt_idx].test_occupied()) {
					break;
				}
			}
			if (alt_idx == container_size) {
				// No free slot was found, need to resize.
				return std::make_pair(end(),false);
			}
			while (alt_idx - bucket_idx >= hop_bucket::n_eff_bits) {
				const size_type orig_idx = alt_idx;
				// First let's try to move as back as possible.
				alt_idx -= hop_bucket::max_shift;
				int msb = mf_int_traits::msb(m_container[alt_idx].m_bitset), min_bit_pos = 2;
				// Msb cannot be in index 1 because that is the empty bucket we are starting from.
				piranha_assert(msb != 1);
				while (msb < min_bit_pos && alt_idx < orig_idx) {
					++alt_idx;
					++min_bit_pos;
					msb = mf_int_traits::msb(m_container[alt_idx].m_bitset);
				}
				if (alt_idx == orig_idx) {
					// No free slot was found, need to resize.
					return std::make_pair(end(),false);
				}
				piranha_assert(msb > 0);
				// NOTE: here we have to take always msb - 1 because the lsb does not count for bucket
				// indexing, as it is used for the occupied flag.
				piranha_assert(hop_bucket::max_shift >= static_cast<unsigned>(msb - 1));
				const size_type next_idx = alt_idx + (hop_bucket::max_shift - static_cast<unsigned>(msb - 1));
				piranha_assert(next_idx < orig_idx && next_idx >= alt_idx && orig_idx >= alt_idx);
				piranha_assert(m_container[alt_idx].test(next_idx - alt_idx));
				piranha_assert(!m_container[alt_idx].test(orig_idx - alt_idx));
				// Move the content of the target bucket to the empty slot.
				::new ((void *)&m_container[orig_idx].m_storage)
					key_type(std::move(*m_container[next_idx].ptr()));
				// Destroy the object in the target bucket.
				m_container[next_idx].ptr()->~key_type();
				// Set the flags.
				m_container[orig_idx].toggle_occupied();
				m_container[next_idx].toggle_occupied();
				m_container[alt_idx].toggle(next_idx - alt_idx);
				m_container[alt_idx].toggle(orig_idx - alt_idx);
				piranha_assert(!m_container[alt_idx].test(next_idx - alt_idx));
				piranha_assert(m_container[alt_idx].test(orig_idx - alt_idx));
				// Set the new alt_idx.
				alt_idx = next_idx;
			}
			// The available slot is within the destination virtual bucket.
			piranha_assert(!m_container[alt_idx].test_occupied());
			piranha_assert(!m_container[bucket_idx].test(alt_idx - bucket_idx));
			::new ((void *)&m_container[alt_idx].m_storage) key_type(std::forward<U>(k));
			m_container[alt_idx].set_occupied();
			m_container[bucket_idx].set(alt_idx - bucket_idx);
			return std::make_pair(iterator(this,alt_idx),true);
		}
		/// Find element (low-level).
		/**
		 * Locate element in the table. The parameter \p bucket_idx is the first-choice bucket for \p k and, for a non-empty table, must be equal to the output
		 * of bucket() before the insertion. This method will not check if the value of \p bucket_idx is correct.
		 * 
		 * @param[in] k element to be located.
		 * @param[in] bucket_idx first-choice bucket for \p k.
		 * 
		 * @return hop_table::iterator to <tt>k</tt>'s position in the table, or end() if \p k is not in the table.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - _bucket(),
		 * - the comparison functor.
		 */
		const_iterator _find(const key_type &k, const size_type &bucket_idx) const
		{
			const size_type container_size = m_container.size();
			piranha_assert(container_size && bucket_idx == _bucket(k));
			const hop_bucket &b = m_container[bucket_idx];
			// Detect if the virtual bucket is empty.
			if (b.none()) {
				return end();
			}
			size_type next_idx = bucket_idx;
			// Walk through the virtual bucket's entries.
			for (mf_uint i = 0; i < hop_bucket::n_eff_bits; ++i, ++next_idx) {
				// Do not try to examine buckets past the end.
				if (next_idx == container_size) {
					break;
				}
				piranha_assert(!b.test(i) || m_container[next_idx].test_occupied());
				if (b.test(i) && m_key_equal(*m_container[next_idx].ptr(),k))
				{
					return const_iterator(this,next_idx);
				}
			}
			return end();
		}
		/// Index of first destination bucket (low-level).
		/**
		 * Equivalent to bucket(), with the exception that this method will not check
		 * if the number of buckets is zero. In such a case, an integral division by zero will occur.
		 * 
		 * @param[in] k input argument.
		 * 
		 * @return index of the first destination bucket for \p k.
		 * 
		 * @throws unspecified any exception thrown by the hashing functor.
		 */
		size_type _bucket(const key_type &k) const
		{
			piranha_assert(m_container.size());
			return m_hasher(k) % m_container.size();
		}
		//@}
	private:
		// Run a consistency check on the table, will return false if something is wrong.
		bool sanity_check() const
		{
			size_type count = 0;
			for (size_type i = 0; i < m_container.size(); ++i) {
				for (mf_uint j = 0; j < std::min<mf_uint>(hop_bucket::n_eff_bits,m_container.size() - i); ++j) {
					if (m_container[i].test(j)) {
						if (!m_container[i + j].test_occupied()) {
							return false;
						}
						if (_bucket(*m_container[i + j].ptr()) != i) {
							return false;
						}
					}
				}
				if (m_container[i].test_occupied()) {
					++count;
				}
			}
			if (count != m_n_elements) {
				return false;
			}
			if (m_container.size() != table_sizes[get_size_index()]) {
				return false;
			}
			// Check size is consistent with number of iterator traversals.
			count = 0;
			for (auto it = begin(); it != end(); ++it, ++count) {}
			if (count != m_n_elements) {
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
			auto cur_size_index = get_size_index();
			if (unlikely(cur_size_index == n_available_sizes - static_cast<decltype(cur_size_index)>(1))) {
				throw std::bad_alloc();
			}
			++cur_size_index;
			std::list<hop_table> temp_tables;
			temp_tables.push_back(hop_table(table_sizes[cur_size_index],m_hasher,m_key_equal));
			try {
				decltype(_unique_insert(key_type(),0)) result;
				const auto it_f = m_container.end();
				for (auto it = m_container.begin(); it != it_f; ++it) {
					if (it->test_occupied()) {
						do {
							result = temp_tables.back()._unique_insert(std::move(*(it->ptr())),temp_tables.back()._bucket(*(it->ptr())));
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
				piranha_assert(temp_tables.size() >= 1);
				while (unlikely(temp_tables.size() > 1)) {
					// Get the first table.
					const auto table_it = temp_tables.begin(), table_it_f = table_it->m_container.end();
					// Try to insert all elements from the first table into the last one.
					// If something goes wrong, append another table at the end and insert there instead.
					for (auto it = table_it->m_container.begin(); it != table_it_f; ++it) {
						if (it->test_occupied()) {
							do {
								result = temp_tables.back()._unique_insert(std::move(*(it->ptr())),temp_tables.back()._bucket(*(it->ptr())));
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
					// The first table was emptied, remove it.
					table_it->m_container = container_type();
					table_it->m_n_elements = 0;
					temp_tables.pop_front();
				}
			} catch (...) {
				// In face of exceptions, zero out the current and temporary tables, and re-throw.
				m_container = container_type();
				m_n_elements = 0;
				for (auto it = temp_tables.begin(); it != temp_tables.end(); ++it) {
					it->m_container = container_type();
					it->m_n_elements = 0;
				}
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
			return *it;
		}
	private:
		container_type	m_container;
		hasher		m_hasher;
		key_equal	m_key_equal;
		size_type	m_n_elements;
};

template <typename T, typename Hash, typename Pred>
template <typename U>
const mf_uint hop_table<T,Hash,Pred>::base_generic_hop_bucket<U>::n_eff_bits;

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
