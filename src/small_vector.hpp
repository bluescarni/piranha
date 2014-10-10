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
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "detail/small_vector_fwd.hpp"
#include "detail/vector_hasher.hpp"
#include "exceptions.hpp"
#include "memory.hpp"
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
// NOTE: now we do not use the allocator at all, as we cannot guarantee it has a standard layout. Keep the comments
// above as an historical reference, might come useful in the future :)
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
		// NOTE: this bit of TMP is to avoid checking an always-false condition on reserve() on most platforms, which triggers a compiler
		// warning on GCC 4.7.
		static const std::size_t max_alloc_size = std::numeric_limits<std::size_t>::max() / sizeof(value_type);
		static const bool need_reserve_check = std::numeric_limits<size_type>::max() > max_alloc_size;
		static bool reserve_check_size(const size_type &, const std::false_type &)
		{
			return false;
		}
		static bool reserve_check_size(const size_type &new_capacity, const std::true_type &)
		{
			return new_capacity > max_alloc_size;
		}
	public:
		static const size_type max_size = std::numeric_limits<size_type>::max();
		using iterator = pointer;
		using const_iterator = const_pointer;
		dynamic_storage() : m_tag(0u),m_size(0u),m_capacity(0u),m_ptr(nullptr) {}
		dynamic_storage(dynamic_storage &&other) noexcept : m_tag(0u),m_size(other.m_size),m_capacity(other.m_capacity),m_ptr(other.m_ptr)
		{
			// Erase the other.
			other.m_size = 0u;
			other.m_capacity = 0u;
			other.m_ptr = nullptr;
		}
		dynamic_storage(const dynamic_storage &other) :
			m_tag(0u),m_size(0u),m_capacity(other.m_size), // NOTE: when copying, we set the capacity to the same value of the size.
			m_ptr(nullptr)
		{
			// Obtain storage. Will just return nullptr if requested size is zero.
			m_ptr = allocate(other.m_size);
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
		~dynamic_storage()
		{
			// NOTE: here we should replace with bidirectional tt, if we ever implement it.
			PIRANHA_TT_CHECK(is_forward_iterator,iterator);
			PIRANHA_TT_CHECK(is_forward_iterator,const_iterator);
			piranha_assert(m_tag == 0u);
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
			// NOTE: the first check is redundant right now, just keep it around in case max_size changes in the future.
			if (unlikely(/*new_capacity > max_size ||*/ reserve_check_size(new_capacity,std::integral_constant<bool,need_reserve_check>()))) {
				piranha_throw(std::bad_alloc,);
			}
			// Start by allocating the new storage. New capacity is at least one at this point.
			pointer new_storage = allocate(new_capacity);
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
			// NOTE: another redundant size check at the moment.
			// if (unlikely(new_size > max_size)) {
			// 	piranha_throw(std::bad_alloc,);
			// }
			if (new_size == m_size) {
				return;
			}
			// The storage we are going to operate on is either the old one, if it has enough capacity,
			// or new storage.
			const bool new_storage = (m_capacity < new_size);
			pointer storage = new_storage ? allocate(new_size) : m_ptr;
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
		// Obtain new storage, and throw an error in case something goes wrong.
		// NOTE: no need to check for zero, will aready return nullptr in that case.
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
		void destroy_and_deallocate()
		{
			piranha_assert(consistency_checks());
			for (size_type i = 0u; i < m_size; ++i) {
				// NOTE: could use POD optimisations here in principle.
				destroy(m_ptr + i);
			}
			// NOTE: no need to check for nullptr, aligned_pfree already does it.
			deallocate(m_ptr);
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

template <typename T>
const std::size_t dynamic_storage<T>::max_alloc_size;

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
	static_assert(Size < std::numeric_limits<std::size_t>::max(),"Overflow error in auto_static_size.");
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

// NOTE: here we are using a special rule from 9.2. If the union is standard-layout, and it contains two or more
// standard-layout classes, it will be possible to inspect the value of the common initial part of any of them. That is,
// we can access the initial m_tag member of the static and dynamic storage via both st and dy, and use it to detect
// which class of the union is active.
// See also:
// http://stackoverflow.com/questions/18564497/writing-into-the-last-byte-of-a-class
// http://www.informit.com/guides/content.aspx?g=cplusplus&seqNum=556
// http://en.wikipedia.org/wiki/C%2B%2B11#Unrestricted_unions
template <typename T, typename S>
union small_vector_union
{
	using s_storage = typename std::conditional<S::value == 0u,static_vector<T,auto_static_size<T>::value>,static_vector<T,S::value>>::type;
	using d_storage = dynamic_storage<T>;
	// NOTE: each constructor must be invoked explicitly.
	small_vector_union():m_st() {}
	small_vector_union(const small_vector_union &other)
	{
		if (other.is_static()) {
			::new (static_cast<void *>(&m_st)) s_storage(other.g_st());
		} else {
			::new (static_cast<void *>(&m_dy)) d_storage(other.g_dy());
		}
	}
	small_vector_union(small_vector_union &&other) noexcept
	{
		if (other.is_static()) {
			::new (static_cast<void *>(&m_st)) s_storage(std::move(other.g_st()));
		} else {
			::new (static_cast<void *>(&m_dy)) d_storage(std::move(other.g_dy()));
		}
	}
	~small_vector_union()
	{
		PIRANHA_TT_CHECK(std::is_standard_layout,s_storage);
		PIRANHA_TT_CHECK(std::is_standard_layout,d_storage);
		PIRANHA_TT_CHECK(std::is_standard_layout,small_vector_union);
		if (is_static()) {
			g_st().~s_storage();
		} else {
			g_dy().~d_storage();
		}
	}
	small_vector_union &operator=(small_vector_union &&other) noexcept
	{
		if (unlikely(this == &other)) {
			return *this;
		}
		if (is_static() == other.is_static()) {
			if (is_static()) {
				g_st() = std::move(other.g_st());
			} else {
				g_dy() = std::move(other.g_dy());
			}
		} else {
			if (is_static()) {
				// static vs dynamic.
				// Destroy static.
				g_st().~s_storage();
				// Move construct dynamic from other.
				::new (static_cast<void *>(&m_dy)) d_storage(std::move(other.g_dy()));
			} else {
				// dynamic vs static.
				g_dy().~d_storage();
				::new (static_cast<void *>(&m_st)) s_storage(std::move(other.g_st()));
			}
		}
		return *this;
	}
	small_vector_union &operator=(const small_vector_union &other)
	{
		if (likely(this != &other)) {
			*this = small_vector_union(other);
		}
		return *this;
	}
	bool is_static() const
	{
		return static_cast<bool>(m_st.m_tag);
	}
	// Getters.
	const s_storage &g_st() const
	{
		piranha_assert(is_static());
		return m_st;
	}
	s_storage &g_st()
	{
		piranha_assert(is_static());
		return m_st;
	}
	const d_storage &g_dy() const
	{
		piranha_assert(!is_static());
		return m_dy;
	}
	d_storage &g_dy()
	{
		piranha_assert(!is_static());
		return m_dy;
	}
	s_storage	m_st;
	d_storage	m_dy;
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
 * - \p T must satisfy piranha::is_container_element;
 * - \p S must be an \p std::integral_constant of type \p std::size_t.
 *
 * \section exception_safety Exception safety guarantee
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * \section move_semantics Move semantics
 *
 * After a move operation, the container will be empty.
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
// NOTE: some possible improvements:
// - the m_size member of dynamic and static could be made a signed integer, the sign establishing the storage type
//   and drop the m_tag member. This in principle would allow to squeeze some extra space from the static vector
//   but not sure this is worth it;
// - POD optimisations in dynamic storage;
// - in the dynamic storage, it looks like on 64bit we can bump up the size member to 16 bit without changing size,
//   thus we could sture ~16000 elements. BUT on 32bit this will change the size.
template <typename T, typename S = std::integral_constant<std::size_t,0u>>
class small_vector
{
		PIRANHA_TT_CHECK(is_container_element,T);
		PIRANHA_TT_CHECK(detail::check_integral_constant,S);
		using u_type = detail::small_vector_union<T,S>;
		using s_storage = typename u_type::s_storage;
		using d_storage = typename u_type::d_storage;
		// The size type will be the one with most range among the two storages.
		using size_type_impl = max_int<typename s_storage::size_type,typename d_storage::size_type>;
		template <typename U>
		static constexpr U get_max(const U &a, const U &b)
		{
			return (a > b) ? a : b;
		}
	public:
		/// Maximum number of elements that can be stored in static storage.
		static const typename std::decay<decltype(s_storage::max_size)>::type max_static_size = s_storage::max_size;
		/// Maximum number of elements that can be stored in dynamic storage.
		static const typename std::decay<decltype(d_storage::max_size)>::type max_dynamic_size = d_storage::max_size;
		/// A fundamental unsigned integer type representing the number of elements stored in the vector.
		using size_type = size_type_impl;
	private:
		// If the size type can assume values larger than d_storage::max_size, then we need to run a check in resize().
		static const bool need_resize_check = std::numeric_limits<size_type>::max() > d_storage::max_size;
		static bool resize_check_size(const size_type &, const std::false_type &)
		{
			return false;
		}
		static bool resize_check_size(const size_type &size, const std::true_type &)
		{
			return size > d_storage::max_size;
		}
	public:
		/// Maximum number of elements that can be stored.
		static const size_type max_size = get_max<size_type>(max_static_size,max_dynamic_size);
		/// Alias for \p T.
		using value_type = T;
		/// Iterator type.
		using iterator = value_type *;
		/// Const iterator type.
		using const_iterator = value_type const *;
	private:
		// NOTE: here we should replace with bidirectional tt, if we ever implement it.
		PIRANHA_TT_CHECK(is_forward_iterator,iterator);
		PIRANHA_TT_CHECK(is_forward_iterator,const_iterator);
		// Enabler for ctor from init list.
		template <typename U>
		using init_list_enabler = typename std::enable_if<std::is_constructible<T,U const &>::value,int>::type;
		// Enabler for equality operator.
		template <typename U>
		using equality_enabler = typename std::enable_if<is_equality_comparable<U>::value,int>::type;
		// Enabler for hash.
		template <typename U>
		using hash_enabler = typename std::enable_if<is_hashable<U>::value,int>::type;
		// Enabler for the addition.
		template <typename U>
		using add_enabler = typename std::enable_if<
			std::is_assignable<U &,decltype(std::declval<U const &>() + std::declval<U const &>()) &&>::value,int>::type;
	public:
		/// Default constructor.
		/**
		 * Will initialise an empty vector with internal static storage.
		 */
		small_vector() = default;
		/// Copy constructor.
		/**
		 * The storage type after successful construction will be the same of \p other.
		 *
		 * @throws std::bad_alloc in case of memory allocation errors.
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		small_vector(const small_vector &) = default;
		/// Move constructor.
		/**
		 * The storage type after successful construction will be the same of \p other.
		 */
		small_vector(small_vector &&) = default;
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
		template <typename U, init_list_enabler<U> = 0>
		explicit small_vector(std::initializer_list<U> l)
		{
			// NOTE: push_back has strong exception safety.
			for (const U &x : l) {
				push_back(T(x));
			}
		}
		/// Constructor from size and value.
		/**
		 * This constructor will initialise a vector containing \p size copies of \p value.
		 *
		 * @param[in] size the desired vector size.
		 * @param[in] value the value used to initialise all the elements of the vector.
		 *
		 * @throws unspecified any exception thrown by push_back().
		 */
		explicit small_vector(const size_type &size, const T &value)
		{
			for (size_type i = 0u; i < size; ++i) {
				push_back(value);
			}
		}
		/// Destructor.
		~small_vector() = default;
		/// Copy assignment operator.
		/**
		 * @return reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		small_vector &operator=(const small_vector &) = default;
		/// Move assignment operator.
		/**
		 * @return reference to \p this.
		 */
		small_vector &operator=(small_vector &&) = default;
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
			if (m_union.is_static()) {
				return m_union.g_st().begin();
			} else {
				return m_union.g_dy().begin();
			}
		}
		/// Mutable end iterator.
		/**
		 * @return iterator to the end of the vector.
		 */
		iterator end()
		{
			if (m_union.is_static()) {
				return m_union.g_st().end();
			} else {
				return m_union.g_dy().end();
			}
		}
		/// Const begin iterator.
		/**
		 * @return iterator to the beginning of the vector.
		 */
		const_iterator begin() const
		{
			if (m_union.is_static()) {
				return m_union.g_st().begin();
			} else {
				return m_union.g_dy().begin();
			}
		}
		/// Const end iterator.
		/**
		 * @return iterator to the end of the vector.
		 */
		const_iterator end() const
		{
			if (m_union.is_static()) {
				return m_union.g_st().end();
			} else {
				return m_union.g_dy().end();
			}
		}
		/// Size.
		/**
		 * @return number of elements stored in the vector.
		 */
		size_type size() const
		{
			if (m_union.is_static()) {
				return m_union.g_st().size();
			} else {
				return m_union.g_dy().size();
			}
		}
		/// Static storage flag.
		/**
		 * @return \p true if the storage being used is the static one, \p false otherwise.
		 */
		bool is_static() const
		{
			return m_union.is_static();
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
		template <typename U = value_type, equality_enabler<U> = 0>
		bool operator==(const small_vector &other) const
		{
			// NOTE: it seems like in C++14 the check on equal sizes is embedded in std::equal
			// when using the new algorithm signature:
			// http://en.cppreference.com/w/cpp/algorithm/equal
			// Just keep it in mind for the future.
			const unsigned mask = static_cast<unsigned>(m_union.is_static()) +
				(static_cast<unsigned>(other.m_union.is_static()) << 1u);
			switch (mask)
			{
				case 0u:
					return m_union.g_dy().size() == other.m_union.g_dy().size() &&
						std::equal(m_union.g_dy().begin(),m_union.g_dy().end(),other.m_union.g_dy().begin());
				case 1u:
					return m_union.g_st().size() == other.m_union.g_dy().size() &&
						std::equal(m_union.g_st().begin(),m_union.g_st().end(),other.m_union.g_dy().begin());
				case 2u:
					return m_union.g_dy().size() == other.m_union.g_st().size() &&
						std::equal(m_union.g_dy().begin(),m_union.g_dy().end(),other.m_union.g_st().begin());
			}
			return m_union.g_st().size() == other.m_union.g_st().size() &&
				std::equal(m_union.g_st().begin(),m_union.g_st().end(),other.m_union.g_st().begin());
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
		template <typename U = value_type, equality_enabler<U> = 0>
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
		template <typename U = value_type, hash_enabler<U> = 0>
		std::size_t hash() const noexcept
		{
			if (m_union.is_static()) {
				return m_union.g_st().hash();
			} else {
				return m_union.g_dy().hash();
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
			if (m_union.is_static()) {
				if (size <= max_static_size) {
					m_union.g_st().resize(static_cast<typename s_storage::size_type>(size));
				} else {
					// NOTE: this check is a repetition of an existing check in dynamic storage's
					// resize. The reason for putting it here as well is to ensure the safety
					// of the static_cast below.
					if (unlikely(resize_check_size(size,std::integral_constant<bool,need_resize_check>()))) {
						piranha_throw(std::bad_alloc,);
					}
					// Move the existing elements into new dynamic storage.
					const auto d_size = static_cast<typename d_storage::size_type>(size);
					d_storage tmp_d;
					tmp_d.reserve(d_size);
					// NOTE: this will not throw, as tmp_d is guaranteed to be of adequate size
					// and thus push_back() will not fail.
					std::move(m_union.g_st().begin(),m_union.g_st().end(),std::back_inserter(tmp_d));
					// Fill in the missing elements.
					tmp_d.resize(d_size);
					// Destroy static, move in dynamic.
					m_union.g_st().~s_storage();
					::new (static_cast<void *>(&(m_union.m_dy))) d_storage(std::move(tmp_d));
				}
			} else {
				if (unlikely(resize_check_size(size,std::integral_constant<bool,need_resize_check>()))) {
					piranha_throw(std::bad_alloc,);
				}
				m_union.g_dy().resize(static_cast<typename d_storage::size_type>(size));
			}
		}
		/// Vector addition.
		/**
		 * \note
		 * This method is enabled only if \p value_type is addable and if the result
		 * of the addition is move-assignable to \p value_type.
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
		template <typename U = value_type, add_enabler<U> = 0>
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
		template <typename U, typename = void>
		struct adder
		{
			auto operator()(const U &a, const U &b) -> decltype(a + b)
			{
				return a + b;
			}
		};
		// NOTE: need this to silence warnings when operating on short ints: they will get
		// promoted to int during addition, hence resulting in a warning when casting back down
		// to short int on return.
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
			if (m_union.is_static()) {
				if (m_union.g_st().size() == m_union.g_st().max_size) {
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
					tmp_d.reserve(static_cast<d_size_type>(static_cast<d_size_type>(m_union.g_st().max_size) + 1u));
					std::move(m_union.g_st().begin(),m_union.g_st().end(),std::back_inserter(tmp_d));
					// Push back the new element.
					tmp_d.push_back(std::forward<U>(x));
					// Now destroy the current static storage, and move-construct new dynamic storage.
					// NOTE: in face of custom allocators here we should be ok, as move construction
					// of custom alloc will not throw and it will preserve the ownership of the moved-in elements.
					m_union.g_st().~s_storage();
					::new (static_cast<void *>(&(m_union.m_dy))) d_storage(std::move(tmp_d));
				} else {
					m_union.g_st().push_back(std::forward<U>(x));
				}
			} else {
				// In case we are already in dynamic storage, don't do anything special.
				m_union.g_dy().push_back(std::forward<U>(x));
			}
		}
	private:
		u_type m_union;
};

template <typename T, typename S>
const typename std::decay<decltype(small_vector<T,S>::s_storage::max_size)>::type small_vector<T,S>::max_static_size;

template <typename T, typename S>
const typename std::decay<decltype(small_vector<T,S>::d_storage::max_size)>::type small_vector<T,S>::max_dynamic_size;

template <typename T, typename S>
const typename small_vector<T,S>::size_type small_vector<T,S>::max_size;

}

#endif
