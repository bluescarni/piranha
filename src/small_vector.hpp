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
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "aligned_memory.hpp"
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
// NOTE: now we do not use the allocator at all, as we cannot guarantee it has a standard layout.
template <typename T>
class dynamic_storage
{
		PIRANHA_TT_CHECK(is_container_element,T);
	public:
		using size_type = unsigned char;
		using value_type = T;
	private:
		using pointer = value_type *;
		using const_pointer = value_type const *;
	public:
		static const size_type max_size = boost::integer_traits<size_type>::const_max;
		using iterator = pointer;
		using const_iterator = const_pointer;
		dynamic_storage() : m_size(0u),m_capacity(0u),m_ptr(nullptr) {}
		dynamic_storage(dynamic_storage &&other) noexcept : m_size(other.m_size),m_capacity(other.m_capacity),m_ptr(other.m_ptr)
		{
			// Erase the other.
			other.m_size = 0u;
			other.m_capacity = 0u;
			other.m_ptr = nullptr;
		}
		dynamic_storage(const dynamic_storage &other) :
			m_size(0u),m_capacity(other.m_size), // NOTE: when copying, we set the capacity to the same value of the size.
			m_ptr(nullptr)
		{
			// Obtain storage. Will just return nullptr if requested size is zero.
			m_ptr = obtain_new_storage(other.m_size);
			// Attempt to copy-construct the elements from other.
			try {
				for (; m_size < other.m_size; ++m_size) {
					construct(m_ptr + m_size,other[m_size]);
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
				m_size = other.m_size;
				m_capacity = other.m_capacity;
				m_ptr = other.m_ptr;
				// Nuke the other.
				other.m_size = 0u;
				other.m_capacity = 0u;
				other.m_ptr = nullptr;
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
			return m_ptr[n];
		}
		const value_type &operator[](const size_type &n) const
		{
			piranha_assert(n < m_size);
			return m_ptr[n];
		}
		size_type size() const
		{
			return m_size;
		}
		iterator begin()
		{
			return m_ptr;
		}
		iterator end()
		{
			// NOTE: in case m_size is zero, this is guaranteed to return
			// the original pointer (5.7/7).
			return m_ptr + m_size;
		}
		const_iterator begin() const
		{
			return m_ptr;
		}
		const_iterator end() const
		{
			return m_ptr + m_size;
		}
		void reserve(const size_type &new_capacity)
		{
			piranha_assert(consistency_checks());
			// No need to do anything if we already have enough capacity.
			if (new_capacity <= m_capacity) {
				return;
			}
			// Check that we are not asking too much.
			// NOTE: the first check is redundant right now, just keep it in case max_size changes in the future.
			if (unlikely(new_capacity > max_size || new_capacity > boost::integer_traits<std::size_t>::const_max / sizeof(value_type))) {
				piranha_throw(std::bad_alloc,);
			}
			// Start by allocating the new storage. New capacity is at least one at this point.
			pointer new_storage = obtain_new_storage(new_capacity);
			assert(new_storage != nullptr);
			// Move in existing elements. Consistency checks ensure
			// that m_size is not greater than m_capacity and, by extension, new_capacity.
			for (size_type i = 0u; i < m_size; ++i) {
				construct(new_storage + i,std::move((*this)[i]));
			}
			// Destroy and deallocate original content.
			destroy_and_deallocate();
			// Move in the new pointer and capacity.
			m_capacity = new_capacity;
			m_ptr = new_storage;
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
			pointer storage = new_storage ? obtain_new_storage(new_size) : m_ptr;
			// NOTE: storage cannot be nullptr:
			// - if new storage, new_size has to be at least 1 (new_size > m_capacity);
			// - if not new storage, new_size <= m_capacity; m_ptr can be null only if capacity is 0, but then
			//   m_size is zero and new_size is 0 as well, and the function never arrived here because of the check above.
			piranha_assert(storage != nullptr);
			// Default-construct excess elements. We need to do this regardless of where the storage is coming from.
			// This is also the only place we care about exception handling.
			size_type i = m_size;
			try {
				for (; i < new_size; ++i) {
					construct(storage + i);
				}
			} catch (...) {
				// Roll back and dealloc.
				for (size_type j = m_size; j < i; ++j) {
					destroy(storage + j);
				}
				deallocate(storage);
				throw;
			}
			// NOTE: no more exceptions thrown after this point.
			if (new_storage) {
				// Move in old elements into the new storage. As we had to increase the capacity,
				// we know that new_size has to be greater than the old one, hence all old elements
				// need to be moved over.
				for (size_type i = 0u; i < m_size; ++i) {
					construct(storage + i,std::move((*this)[i]));
				}
				// Erase the old content and assign new.
				destroy_and_deallocate();
				m_capacity = new_size;
				m_ptr = storage;
			} else {
				// Destroy excess elements in the old storage.
				for (size_type i = new_size; i < m_size; ++i) {
					destroy(storage + i);
				}
			}
			// In any case, we need to update the size.
			m_size = new_size;
		}
	private:
		template <typename ... Args>
		static void construct(pointer p, Args && ... args)
		{
			::new (static_cast<void *>(p)) value_type(std::forward<Args>(args)...);
		}
		static void destroy(pointer p)
		{
			p->~value_type();
		}
		static pointer allocate(const size_type &s)
		{
			return static_cast<pointer>(aligned_palloc(0u,static_cast<std::size_t>(s * sizeof(value_type))));
		}
		static void deallocate(pointer p)
		{
			aligned_pfree(0u,p);
		}
		// Common implementation of push_back().
		template <typename U>
		void push_back_impl(U &&x)
		{
			piranha_assert(consistency_checks());
			if (unlikely(m_capacity == m_size)) {
				increase_capacity();
			}
			construct(m_ptr + m_size,std::forward<U>(x));
			++m_size;
		}
		void destroy_and_deallocate() noexcept
		{
			piranha_assert(consistency_checks());
			for (size_type i = 0u; i < m_size; ++i) {
				// NOTE: could use POD optimisations here in principle.
				destroy(m_ptr + i);
			}
			if (m_ptr != nullptr) {
				deallocate(m_ptr);
			}
		}
		// Obtain new storage, and throw an error in case something goes wrong.
		pointer obtain_new_storage(const size_type &size)
		{
			// NOTE: no need to check for zero, will aready return nullptr in that case.
			return allocate(size);
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
		bool consistency_checks()
		{
			// Size cannot be greater than capacity.
			if (unlikely(m_size > m_capacity)) {
				return false;
			}
			// If we allocated something, capacity cannot be zero.
			if (unlikely(m_ptr != nullptr && m_capacity == 0u)) {
				return false;
			}
			// If we have nothing allocated, capacity must be zero.
			if (unlikely(m_ptr == nullptr && m_capacity != 0u)) {
				return false;
			}
			return true;
		}
	private:
		unsigned char	m_tag;
		size_type	m_size;
		size_type	m_capacity;
		pointer		m_ptr;
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

template <typename T>
struct check_integral_constant
{
	static const bool value = false;
};

template <std::size_t Size>
struct check_integral_constant<std::integral_constant<std::size_t,Size>>
{
	static const bool value = true;
};

}

/// Small vector class.
/**
 * This class is a sequence container similar to the standard <tt>std::vector</tt> class. The class will avoid dynamic
 * memory allocation by using internal static storage up to a certain number of stored elements. If \p S is a zero integral constant, this
 * number is calculated automatically (but it will always be at least 1). Otherwise, the limit number is set to \p S::value.
 *
 * \section type_requirements Type requirements
 *
 * - \p T must satisfy piranha::is_container_element.
 * - \p S must be an \p std::integral_constant of type \p std::size_t.
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
template <typename T, typename S = std::integral_constant<std::size_t,0u>>
class small_vector
{
		PIRANHA_TT_CHECK(is_container_element,T);
		PIRANHA_TT_CHECK(detail::check_integral_constant,S);
		using s_storage = typename std::conditional<S::value == 0u,static_vector<T,detail::auto_static_size<T>::value>,static_vector<T,S::value>>::type;
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
		/// Maximum number of elements that can be stored in static storage.
		static const typename std::decay<decltype(s_storage::max_size)>::type max_static_size = s_storage::max_size;
		/// Maximum number of elements that can be stored in dynamic storage.
		static const typename std::decay<decltype(d_storage::max_size)>::type max_dynamic_size = d_storage::max_size;
		/// A fundamental unsigned integer type representing the number of elements stored in the vector.
		using size_type = size_type_impl;
		/// Maximum number of elements that can be stored.
		static const size_type max_size = get_max<size_type>(max_static_size,s_storage::max_size);
		/// Alias for \p T.
		using value_type = T;
		/// Iterator type.
		using iterator = value_type *;
		/// Const iterator type.
		using const_iterator = value_type const *;
		/// Default constructor.
		/**
		 * Will initialise an empty vector with internal static storage.
		 */
		small_vector():m_static(true)
		{
			::new (get_vs()) s_storage();
		}
		/// Copy constructor.
		/**
		 * The storage type after successful construction will be the same of \p other.
		 *
		 * @param[in] other object to be copied.
		 *
		 * @throws std::bad_alloc in case of memory allocation errors.
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		small_vector(const small_vector &other):m_static(other.m_static)
		{
			if (m_static) {
				::new (get_vs()) s_storage(*other.get_s());
			} else {
				::new (get_vs()) d_storage(*other.get_d());
			}
		}
		/// Move constructor.
		/**
		 * The storage type after successful construction will be the same of \p other.
		 *
		 * @param[in] other object to be moved.
		 */
		small_vector(small_vector &&other) noexcept : m_static(other.m_static)
		{
			if (m_static) {
				::new (get_vs()) s_storage(std::move(*other.get_s()));
			} else {
				::new (get_vs()) d_storage(std::move(*other.get_d()));
			}
		}
		/// Constructor from initializer list.
		/**
		 * \note
		 * This constructor is enabled only if \p T is constructible from \p U.
		 *
		 * The elements of \p l will be added to a default-constructed object.
		 *
		 * @param[in] l list that will be used for initialisation.
		 *
		 * @throws unspecified any exception thrown by push_back().
		 */
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
		/// Destructor.
		~small_vector() noexcept
		{
			if (m_static) {
				get_s()->~s_storage();
			} else {
				get_d()->~d_storage();
			}
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other object to be used for assignment.
		 *
		 * @return reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		small_vector &operator=(const small_vector &other)
		{
			if (likely(this != &other)) {
				*this = small_vector(other);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other object to be used for assignment.
		 *
		 * @return reference to \p this.
		 */
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
		/// Const subscript operator.
		/**
		 * @param[in] n index of the element to be accessed.
		 *
		 * @return const reference to the <tt>n</tt>-th element of the vector.
		 */
		const value_type &operator[](const size_type &n) const
		{
			return begin()[n];
		}
		/// Subscript operator.
		/**
		 * @param[in] n index of the element to be accessed.
		 *
		 * @return reference to the <tt>n</tt>-th element of the vector.
		 */
		value_type &operator[](const size_type &n)
		{
			return begin()[n];
		}
		/// Copy-add element at the end.
		/**
		 * @param[in] x object that will be added at the end of the vector.
		 *
		 * @throws std::bad_alloc in case of memory allocation errors or if the size limit is exceeded.
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		void push_back(const value_type &x)
		{
			push_back_impl(x);
		}
		/// Move-add element at the end.
		/**
		 * @param[in] x object that will be added at the end of the vector.
		 *
		 * @throws std::bad_alloc in case of memory allocation errors or if the size limit is exceeded.
		 */
		void push_back(value_type &&x)
		{
			push_back_impl(std::move(x));
		}
		/// Mutable begin iterator.
		/**
		 * @return iterator to the beginning of the vector.
		 */
		iterator begin()
		{
			if (m_static) {
				return get_s()->begin();
			} else {
				return get_d()->begin();
			}
		}
		/// Mutable end iterator.
		/**
		 * @return iterator to the end of the vector.
		 */
		iterator end()
		{
			if (m_static) {
				return get_s()->end();
			} else {
				return get_d()->end();
			}
		}
		/// Const begin iterator.
		/**
		 * @return iterator to the beginning of the vector.
		 */
		const_iterator begin() const
		{
			if (m_static) {
				return get_s()->begin();
			} else {
				return get_d()->begin();
			}
		}
		/// Const end iterator.
		/**
		 * @return iterator to the end of the vector.
		 */
		const_iterator end() const
		{
			if (m_static) {
				return get_s()->end();
			} else {
				return get_d()->end();
			}
		}
		/// Size.
		/**
		 * @return number of elements stored in the vector.
		 */
		size_type size() const
		{
			if (m_static) {
				return get_s()->size();
			} else {
				return get_d()->size();
			}
		}
		/// Static storage flag.
		/**
		 * @return \p true if the storage being used is the static one, \p false otherwise.
		 */
		bool is_static() const
		{
			return m_static;
		}
		/// Equality operator.
		/**
		 * \note
		 * This method is enabled only if \p T is equality comparable.
		 *
		 * @param[in] other argument for the comparison.
		 *
		 * @return \p true if the sizes of \p this and \p other coincide and the element-wise comparison
		 * of the stored objects is \p true, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by the equality operator of \p T.
		 */
		template <typename U = value_type, typename = typename std::enable_if<
			is_equality_comparable<U>::value
			>::type>
		bool operator==(const small_vector &other) const
		{
			// NOTE: it seems like in C++14 the check on equal sizes is embedded in std::equal
			// when using the new algorithm signature:
			// http://en.cppreference.com/w/cpp/algorithm/equal
			// Just keep it in mind for the future.
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
		/// Inequality operator.
		/**
		 * \note
		 * This method is enabled only if \p T is equality comparable.
		 *
		 * @param[in] other argument for the comparison.
		 *
		 * @return the opposite of operator==().
		 *
		 * @throws unspecified any exception thrown by operator==().
		 */
		template <typename U = value_type, typename = typename std::enable_if<
			is_equality_comparable<U>::value
			>::type>
		bool operator!=(const small_vector &other) const
		{
			return !(this->operator==(other));
		}
		/// Hash method.
		/**
		 * \note
		 * This method is enabled only if \p value_type satisfies piranha::is_hashable.
		 *
		 * @return a hash value for \p this.
		 */
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
		/// Resize.
		/**
		 * Resize the vector to \p size. Elements in excess will be destroyed,
		 * new elements will be value-initialised and placed at the end.
		 *
		 * @param[in] size new size.
		 *
		 * @throws std::bad_alloc in case of memory allocation errors or if the size limit is exceeded.
		 * @throws unspecified any exception thrown by the default constructor of \p T.
		 */
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
		/// Vector addition.
		/**
		 * \note
		 * This method is enabled only if \p value_type is addable and assignable, and if the result
		 * of the addition is convertible to \p value_type.
		 *
		 * Will compute the element-wise addition of \p this and \p other, storing the result in \p retval.
		 * In face of exceptions during the addition of two elements, retval will be left in an unspecified
		 * but valid state, provided that the addition operator of \p value_type offers the basic exception
		 * safety guarantee.
		 *
		 * @param[out] retval result of the addition.
		 * @param[in] other argument for the addition.
		 *
		 * @throws std::invalid_argument if the sizes of \p this and \p other do not coincide.
		 * @throws unspecified any exception thrown by:
		 * - resize(),
		 * - the addition and assignment operators of \p value_type.
		 */
		template <typename U = value_type, typename = typename std::enable_if<
			std::is_convertible<decltype(std::declval<U const &>() + std::declval<U const &>()),U>::value &&
			std::is_assignable<value_type &, value_type>::value>::type>
		void add(small_vector &retval, const small_vector &other) const
		{
			const auto s = size();
			if (unlikely(other.size() != s)) {
				piranha_throw(std::invalid_argument,"vector size mismatch");
			}
			retval.resize(s);
			std::transform(begin(),end(),other.begin(),retval.begin(),adder<value_type>());
		}
	private:
		// NOTE: need this to silence warnings when operating on short ints: they will get
		// promoted to int during addition, hence resulting in a warning when casting back down
		// to short int on return.
		template <typename U, typename = void>
		struct adder
		{
			U operator()(const U &a, const U &b) const
			{
				return a + b;
			}
		};
		template <typename U>
		struct adder<U,typename std::enable_if<std::is_integral<U>::value>::type>
		{
			U operator()(const U &a, const U &b) const
			{
				return static_cast<U>(a + b);
			}
		};
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

template <typename T, typename S>
const typename std::decay<decltype(small_vector<T,S>::s_storage::max_size)>::type small_vector<T,S>::max_static_size;

template <typename T, typename S>
const typename std::decay<decltype(small_vector<T,S>::d_storage::max_size)>::type small_vector<T,S>::max_dynamic_size;

template <typename T, typename S>
const typename small_vector<T,S>::size_type small_vector<T,S>::max_size;

}

#endif
