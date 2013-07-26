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

#ifndef PIRANHA_SMALL_VECTOR_HPP
#define PIRANHA_SMALL_VECTOR_HPP

#include <boost/integer_traits.hpp>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>

#include "config.hpp"

namespace piranha {

namespace detail
{

template <typename T, typename Allocator>
class dynamic_storage
{
	public:
		using size_type = unsigned short;
		using value_type = T;
		using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	private:
		using a_traits = std::allocator_traits<allocator_type>;
		using pointer = typename a_traits::pointer;
		using const_pointer = typename a_traits::const_pointer;
		static_assert(std::is_same<pointer,value_type *>::value && std::is_same<const_pointer,const value_type *>::value,
			"The pointer type of the allocator must be a raw pointer.");
		// NOTE: this is guaranteed to be within the range of unsigned short.
		static const size_type max_size = 65535u;
	public:
		using iterator = pointer;
		using const_iterator = const_pointer;
		dynamic_storage():m_tuple(nullptr,allocator_type()),m_size(0u),m_capacity(0u) {}
		template <typename InputIt, typename = typename std::enable_if<
			std::is_constructible<T,typename std::iterator_traits<InputIt>::value_type>::value
			>::type>
		explicit dynamic_storage(InputIt first, InputIt last):m_tuple(nullptr,allocator_type()),m_size(0u),m_capacity(0u)
		{

		}
		void push_back(const value_type &x)
		{
			if (m_capacity == m_size) {
				increase_capacity();
			}
		}
		~dynamic_storage() noexcept
		{
			piranha_assert(consistency_checks());
			for (size_type i = 0u; i < m_size; ++i) {
				a_traits::destroy(alloc(),ptr() + i);
			}
			if (ptr() != nullptr) {
				a_traits::deallocate(alloc(),ptr(),m_capacity);
			}
		}
		value_type &operator[](const size_type &n)
		{
			return ptr()[n];
		}
		const value_type &operator[](const size_type &n) const
		{
			return ptr()[n];
		}
		size_type size() const
		{
			return m_size;
		}
		iterator begin()
		{
			return ptr();
		}
		iterator end()
		{
			return ptr() + m_size;
		}
		const_iterator begin() const
		{
			return ptr();
		}
		const_iterator end() const
		{
			return ptr() + m_size;
		}
		void reserve(const size_type &size) const
		{
			// No need to do anything if we already have enough capacity.
			if (size <= m_capacity) {
				return;
			}
			// TODO likely/unlikely.
			// Check that we are not asking too much.
			if (size > max_size || size > a_traits::max_size(alloc())) {
				// TODO throw error.
			}
			// Start by allocating the new storage.
			pointer new_storage = a_traits::allocate(alloc(),size);
			if (new_storage == nullptr) {
				// TODO throw throw.
			}
			// Move in existing elements.

		}
	private:
		void increase_capacity()
		{

		}
		pointer &ptr()
		{
			return std::get<0u>(m_tuple);
		}
		const pointer &ptr() const
		{
			return std::get<0u>(m_tuple);
		}
		allocator_type &alloc()
		{
			return std::get<1u>(m_tuple);
		}
		const allocator_type &alloc() const
		{
			return std::get<1u>(m_tuple);
		}
		bool consistency_checks()
		{
			// Size cannot be greater than capacity.
			if (m_size > m_capacity) {
				return false;
			}
			// If we allocated something, capacity cannot be zero.
			if (ptr() != nullptr && m_capacity == 0u) {
				return false;
			}
			// If we have nothing allocated, capacity must be zero.
			if (ptr() == nullptr && m_capacity != 0u) {
				return false;
			}
			return true;
		}
	private:
		// NOTE: here we use a tuple because many implementations will optimise the size
		// to be the same size of pointer when the allocator is stateless (using empty
		// base optimisation). Consider replacing this with boost compressed pair if we need
		// to enforce the behaviour.
		std::tuple<pointer,allocator_type>	m_tuple;
		size_type				m_size;
		size_type				m_capacity;
};

template <typename T, std::size_t Size>
class static_storage
{
		// NOTE: some notes about the use of raw storage, after some research in the standard:
		// - storage type is guaranteed to be a POD able to hold any object whose size is at most Len
		//   and alignment a divisor of Align (20.9.7.6). Thus, we can store on object of type T at the
		//   beginning of the storage;
		// - POD types are trivially copyable, and thus they occupy contiguous bytes of storage (1.8/5);
		// - the meaning of "contiguous" seems not to be explicitly stated, but it can be inferred
		//   (e.g., 23.3.2.1/1) that it has to do with the fact that one can do pointer arithmetics
		//   to move around in the storage;
		// - the footnote in 5.7/6 implies that casting around to different pointer types and then doing
		//   pointer arithmetics has the expected behaviour of moving about with discrete offsets equal to the sizeof
		//   the pointed-to object (i.e., it seems like pointer arithmetics does not suffer from
		//   strict aliasing and type punning issues - at least as long as one goest through void *
		//   conversions to get the address of the storage - note also 3.9.2/3). This supposedly applies to arrays only,
		//   but, again, it is implied in other places that it also holds true for contiguous storage;
		// - the no-padding guarantee for arrays (which also consist of contiguous storage) implies that sizeof must be
		//   a multiple of the alignment for a given type;
		// - the definition of alignment (3.11) implies that if one has contiguous storage starting at an address in
		//   which T can be constructed, the offsetting the initial address by multiples of the alignment value will
		//   still produce addresses at which the object can be constructed.
		// TODO overflow assert guard.
		using storage_type = typename std::aligned_storage<sizeof(T) * Size,alignof(T)>::type;
	public:
		using size_type = unsigned char;
		using value_type = T;
		static_storage():m_size(0u) {}
		~static_storage() noexcept
		{
			// NOTE: here it could be optimised for trivially destructible types (3.8),
			// but GCC 4.7 does not implement the trait yet and the compiler is likely
			// to optimise the loop away anyway.
			for (size_type i = 0u; i < m_size; ++i) {
				(*this)[i].~value_type();
			}
		}
		value_type &operator[](const size_type &n)
		{
			return *(static_cast<value_type *>(static_cast<void *>(&m_storage)) + n);
		}
		const value_type &operator[](const size_type &n) const
		{
			return *(static_cast<const value_type *>(static_cast<const void *>(&m_storage)) + n);
		}
		size_type size() const
		{
			return m_size;
		}
	private:
		storage_type	m_storage;
		size_type	m_size;
};

template <typename T, typename Allocator, std::size_t Size = 1u, typename = void>
struct auto_static_size
{
	static const std::size_t value = Size;
};

template <typename T, typename Allocator, std::size_t Size>
struct auto_static_size<T,Allocator,Size,typename std::enable_if<
	(sizeof(dynamic_storage<T,Allocator>) > sizeof(static_storage<T,Size>))
	>::type>
{
	// TODO static assert overflow.
	static const std::size_t value = auto_static_size<T,Allocator,Size + 1u>::value;
};

}

template <typename T, std::size_t Size = 0u, typename Allocator = std::allocator<T>>
class small_vector
{
		using s_storage = typename std::conditional<Size == 0u,static_storage<T,auto_static_size<T,Allocator>::value>,static_storage<T,Size>>::type;
		using d_storage = dynamic_storage<T,Allocator>;
		static constexpr std::size_t get_max(const std::size_t &a, const std::size_t &b)
		{
			return (a > b) ? a : b;
		}
		static const std::size_t align_union = get_max(alignof(s_storage),alignof(d_storage));
		static const std::size_t size_union = get_max(sizeof(s_storage),sizeof(d_storage));
		typedef typename std::aligned_storage<size_union,align_union>::type storage_type;
		s_storage *get_s()
		{
			return static_cast<s_storage *>(get_vs());
		}
		const s_storage *get_s() const
		{
			return static_cast<const s_storage *>(get_vs());
		}
		d_storage *get_d()
		{
			return static_cast<d_storage *>(get_vs());
		}
		const d_storage *get_d() const
		{
			return static_cast<const d_storage *>(get_vs());
		}
		void *get_vs()
		{
			return static_cast<void *>(&m_storage);
		}
		const void *get_vs() const
		{
			return static_cast<const void *>(&m_storage);
		}
		using size_type_impl = typename std::conditional<(boost::integer_traits<typename s_storage::size_type>::const_max >
			boost::integer_traits<typename d_storage::size_type>::const_max),typename s_storage::size_type,
			typename d_storage::size_type>::type;
	public:
		/// An unsigned integer type representing the number of elements stored in the vector.
		using size_type = size_type_impl;
		small_vector():m_static(true)
		{
			::new (get_vs()) s_storage();
		}
		small_vector(const small_vector &other):m_static(other.m_static)
		{
			if (m_static) {
				::new (get_vs()) s_storage(*other.get_s());
			} else {
				::new (get_vs()) d_storage(*other.get_d());
			}
		}
		~small_vector() noexcept
		{
			if (m_static) {
				get_s()->~s_storage();
			} else {
				get_d()->~d_storage();
			}
		}
		size_type size() const
		{
			if (m_static) {
				return get_s()->size();
			}
			return get_d()->size();
		}
	private:
		storage_type	m_storage;
		bool		m_static;
};

}

#endif
