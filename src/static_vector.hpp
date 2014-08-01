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

#ifndef PIRANHA_STATIC_VECTOR_HPP
#define PIRANHA_STATIC_VECTOR_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <tuple>
#include <type_traits>

#include "config.hpp"
#include "detail/small_vector_fwd.hpp"
#include "detail/vector_hasher.hpp"
#include "exceptions.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// TMP to determine size type big enough to represent Size. The candidate types are the fundamental unsigned integer types.
using static_vector_size_types = std::tuple<unsigned char, unsigned short, unsigned, unsigned long, unsigned long long>;

template <std::size_t Size, std::size_t Index = 0u>
struct static_vector_size_type
{
	using candidate_type = typename std::tuple_element<Index,static_vector_size_types>::type;
	using type = typename std::conditional<(std::numeric_limits<candidate_type>::max() >= Size),
		candidate_type,
		typename static_vector_size_type<Size,static_cast<std::size_t>(Index + 1u)>::type>::type;
};

template <std::size_t Size>
struct static_vector_size_type<Size,static_cast<std::size_t>(std::tuple_size<static_vector_size_types>::value - 1u)>
{
	using type = typename std::tuple_element<static_cast<std::size_t>(std::tuple_size<static_vector_size_types>::value - 1u),
		static_vector_size_types>::type;
	static_assert(std::numeric_limits<type>::max() >= Size,"Cannot determine size type for static vector.");
};

}

/// Static vector class.
/**
 * This class represents a dynamic array that avoids runtime memory allocation. Memory storage is provided by
 * a contiguous memory block stored within the class. The size of the internal storage is enough to fit
 * at least \p MaxSize objects.
 * 
 * The interface of this class mimics part of the interface of \p std::vector.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must satisfy piranha::is_container_element.
 * - \p MaxSize must be non-null.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * After a move operation, the container will be left in a state equivalent to a default-constructed instance.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T, std::size_t MaxSize>
class static_vector
{
		static_assert(MaxSize > 0u,"Maximum size must be strictly positive.");
		template <typename, typename>
		friend union detail::small_vector_union;
	public:
		/// Size type.
		/**
		 * An unsigned integer type large enough to represent \p MaxSize.
		 */
		using size_type = typename detail::static_vector_size_type<MaxSize>::type;
	private:
		PIRANHA_TT_CHECK(is_container_element,T);
		// This check is against overflows when using memcpy.
		static_assert(std::numeric_limits<size_type>::max() <= std::numeric_limits<std::size_t>::max() / sizeof(T),
			"The size type for static_vector might overflow.");
		// Check for overflow in the definition of the storage type.
		static_assert(MaxSize < std::numeric_limits<std::size_t>::max() / sizeof(T),"Overflow in the computation of storage size.");
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
		//   still produce addresses at which the object can be constructed;
		// - in general, we are assuming here that we can handle contiguous storage the same way arrays can be handled (e.g.,
		//   to get the end() iterator we get one past the last element);
		// - note that placement new will work as expected (i.e., it will construct the object exactly at the address passed
		//   in as parameter).
		typedef typename std::aligned_storage<sizeof(T) * MaxSize,alignof(T)>::type storage_type;
	public:
		/// Maximum size.
		/**
		 * Alias for \p MaxSize.
		 */
		static const size_type max_size = MaxSize;
		/// Contained type.
		typedef T value_type;
		/// Iterator type.
		typedef value_type * iterator;
		/// Const iterator type.
		typedef value_type const * const_iterator;
		/// Default constructor.
		/**
		 * Will construct a vector of size 0.
		 */
		static_vector():m_tag(1u),m_size(0u) {}
		/// Copy constructor.
		/**
		 * @param[in] other target of the copy operation.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		static_vector(const static_vector &other):m_tag(1u),m_size(0u)
		{
			// NOTE: here and elsewhere, the standard implies (3.9/2) that we can use this optimisation
			// for trivially copyable types. GCC does not support the type trait yet, so we restrict the
			// optimisation to POD types (which are trivially copyable).
			if (std::is_pod<T>::value) {
				std::memcpy(vs(),other.vs(),other.m_size * sizeof(T));
				m_size = other.m_size;
			} else {
				try {
					const auto size = other.size();
					for (size_type i = 0u; i < size; ++i) {
						push_back(other[i]);
					}
				} catch (...) {
					// Roll back the constructed items before re-throwing.
					destroy_items();
					throw;
				}
			}
		}
		/// Move constructor.
		/**
		 * @param[in] other target of the move operation.
		 */
		static_vector(static_vector &&other) noexcept:m_tag(1u),m_size(0u)
		{
			const auto size = other.size();
			if (std::is_pod<T>::value) {
				std::memcpy(vs(),other.vs(),size * sizeof(T));
				m_size = size;
			} else {
				// NOTE: here no need for rollback, as we assume move ctors do not throw.
				for (size_type i = 0u; i < size; ++i) {
					push_back(std::move(other[i]));
				}
			}
			// Nuke other.
			other.destroy_items();
			other.m_size = 0u;
		}
		/// Constructor from multiple copies.
		/**
		 * Will construct a vector containing \p n copies of \p x.
		 * 
		 * @param[in] n number of copies of \p x that will be inserted in the vector.
		 * @param[in] x element whose copies will be inserted in the vector.
		 * 
		 * @throws unspecified any exception thrown by push_back().
		 */
		explicit static_vector(const size_type &n, const value_type &x):m_tag(1u),m_size(0u)
		{
			try {
				for (size_type i = 0u; i < n; ++i) {
					push_back(x);
				}
			} catch (...) {
				destroy_items();
				throw;
			}
		}
		/// Destructor.
		/**
		 * Will destroy all elements of the vector.
		 */
		~static_vector()
		{
			// NOTE: here we should replace with bidirectional tt, if we ever implement it.
			PIRANHA_TT_CHECK(is_forward_iterator,iterator);
			PIRANHA_TT_CHECK(is_forward_iterator,const_iterator);
			piranha_assert(m_tag == 1u);
			destroy_items();
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other target of the copy assignment operation.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		static_vector &operator=(const static_vector &other)
		{
			if (likely(this != &other)) {
				if (std::is_pod<T>::value) {
					std::memcpy(vs(),other.vs(),other.m_size * sizeof(T));
					m_size = other.m_size;
				} else {
					static_vector tmp(other);
					*this = std::move(tmp);
				}
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other target of the move assignment operation.
		 * 
		 * @return reference to \p this.
		 */
		static_vector &operator=(static_vector &&other) noexcept
		{
			if (likely(this != &other)) {
				if (std::is_pod<T>::value) {
					std::memcpy(vs(),other.vs(),other.m_size * sizeof(T));
				} else {
					const size_type old_size = m_size, new_size = other.m_size;
					if (old_size <= new_size) {
						// Move assign new content into old content.
						for (size_type i = 0u; i < old_size; ++i) {
							(*this)[i] = std::move(other[i]);
						}
						// Move construct remaining elements.
						for (size_type i = old_size; i < new_size; ++i) {
							::new (static_cast<void *>(ptr() + i)) value_type(std::move(other[i]));
						}
					} else {
						// Move assign new content into old content.
						for (size_type i = 0u; i < new_size; ++i) {
							(*this)[i] = std::move(other[i]);
						}
						// Destroy excess content.
						for (size_type i = new_size; i < old_size; ++i) {
							ptr()[i].~T();
						}
					}
				}
				m_size = other.m_size;
				// Nuke the other.
				other.destroy_items();
				other.m_size = 0u;
			}
			return *this;
		}
		/// Const index operator.
		/**
		 * @param[in] n index of the desired element.
		 * 
		 * @return const reference to the element contained at index \p n;
		 */
		const value_type &operator[](const size_type &n) const
		{
			piranha_assert(n < m_size);
			return ptr()[n];
		}
		/// Index operator.
		/**
		 * @param[in] n index of the desired element.
		 * 
		 * @return reference to the element contained at index \p n;
		 */
		value_type &operator[](const size_type &n)
		{
			piranha_assert(n < m_size);
			return ptr()[n];
		}
		/// Begin iterator.
		/**
		 * @return iterator to the first element, or end() if the vector is empty.
		 */
		iterator begin()
		{
			return ptr();
		}
		/// End iterator.
		/**
		 * @return iterator to the position one past the last element of the vector.
		 */
		iterator end()
		{
			return ptr() + m_size;
		}
		/// Const begin iterator.
		/**
		 * @return const iterator to the first element, or end() if the vector is empty.
		 */
		const_iterator begin() const
		{
			return ptr();
		}
		/// Const end iterator.
		/**
		 * @return const iterator to the position one past the last element of the vector.
		 */
		const_iterator end() const
		{
			return ptr() + m_size;
		}
		/// Equality operator.
		/**
		 * @param[in] other argument for the comparison.
		 * 
		 * @return \p true if the sizes of the vectors are the same and all elements of \p this compare as equal
		 * to the elements in \p other, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the comparison operator of the value type.
		 */
		bool operator==(const static_vector &other) const
		{
			return (m_size == other.m_size && std::equal(begin(),end(),other.begin()));
		}
		/// Inequality operator.
		/**
		 * @param[in] other argument for the comparison.
		 * 
		 * @return the opposite of operator==().
		 * 
		 * @throws unspecified any exception thrown by operator==().
		 */
		bool operator!=(const static_vector &other) const
		{
			return !operator==(other);
		}
		/// Copy-add element at the end of the vector.
		/**
		 * \p x is copy-inserted at the end of the container.
		 * 
		 * @param[in] x element to be inserted.
		 * 
		 * @throws std::bad_alloc if the insertion of \p x would lead to a size greater than \p MaxSize.
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		void push_back(const value_type &x)
		{
			if (unlikely(m_size == MaxSize)) {
				piranha_throw(std::bad_alloc,);
			}
			::new (static_cast<void *>(ptr() + m_size)) value_type(x);
			++m_size;
		}
		/// Move-add element at the end of the vector.
		/**
		 * \p x is move-inserted at the end of the container.
		 * 
		 * @param[in] x element to be inserted.
		 * 
		 * @throws std::bad_alloc if the insertion of \p x would lead to a size greater than \p MaxSize.
		 */
		void push_back(value_type &&x)
		{
			if (unlikely(m_size == MaxSize)) {
				piranha_throw(std::bad_alloc,);
			}
			::new (static_cast<void *>(ptr() + m_size)) value_type(std::move(x));
			++m_size;
		}
		/// Construct in-place at the end of the vector.
		/**
		 * \note
		 * This method is enabled only if \p value_type is constructible from the variadic arguments pack.
		 *
		 * Input parameters will be used to construct an instance of \p T at the end of the container.
		 * 
		 * @param[in] params arguments that will be used to construct the new element.
		 * 
		 * @throws std::bad_alloc if the insertion of the new element would lead to a size greater than \p MaxSize.
		 * @throws unspecified any exception thrown by the constructor of \p T from the input parameters.
		 */
		template <typename... Args, typename = typename std::enable_if<std::is_constructible<value_type,Args && ...>::value>::type>
		void emplace_back(Args && ... params)
		{
			if (unlikely(m_size == MaxSize)) {
				piranha_throw(std::bad_alloc,);
			}
			::new (static_cast<void *>(ptr() + m_size)) value_type(std::forward<Args>(params)...);
			++m_size;
		}
		/// Size.
		/**
		 * @return the number of elements currently stored in \p this.
		 */
		size_type size() const
		{
			return m_size;
		}
		/// Resize.
		/**
		 * After this operation, the number of elements stored in the container will be \p new_size. If \p new_size is greater than
		 * the size of the object before the operation, the new elements will be value-initialized and placed at the end of the container.
		 * If \p new_size is smaller than the size of the object before the operation, the first \p new_size
		 * object in the vector will be preserved.
		 * 
		 * @param[in] new_size new size for the vector.
		 * 
		 * @throws std::bad_alloc if \p new_size is greater than \p MaxSize.
		 * @throws unspecified any exception thrown by the default constructor of \p T.
		 */
		void resize(const size_type &new_size)
		{
			if (unlikely(new_size > MaxSize)) {
				piranha_throw(std::bad_alloc,);
			}
			const auto old_size = m_size;
			if (new_size == old_size) {
				return;
			} else if (new_size > old_size) {
				size_type i = old_size;
				// Construct new in case of larger size.
				try {
					for (; i < new_size; ++i) {
						// NOTE: placement-new syntax for value initialization, the same as performed by
						// std::vector (see 23.3.6.2 and 23.3.6.3/9).
						// http://en.cppreference.com/w/cpp/language/value_initialization
						::new (static_cast<void *>(ptr() + i)) value_type();
						piranha_assert(m_size != MaxSize);
						++m_size;
					}
				} catch (...) {
					for (size_type j = old_size; j < i; ++j) {
						ptr()[j].~T();
						piranha_assert(m_size);
						--m_size;
					}
					throw;
				}
			} else {
				// Destroy in case of smaller size.
				for (size_type i = new_size; i < old_size; ++i) {
					ptr()[i].~T();
					piranha_assert(m_size);
					--m_size;
				}
			}
		}
		/// Hash value.
		/**
		 * \note
		 * This method is enabled only if \p T satisfies piranha::is_hashable.
		 *
		 * @return one of the following:
		 * - 0 if size() is 0,
		 * - the hash of the first element (via \p std::hash) if size() is 1,
		 * - in all other cases, the result of iteratively mixing via \p boost::hash_combine the hash
		 *   values of all the elements of the container, calculated via \p std::hash,
		 *   with the hash value of the first element as seed value.
		 *
		 * @throws unspecified any exception resulting from calculating the hash value(s) of the
		 * individual elements.
		 *
		 * @see http://www.boost.org/doc/libs/release/doc/html/hash/combine.html
		 */
		template <typename U = T, typename = typename std::enable_if<
			is_hashable<U>::value
			>::type>
		std::size_t hash() const noexcept
		{
			return detail::vector_hasher(*this);
		}
		/// Stream operator overload for piranha::static_vector.
		/**
		 * Will print to stream a human-readable representation of \p v.
		 * 
		 * @param[in] os target stream.
		 * @param[in] v vector to be streamed.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by printing to stream instances of the value type of \p v.
		 */
		friend std::ostream &operator<<(std::ostream &os, const static_vector &v)
		{
			os << '[';
			for (decltype(v.size()) i = 0u; i < v.size(); ++i) {
				os << v[i];
				if (i != v.size() - 1u) {
					os << ',';
				}
			}
			os << ']';
			return os;
		}
	private:
		void destroy_items()
		{
			// NOTE: no need to destroy anything in this case:
			// http://en.cppreference.com/w/cpp/language/destructor
			// NOTE: here we could have the destructor defined in a base class to be specialised
			// if T is trivially destructible. Then we can default the dtor here and static_vector
			// will be trivially destructible if T is.
			if (std::is_trivially_destructible<T>::value) {
				return;
			}
			const auto size = m_size;
			for (size_type i = 0u; i < size; ++i) {
				ptr()[i].~T();
			}
		}
		// Getters for storage cast to void.
		void *vs()
		{
			return static_cast<void *>(&m_storage);
		}
		const void *vs() const
		{
			return static_cast<const void *>(&m_storage);
		}
		// Getters for T pointer.
		T *ptr()
		{
			return static_cast<T *>(vs());
		}
		const T *ptr() const
		{
			return static_cast<const T *>(vs());
		}
	private:
		unsigned char	m_tag;
		size_type	m_size;
		storage_type	m_storage;
};

template <typename T, std::size_t MaxSize>
const typename static_vector<T,MaxSize>::size_type static_vector<T,MaxSize>::max_size;

}

#endif
