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
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <new>
#include <type_traits>

#include "concepts/container_element.hpp"
#include "config.hpp"
#include "exceptions.hpp"

namespace piranha
{

/// Static vector class.
/**
 * This class represents a dynamic array that avoids runtime memory allocation. Memory storage is provided by an internal
 * array of statically allocated objects of type \p T. The size of the internal storage is enough to fit
 * a maximum of \p MaxSize objects.
 * 
 * The interface of this class mimics part of the interface of \p std::vector.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must be a model of piranha::concept::ContainerElement.
 * - \p MaxSize must be non-null.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety for all operations, apart from resize() (see documentation of the method).
 * 
 * \section move_semantics Move semantics
 * 
 * After a move operation, the size of the object will not change, and its elements will be left in a moved-from state.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T, std::uint8_t MaxSize>
class static_vector
{
		BOOST_CONCEPT_ASSERT((concept::ContainerElement<T>));
		static_assert(MaxSize > 0u,"Maximum size must be strictly positive.");
		typedef typename std::aligned_storage<sizeof(T[MaxSize]),alignof(T[MaxSize])>::type storage_type;
	public:
		/// Size type.
		typedef std::uint8_t size_type;
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
		static_vector():m_size(0u) {}
		/// Copy constructor.
		/**
		 * @param[in] other target of the copy operation.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		static_vector(const static_vector &other):m_size(0u)
		{
			if (std::is_pod<T>::value) {
				std::memcpy(static_cast<void *>(&m_storage),static_cast<void const *>(&other.m_storage),other.m_size * sizeof(T));
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
		 * @param[in] other target of the copy operation.
		 */
		static_vector(static_vector &&other):m_size(0u)
		{
			if (std::is_pod<T>::value) {
				std::memcpy(static_cast<void *>(&m_storage),static_cast<void const *>(&other.m_storage),other.m_size * sizeof(T));
				m_size = other.m_size;
			} else {
				// NOTE: here no need for rollback, as we assume move ctors do not throw.
				const auto size = other.size();
				for (size_type i = 0u; i < size; ++i) {
					push_back(std::move(other[i]));
				}
			}
		}
		explicit static_vector(const size_type &n, const value_type &x):m_size(0u)
		{
			for (size_type i = 0u; i < n; ++i) {
				push_back(x);
			}
		}
		/// Destructor.
		/**
		 * Will destroy all elements of the vector.
		 */
		~static_vector()
		{
			if (!std::is_pod<T>::value) {
				destroy_items();
			}
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
					std::memcpy(static_cast<void *>(&m_storage),static_cast<void const *>(&other.m_storage),other.m_size * sizeof(T));
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
		static_vector &operator=(static_vector &&other)
		{
			if (likely(this != &other)) {
				if (std::is_pod<T>::value) {
					std::memcpy(static_cast<void *>(&m_storage),static_cast<void const *>(&other.m_storage),other.m_size * sizeof(T));
				} else {
					const size_type old_size = m_size, new_size = other.m_size;
					if (old_size <= new_size) {
						// Move assign new content into old content.
						for (size_type i = 0u; i < old_size; ++i) {
							(*this)[i] = std::move(other[i]);
						}
						// Move construct remaining elements.
						for (size_type i = old_size; i < new_size; ++i) {
							::new ((void *)(ptr() + i)) value_type(std::move(other[i]));
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
		 */
		bool operator==(const static_vector &other) const
		{
			return (m_size == other.m_size && std::equal(begin(),end(),other.begin()));
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
				piranha_throw(std::bad_alloc,0);
			}
			::new ((void *)(ptr() + m_size)) value_type(x);
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
				piranha_throw(std::bad_alloc,0);
			}
			::new ((void *)(ptr() + m_size)) value_type(std::move(x));
			++m_size;
		}
		/// Construct in-place at the end of the vector.
		/**
		 * Input parameters will be used to construct an instance of \p T at the end of the container.
		 * 
		 * @param[in] params arguments that will be used to construct the new element.
		 * 
		 * @throws std::bad_alloc if the insertion of the new element would lead to a size greater than \p MaxSize.
		 * @throws unspecified any exception thrown by the constructor of \p T from the input parameters.
		 */
		template <typename... Args>
		void emplace_back(Args && ... params)
		{
			if (unlikely(m_size == MaxSize)) {
				piranha_throw(std::bad_alloc,0);
			}
			::new ((void *)(ptr() + m_size)) value_type(std::forward<Args>(params)...);
			++m_size;
		}
		/// Reserve space.
		/**
		 * This method is a no-op, kept for compatibility with \p std::vector.
		 */
		void reserve(const size_type &)
		{}
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
		 * the size of the object before the operation, the new elements will be copy-constructed from a default-constructed instance of \p T
		 * and placed at the end().
		 * If \p new_size is smaller than the size of the object before the operation, the first \p new_size object in the vector will be preserved.
		 * 
		 * This method offers the strong exception safety guarantee when reducing the size of the vector or if the size does not change.
		 * If an exception is thrown while increasing the size of the vector, the object will be left in a valid but non-specified state.
		 * 
		 * @param[in] new_size new size for the vector.
		 * 
		 * @throws std::bad_alloc if \p new_size is greater than \p MaxSize.
		 * @throws unspecified any exception thrown by the copy and default constructors of \p T.
		 */
		void resize(const size_type &new_size)
		{
			if (unlikely(new_size > MaxSize)) {
				piranha_throw(std::bad_alloc,0);
			}
			const auto old_size = m_size;
			if (new_size == old_size) {
				return;
			} else if (new_size > old_size) {
				// Construct new in case of larger size.
				const value_type tmp = value_type();
				for (size_type i = old_size; i < new_size; ++i) {
					::new ((void *)(ptr() + i)) value_type(tmp);
					piranha_assert(m_size != MaxSize);
					++m_size;
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
			const auto size = m_size;
			for (size_type i = 0u; i < size; ++i) {
				ptr()[i].~T();
			}
		}
		T *ptr()
		{
			return static_cast<T *>(static_cast<void *>(&m_storage));
		}
		const T *ptr() const
		{
			return static_cast<const T *>(static_cast<const void *>(&m_storage));
		}
	private:
		storage_type	m_storage;
		size_type	m_size;
};

}

#endif
