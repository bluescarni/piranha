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

#ifndef PIRANHA_ARRAY_KEY_HPP
#define PIRANHA_ARRAY_KEY_HPP

#include <boost/concept/assert.hpp>
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "concepts/container_element.hpp"
#include "concepts/crtp.hpp"
#include "config.hpp"

namespace piranha
{

/// Array key.
/**
 * Key type represented by an array-like sequence of instances of type \p T. Interface and semantics
 * mimic those of \p std::vector.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must be a model of piranha::concept::ContainerElement.
 * - \p Derived must be a model of piranha::concept::CRTP.
 * 
 * \section exception_safety Exception safety guarantees
 * 
 * This class provides the same exception safety guarantee as \p std::vector.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics for this class are equivalent to those for \p std::vector.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T, typename Derived>
class array_key
{
		BOOST_CONCEPT_ASSERT((concept::ContainerElement<T>));
		BOOST_CONCEPT_ASSERT((concept::CRTP<array_key<T,Derived>,Derived>));
		// Underlying container.
		typedef std::vector<T> container_type;
	public:
		/// Value type.
		typedef T value_type;
		/// Size type.
		typedef typename container_type::size_type size_type;
		/// Defaulted default constructor.
		array_key() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by <tt>std::vector</tt>'s copy constructor.
		 */
		array_key(const array_key &) = default;
		/// Move constructor.
		/**
		 * @param[in] other object to move from.
		 */
		array_key(array_key &&other) piranha_noexcept_spec(true) : m_container(std::move(other.m_container))
		{}
		/// Constructor from initializer list.
		/**
		 * Will set the values of the internal array to those in \p list.
		 * 
		 * @param[in] list initializer list.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::vector</tt>'s constructor from initializer list.
		 */
		array_key(std::initializer_list<value_type> list):m_container(list) {}
		/// Trivial destructor.
		~array_key() piranha_noexcept_spec(true)
		{}
		/// Defaulted copy assignment operator.
		/**
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::vector</tt>'s copy assignment operator.
		 */
		array_key &operator=(const array_key &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment target.
		 * 
		 * @return reference to \p this.
		 */
		array_key &operator=(array_key &&other) piranha_noexcept_spec(true)
		{
			m_container = std::move(other.m_container);
			return *this;
		}
		/// Size of the internal array container.
		/**
		 * @return size of the internal array container.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Element access.
		/**
		 * @param[in] i index of the element to be accessed.
		 * 
		 * @return reference to the <tt>i</tt>-th element of the container.
		 */
		T &operator[](const size_type &i)
		{
			return m_container[i];
		}
		/// Const element access.
		/**
		 * @param[in] i index of the element to be accessed.
		 * 
		 * @return const reference to the <tt>i</tt>-th element of the container.
		 */
		const T &operator[](const size_type &i) const
		{
			return m_container[i];
		}
		/// Hash value.
		/**
		 * @return 0 if size() is 0, otherwise the result of mixing with \p boost::hash_combine the hash
		 * values of the elements of the container, calculated via a default-constructed \p std::hash instance.
		 * 
		 * @see http://www.boost.org/doc/libs/release/doc/html/hash/combine.html
		 */
		std::size_t hash() const
		{
			std::size_t retval = 0;
			const auto size = m_container.size();
			std::hash<T> hasher;
			for (decltype(m_container.size()) i = 0; i < size; ++i) {
				boost::hash_combine(retval,hasher(m_container[i]));
			}
			return retval;
		}
		/// Add element at the end.
		/**
		 * Move-add \p x at the end of the internal array.
		 * 
		 * @param[in] x element to be added to the internal array.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::vector::push_back()</tt>.
		 */
		void push_back(value_type &&x)
		{
			m_container.push_back(std::move(x));
		}
		/// Add element at the end.
		/**
		 * Copy-add \p x at the end of the internal array.
		 * 
		 * @param[in] x element to be added to the internal array.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::vector::push_back()</tt>.
		 */
		void push_back(const value_type &x)
		{
			m_container.push_back(x);
		}
		/// Equality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return true if <tt>other</tt>'s size() matches the size of \p this and all elements are equal,
		 * false otherwise.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::vector</tt>'s equality operator.
		 */
		bool operator==(const array_key &other) const
		{
			return m_container == other.m_container;
		}
		/// Inequality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return negation of operator==().
		 * 
		 * @throws unspecified any exception thrown by <tt>std::vector</tt>'s equality operator.
		 */
		bool operator!=(const array_key &other) const
		{
			return !operator==(other);
		}
		/// Overload of stream operator for piranha::array_key.
		/**
		 * Will direct to stream a human-readable representation of the key.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] a piranha::array_key that will be directed to \p os.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by directing to the stream instances of piranha::array_key::value_type.
		 */
		friend inline std::ostream &operator<<(std::ostream &os, const array_key &a)
		{
			typedef decltype(a.m_container.size()) size_type;
			os << '[';
			for (size_type i = 0; i < a.m_container.size(); ++i) {
				os << a[i];
				if (i != a.m_container.size() - static_cast<size_type>(1)) {
					os << ',';
				}
			}
			os << ']';
			return os;
		}
	private:
		container_type m_container;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::array_key.
template <typename T, typename Derived>
struct hash<piranha::array_key<T,Derived>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::array_key<T,Derived> argument_type;
	/// Hash operator.
	/**
	 * @param[in] a piranha::array_key whose hash value will be returned.
	 * 
	 * @return piranha::array_key::hash().
	 */
	result_type operator()(const argument_type &a) const
	{
		return a.hash();
	}
};

}

#endif
