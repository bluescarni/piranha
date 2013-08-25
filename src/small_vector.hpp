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

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>

#include "config.hpp"
#include "detail/vector_hasher.hpp"
#include "exceptions.hpp"
#include "static_vector.hpp"
#include "type_traits.hpp"

namespace piranha {

namespace detail
{

// This is essentially a reduced std::vector replacement that should use less storage on most implementations
// (e.g., it has a size of 16 on Linux 64-bit and it _should_ have a size of 8 on many 32-bit archs).
// NOTE: earlier versions of this class used to have support for custom allocation. See, e.g., commit
// 6e1dd235c729ed05eb6e872dceece2f20a624f32
// It seems however that there are some defects currently in the standard C++ library that make it quite hard
// to implement reasonably strong exception safety and nothrow guarantees. See, e.g.,
// http://cplusplus.github.io/LWG/lwg-defects.html#2103
// It seems like in the current standard it is not possible to implemement noexcept move assignment not even
// for std::alloc. The crux of the matter is that if propagate_on_container_move_assignment is not defined,
// we will need in general a memory allocation (that can fail).
// Another, less important, problem is that for copy assignment the presence of
// propagate_on_container_copy_assignment seems to force one to nuke pre-emptively the content of the container
// before doing anything (thus preventing strong forms of exception safety).
// The approach taken here is thus to just hard-code std::allocator, which is stateless, and force nothrow
// behaviour when doing assignments. This should work with all stateless allocators (including our own allocator).
// Unfortunately it is not clear if the extension allocators from GCC are stateless, so better not to muck
// with them (apart from experimentation).
// See also:
// http://stackoverflow.com/questions/12332772/why-arent-container-move-assignment-operators-noexcept
template <typename T>
class dynamic_storage
{
		PIRANHA_TT_CHECK(is_container_element,T);
	public:
		using size_type = unsigned short;
		using value_type = T;
		using allocator_type = std::allocator<T>;
	private:
		using a_traits = std::allocator_traits<allocator_type>;
		using pointer = typename a_traits::pointer;
		using const_pointer = typename a_traits::const_pointer;
		// NOTE: common tuple implementations use EBCO, try to reduce size in case
		// of stateless allocators (such as std::allocator).
		// http://flamingdangerzone.com/cxx11/2012/07/06/optimal-tuple-i.html
		using pair_type = std::tuple<pointer,allocator_type>;
	public:
		// NOTE: this is guaranteed to be within the range of unsigned short.
		static const size_type max_size = 32768u;
		using iterator = pointer;
		using const_iterator = const_pointer;
		dynamic_storage():m_pair(nullptr,allocator_type()),m_size(0u),m_capacity(0u) {}
		// NOTE: move ctor of allocator is guaranteed not to throw (23.2.1/7).
		dynamic_storage(dynamic_storage &&other) noexcept : m_pair(std::move(other.m_pair)),m_size(other.m_size),m_capacity(other.m_capacity)
		{
			// Erase the other.
			other.ptr() = nullptr;
			other.m_size = 0u;
			other.m_capacity = 0u;
		}
		// NOTE: just keep the used of a_traits for copying the allocator, it won't hurt.
		dynamic_storage(const dynamic_storage &other) : m_pair(nullptr,a_traits::select_on_container_copy_construction(other.alloc())),
			m_size(0u),m_capacity(other.m_size) // NOTE: when copying, we set the capacity to the same value of the size.
		{
			// Obtain storage.
			// NOTE: here we are using the allocator obtained from copy, and we know the allocator can satisfy this (in terms of max_size()) as it
			// satisfied already a request equal or larger in other. Will just return nullptr if requested size is zero.
			ptr() = obtain_new_storage(other.m_size);
			// Attempt to copy-construct the elements from other.
			try {
				for (; m_size < other.m_size; ++m_size) {
					a_traits::construct(alloc(),ptr() + m_size,other[m_size]);
				}
			} catch (...) {
				// Roll back the construction and deallocate before re-throwing.
				destroy_and_deallocate();
				throw;
			}
		}
		~dynamic_storage() noexcept
		{
			destroy_and_deallocate();
		}
		dynamic_storage &operator=(dynamic_storage &&other) noexcept
		{
			if (likely(this != &other)) {
				// Destroy and deallocate this.
				destroy_and_deallocate();
				// Just pilfer the resources.
				m_pair = std::move(other.m_pair);
				m_size = other.m_size;
				m_capacity = other.m_capacity;
				// Nuke the other.
				other.ptr() = nullptr;
				other.m_size = 0u;
				other.m_capacity = 0u;
			}
			return *this;
		}
		dynamic_storage &operator=(const dynamic_storage &other)
		{
			if (likely(this != &other)) {
				dynamic_storage tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		bool empty() const
		{
			return m_size == 0u;
		}
		void push_back(const value_type &x)
		{
			push_back_impl(x);
		}
		void push_back(value_type &&x)
		{
			push_back_impl(std::move(x));
		}
		value_type &operator[](const size_type &n)
		{
			piranha_assert(n < m_size);
			return ptr()[n];
		}
		const value_type &operator[](const size_type &n) const
		{
			piranha_assert(n < m_size);
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
			// NOTE: in case m_size is zero, this is guaranteed to return
			// the original pointer (5.7/7).
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
		void reserve(const size_type &new_capacity)
		{
			piranha_assert(consistency_checks());
			// No need to do anything if we already have enough capacity.
			if (new_capacity <= m_capacity) {
				return;
			}
			// Check that we are not asking too much.
			// NOTE: a_traits::max_size() returns an unsigned integer type, so we are sure the comparison
			// will work as expected.
			// http://en.cppreference.com/w/cpp/concept/Allocator
			if (unlikely(new_capacity > max_size || new_capacity > a_traits::max_size(alloc()))) {
				piranha_throw(std::bad_alloc,);
			}
			// Start by allocating the new storage. New capacity is at least one at this point.
			pointer new_storage = obtain_new_storage(new_capacity);
			assert(new_storage != nullptr);
			// Move in existing elements. Consistency checks ensure
			// that m_size is not greater than m_capacity and, by extension, new_capacity.
			for (size_type i = 0u; i < m_size; ++i) {
				a_traits::construct(alloc(),new_storage + i,std::move((*this)[i]));
			}
			// Destroy and deallocate original content.
			destroy_and_deallocate();
			// Move in the new pointer and capacity.
			ptr() = new_storage;
			m_capacity = new_capacity;
		}
		size_type capacity() const
		{
			return m_capacity;
		}
		std::size_t hash() const
		{
			return detail::vector_hasher(*this);
		}
		void resize(const size_type &new_size)
		{
			if (unlikely(new_size > max_size)) {
				piranha_throw(std::bad_alloc,);
			}
			if (new_size == m_size) {
				return;
			}
			// The storage we are going to operate on is either the old one, if it has enough capacity,
			// or new storage.
			const bool new_storage = (m_capacity < new_size);
			pointer storage = new_storage ? obtain_new_storage(new_size) : ptr();
			// Default-construct excess elements. We need to do this regardless of where the storage is coming from.
			// This is also the only place we care about exception handling.
			size_type i = m_size;
			try {
				for (; i < new_size; ++i) {
					a_traits::construct(alloc(),storage + i);
				}
			} catch (...) {
				// Roll back and dealloc.
				for (size_type j = m_size; j < i; ++j) {
					a_traits::destroy(alloc(),storage + j);
				}
				a_traits::deallocate(alloc(),storage,new_size);
				throw;
			}
			// NOTE: no more exceptions thrown after this point.
			if (new_storage) {
				// Move in old elements into the new storage. As we had to increase the capacity,
				// we know that new_size has to be greater than the old one, hence all old elements
				// need to be moved over.
				for (size_type i = 0u; i < m_size; ++i) {
					a_traits::construct(alloc(),storage + i,std::move((*this)[i]));
				}
				// Erase the old content and assign new.
				destroy_and_deallocate();
				ptr() = storage;
				m_capacity = new_size;
			} else {
				// Destroy excess elements in the old storage.
				for (size_type i = new_size; i < m_size; ++i) {
					a_traits::destroy(alloc(),storage + i);
				}
			}
			// In any case, we need to update the size.
			m_size = new_size;
		}
	private:
		// Common implementation of push_back().
		template <typename U>
		void push_back_impl(U &&x)
		{
			piranha_assert(consistency_checks());
			if (unlikely(m_capacity == m_size)) {
				increase_capacity();
			}
			a_traits::construct(alloc(),ptr() + m_size,std::forward<U>(x));
			++m_size;
		}
		void destroy_and_deallocate() noexcept
		{
			piranha_assert(consistency_checks());
			for (size_type i = 0u; i < m_size; ++i) {
				// NOTE: could use POD optimisations here in principle, but keep it like
				// this in case in the future we allow to select the allocator.
				// NOTE: exceptions here will call std::abort(), as we are in noexcept land.
				a_traits::destroy(alloc(),ptr() + i);
			}
			if (ptr() != nullptr) {
				// NOTE: deallocate can never throw.
				// http://en.cppreference.com/w/cpp/concept/Allocator
				a_traits::deallocate(alloc(),ptr(),m_capacity);
			}
		}
		// Obtain new storage, and throw an error in case something goes wrong.
		pointer obtain_new_storage(const size_type &size)
		{
			if (size == 0u) {
				return pointer(nullptr);
			}
			pointer storage = a_traits::allocate(alloc(),size);
			if (unlikely(storage == nullptr)) {
				piranha_throw(std::bad_alloc,);
			}
			return storage;
		}
		// Will try to double the capacity, or, in case this is not possible,
		// will set the capacity to max_size. If the initial capacity is already max,
		// then will throw an error.
		void increase_capacity()
		{
			if (unlikely(m_capacity == max_size)) {
				piranha_throw(std::bad_alloc,);
			}
			// NOTE: capacity should double, but without going past max_size, and in case it is zero it should go to 1.
			const size_type new_capacity = (m_capacity > max_size / 2u) ? max_size : ((m_capacity != 0u) ?
				static_cast<size_type>(m_capacity * 2u) : static_cast<size_type>(1u));
			reserve(new_capacity);
		}
		pointer &ptr()
		{
			return std::get<0u>(m_pair);
		}
		const pointer &ptr() const
		{
			return std::get<0u>(m_pair);
		}
		allocator_type &alloc()
		{
			return std::get<1u>(m_pair);
		}
		const allocator_type &alloc() const
		{
			return std::get<1u>(m_pair);
		}
		bool consistency_checks()
		{
			// Size cannot be greater than capacity.
			if (unlikely(m_size > m_capacity)) {
				return false;
			}
			// If we allocated something, capacity cannot be zero.
			if (unlikely(ptr() != nullptr && m_capacity == 0u)) {
				return false;
			}
			// If we have nothing allocated, capacity must be zero.
			if (unlikely(ptr() == nullptr && m_capacity != 0u)) {
				return false;
			}
			return true;
		}
	private:
		pair_type	m_pair;
		size_type	m_size;
		size_type	m_capacity;
};

template <typename T>
const typename dynamic_storage<T>::size_type dynamic_storage<T>::max_size;

// TMP to determine automatically the size of the static storage in small_vector.
template <typename T, std::size_t Size = 1u, typename = void>
struct auto_static_size
{
	static const std::size_t value = Size;
};

template <typename T, std::size_t Size>
struct auto_static_size<T,Size,typename std::enable_if<
	(sizeof(dynamic_storage<T>) > sizeof(static_vector<T,Size>))
	>::type>
{
	static_assert(Size < boost::integer_traits<std::size_t>::const_max,"Overflow error in auto_static_size.");
	static const std::size_t value = auto_static_size<T,Size + 1u>::value;
};

}

/// Small vector class.
/**
 * This class is a sequence container similar to the standard <tt>std::vector</tt> class. The class will avoid dynamic
 * memory allocation by using internal static storage up to a certain number of stored elements. If \p Size is zero, this
 * number is calculated automatically (but it will always be at least 1). Otherwise, the limit number is set to \p Size.
 *
 * \section type_requirements Type requirements
 *
 * \p T must satisfy piranha::is_container_element.
 *
 * \section exception_safety Exception safety guarantee
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * \section move_semantics Move semantics
 *
 * After a move operation, the container is left in a state which is destructible and assignable (as long as \p T
 * is as well).
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T, std::size_t Size = 0u>
class small_vector
{
		PIRANHA_TT_CHECK(is_container_element,T);
		using s_storage = typename std::conditional<Size == 0u,static_vector<T,detail::auto_static_size<T>::value>,static_vector<T,Size>>::type;
		using d_storage = detail::dynamic_storage<T>;
		template <typename U>
		static constexpr U get_max(const U &a, const U &b)
		{
			return (a > b) ? a : b;
		}
		static const std::size_t align_union = get_max(alignof(s_storage),alignof(d_storage));
		static const std::size_t size_union = get_max(sizeof(s_storage),sizeof(d_storage));
		typedef typename std::aligned_storage<size_union,align_union>::type storage_type;
		s_storage *get_s()
		{
			piranha_assert(m_static);
			return static_cast<s_storage *>(get_vs());
		}
		const s_storage *get_s() const
		{
			piranha_assert(m_static);
			return static_cast<const s_storage *>(get_vs());
		}
		d_storage *get_d()
		{
			piranha_assert(!m_static);
			return static_cast<d_storage *>(get_vs());
		}
		const d_storage *get_d() const
		{
			piranha_assert(!m_static);
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
		// The size type will be the one with most range among the two storages.
		using size_type_impl = max_int<typename s_storage::size_type,typename d_storage::size_type>;
	public:
		static const typename std::decay<decltype(s_storage::max_size)>::type max_static_size = s_storage::max_size;
		static const typename std::decay<decltype(d_storage::max_size)>::type max_dynamic_size = d_storage::max_size;
		/// An unsigned integer type representing the number of elements stored in the vector.
		using size_type = size_type_impl;
		static const size_type max_size = get_max<size_type>(max_static_size,s_storage::max_size);
		using value_type = T;
		using iterator = value_type *;
		using const_iterator = value_type const *;
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
		small_vector(small_vector &&other) noexcept : m_static(other.m_static)
		{
			if (m_static) {
				::new (get_vs()) s_storage(std::move(*other.get_s()));
			} else {
				::new (get_vs()) d_storage(std::move(*other.get_d()));
			}
		}
		template <typename U, typename = typename std::enable_if<std::is_constructible<T,U const &>::value>::type>
		explicit small_vector(std::initializer_list<U> l):m_static(true)
		{
			::new (get_vs()) s_storage();
			try {
				for (const U &x : l) {
					push_back(T(x));
				}
			} catch (...) {
				// NOTE: push_back has strong exception safety, all we need to do is to invoke
				// the appropriate destructor.
				if (m_static) {
					get_s()->~s_storage();
				} else {
					get_d()->~d_storage();
				}
				throw;
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
		small_vector &operator=(const small_vector &other)
		{
			if (likely(this != &other)) {
				*this = small_vector(other);
			}
			return *this;
		}
		small_vector &operator=(small_vector &&other) noexcept
		{
			if (unlikely(this == &other)) {
				return *this;
			}
			if (m_static == other.m_static) {
				if (m_static) {
					get_s()->operator=(std::move(*other.get_s()));
				} else {
					get_d()->operator=(std::move(*other.get_d()));
				}
			} else {
				if (m_static) {
					// static vs dynamic.
					// Destroy static.
					get_s()->~s_storage();
					m_static = false;
					// Move construct dynamic from other.
					::new (get_vs()) d_storage(std::move(*other.get_d()));
				} else {
					// dynamic vs static.
					get_d()->~d_storage();
					m_static = true;
					::new (get_vs()) s_storage(std::move(*other.get_s()));
				}
			}
			return *this;
		}
		const value_type &operator[](const size_type &n) const
		{
			return begin()[n];
		}
		value_type &operator[](const size_type &n)
		{
			return begin()[n];
		}
		void push_back(const value_type &x)
		{
			push_back_impl(x);
		}
		void push_back(value_type &&x)
		{
			push_back_impl(std::move(x));
		}
		iterator begin()
		{
			if (m_static) {
				return get_s()->begin();
			} else {
				return get_d()->begin();
			}
		}
		iterator end()
		{
			if (m_static) {
				return get_s()->end();
			} else {
				return get_d()->end();
			}
		}
		const_iterator begin() const
		{
			if (m_static) {
				return get_s()->begin();
			} else {
				return get_d()->begin();
			}
		}
		const_iterator end() const
		{
			if (m_static) {
				return get_s()->end();
			} else {
				return get_d()->end();
			}
		}
		size_type size() const
		{
			if (m_static) {
				return get_s()->size();
			} else {
				return get_d()->size();
			}
		}
		bool is_static() const
		{
			return m_static;
		}
		template <typename U = value_type, typename = typename std::enable_if<
			is_equality_comparable<U>::value
			>::type>
		bool operator==(const small_vector &other) const
		{
			const unsigned mask = static_cast<unsigned>(m_static) +
				(static_cast<unsigned>(other.m_static) << 1u);
			switch (mask)
			{
				case 0u:
					return get_d()->size() == other.get_d()->size() &&
						std::equal(get_d()->begin(),get_d()->end(),other.get_d()->begin());
				case 1u:
					return get_s()->size() == other.get_d()->size() &&
						std::equal(get_s()->begin(),get_s()->end(),other.get_d()->begin());
				case 2u:
					return get_d()->size() == other.get_s()->size() &&
						std::equal(get_d()->begin(),get_d()->end(),other.get_s()->begin());
			}
			return get_s()->size() == other.get_s()->size() &&
				std::equal(get_s()->begin(),get_s()->end(),other.get_s()->begin());
		}
		template <typename U = value_type, typename = typename std::enable_if<
			is_equality_comparable<U>::value
			>::type>
		bool operator!=(const small_vector &other) const
		{
			return !(this->operator==(other));
		}
		template <typename U = value_type, typename = typename std::enable_if<
			is_hashable<U>::value
			>::type>
		std::size_t hash() const noexcept
		{
			if (m_static) {
				return get_s()->hash();
			} else {
				return get_d()->hash();
			}
		}
		void resize(const size_type &size)
		{
			if (m_static) {
				if (size <= max_static_size) {
					get_s()->resize(static_cast<typename s_storage::size_type>(size));
				} else {
					// NOTE: this check is a repetition of an existing check in dynamic storage's
					// resize. The reason for putting it here as well is to ensure the safety
					// of the static_cast below.
					if (unlikely(size > d_storage::max_size)) {
						piranha_throw(std::bad_alloc,);
					}
					// Move the existing elements into new dynamic storage.
					const auto d_size = static_cast<typename d_storage::size_type>(size);
					d_storage tmp_d;
					tmp_d.reserve(d_size);
					// NOTE: this will not throw, as tmp_d is guaranteed to be of adequate size
					// and thus push_back() will not fail.
					std::move(get_s()->begin(),get_s()->end(),std::back_inserter(tmp_d));
					// Fill in the missing elements.
					tmp_d.resize(d_size);
					// Destroy static, move in dynamic.
					get_s()->~s_storage();
					m_static = false;
					::new (get_vs()) d_storage(std::move(tmp_d));
				}
			} else {
				if (unlikely(size > d_storage::max_size)) {
					piranha_throw(std::bad_alloc,);
				}
				get_d()->resize(static_cast<typename d_storage::size_type>(size));
			}
		}
	private:
		template <typename U>
		void push_back_impl(U &&x)
		{
			if (m_static) {
				if (get_s()->size() == get_s()->max_size) {
					// Create a new dynamic vector, and move in the current
					// elements from static storage.
					using d_size_type = typename d_storage::size_type;
					// NOTE: this check ensures that the d_storage::max_size is strictly greater than
					// the current (static) size (equal to max static size). This means we can compute safely
					// current size + 1 while casting to dynamic storage size type and try to allocate enough space for
					// the std::move and push_back() below.
					// NOTE: this check is analogous to current_size + 1 > d_storage::max_size, i.e., the same
					// check in resize(), but it will use static const variables written as this.
					if (unlikely(s_storage::max_size >= d_storage::max_size)) {
						piranha_throw(std::bad_alloc,);
					}
					d_storage tmp_d;
					tmp_d.reserve(static_cast<d_size_type>(static_cast<d_size_type>(get_s()->max_size) + 1u));
					std::move(get_s()->begin(),get_s()->end(),std::back_inserter(tmp_d));
					// Push back the new element.
					tmp_d.push_back(std::forward<U>(x));
					// Now destroy the current static storage, and move-construct new dynamic storage.
					// NOTE: in face of custom allocators here we should be ok, as move construction
					// of custom alloc will not throw and it will preserve the ownership of the moved-in elements.
					get_s()->~s_storage();
					m_static = false;
					::new (get_vs()) d_storage(std::move(tmp_d));
				} else {
					get_s()->push_back(std::forward<U>(x));
				}
			} else {
				// In case we are already in dynamic storage, don't do anything special.
				get_d()->push_back(std::forward<U>(x));
			}
		}
	private:
		storage_type	m_storage;
		bool		m_static;
};

template <typename T, std::size_t Size>
const typename std::decay<decltype(small_vector<T,Size>::s_storage::max_size)>::type small_vector<T,Size>::max_static_size;

template <typename T, std::size_t Size>
const typename std::decay<decltype(small_vector<T,Size>::d_storage::max_size)>::type small_vector<T,Size>::max_dynamic_size;

template <typename T, std::size_t Size>
const typename small_vector<T,Size>::size_type small_vector<T,Size>::max_size;

}

#endif
