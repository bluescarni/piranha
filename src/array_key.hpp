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

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "concepts/array_key_value_type.hpp"
#include "concepts/container_element.hpp"
#include "concepts/crtp.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/array_key_fwd.hpp"
#include "exceptions.hpp"
#include "static_vector.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Static size tag for piranha::array_key.
/**
 * When this class is used as parameter type in piranha::array_key, the underlying container of the
 * piranha::array_key class will be a piranha::static_vector of \p T with maximum
 * size \p MaxSize.
 */
template <typename T, std::uint_least8_t MaxSize>
struct static_size {};

/// Array key.
/**
 * Key type represented by an array-like sequence of instances of type \p T. Interface and semantics
 * mimic those of \p std::vector.
 * 
 * If \p T is piranha::static_size of \p U and \p MaxSize, then the underlying array container will
 * be piranha::static_vector of \p U and \p MaxSize (and hence the size of the array key will be
 * at most \p MaxSize). Otherwise, the underlying container will be \p std::vector.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must either be a model of piranha::concept::ArrayKeyValueType, or piranha::static_size of \p U and \p MaxSize,
 *   in which case \p U must be a model of piranha::concept::ArrayKeyValueType,
 * - \p Derived must be a model of piranha::concept::CRTP, with piranha::array_key
 *   of \p T and \p Derived as base class.
 * - \p Derived must be a model of piranha::concept::ContainerElement.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to <tt>std::vector</tt>'s or piranha::static_vector's move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T, typename Derived>
class array_key: detail::array_key_tag
{
		BOOST_CONCEPT_ASSERT((concept::CRTP<array_key<T,Derived>,Derived>));
		template <typename U>
		struct determine_container_type
		{
			BOOST_CONCEPT_ASSERT((concept::ArrayKeyValueType<U>));
			typedef std::vector<U> type;
		};
		template <typename U, std::uint_least8_t MaxSize>
		struct determine_container_type<static_size<U,MaxSize>>
		{
			typedef static_vector<U,MaxSize> type;
		};
		template <typename U>
		friend class debug_access;
	public:
		/// Internal container type.
		typedef typename determine_container_type<T>::type container_type;
		/// Value type.
		typedef typename container_type::value_type value_type;
		/// Iterator type.
		typedef typename container_type::iterator iterator;
		/// Const iterator type.
		typedef typename container_type::const_iterator const_iterator;
	private:
		template <typename U>
		static container_type forward_for_construction(U &&x, const symbol_set &args,
			typename std::enable_if<std::is_same<value_type,typename std::decay<U>::type::value_type>::value &&
			is_nonconst_rvalue_ref<U &&>::value>::type * = piranha_nullptr)
		{
			if (unlikely(args.size() != x.size())) {
				piranha_throw(std::invalid_argument,"inconsistent sizes in generic array_key constructor");
			}
			return container_type(std::move(x.m_container));
		}
		template <typename U>
		static container_type forward_for_construction(U &&x, const symbol_set &args,
			typename std::enable_if<std::is_same<value_type,typename std::decay<U>::type::value_type>::value &&
			!is_nonconst_rvalue_ref<U &&>::value>::type * = piranha_nullptr)
		{
			if (unlikely(args.size() != x.size())) {
				piranha_throw(std::invalid_argument,"inconsistent sizes in generic array_key constructor");
			}
			return container_type(x.m_container);
		}
		template <typename U>
		static void fill_for_construction(container_type &retval, U &&x,
			typename std::enable_if<is_nonconst_rvalue_ref<U &&>::value>::type * = piranha_nullptr)
		{
			for (auto it = x.begin(); it != x.end(); ++it) {
				retval.emplace_back(std::move(*it));
			}
		}
		template <typename U>
		static void fill_for_construction(container_type &retval, U &&x,
			typename std::enable_if<!is_nonconst_rvalue_ref<U &&>::value>::type * = piranha_nullptr)
		{
			for (auto it = x.begin(); it != x.end(); ++it) {
				retval.emplace_back(*it);
			}
		}
		template <typename U>
		static container_type forward_for_construction(U &&x, const symbol_set &args,
			typename std::enable_if<!std::is_same<value_type,typename std::decay<U>::type::value_type>::value>::type * = piranha_nullptr)
		{
			if (unlikely(args.size() != x.size())) {
				piranha_throw(std::invalid_argument,"inconsistent sizes in generic array_key constructor");
			}
			container_type retval;
			retval.reserve(x.size());
			fill_for_construction(retval,std::forward<U>(x));
			return retval;
		}
		template <typename U>
		static container_type construct_from_init_list(U &&init_list)
		{
			container_type retval;
			retval.reserve(init_list.size());
			fill_for_construction(retval,init_list);
			return retval;
		}
	public:
		/// Size type.
		typedef typename container_type::size_type size_type;
		/// Defaulted default constructor.
		array_key() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of the underlying container.
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
		 * The elements in \p list will be forwarded to construct the elements of the internal container.
		 * 
		 * @param[in] list initializer list.
		 * 
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector or by the construction
		 * of objects of type \p value_type from objects of type \p U.
		 */
		template <typename U>
		explicit array_key(std::initializer_list<U> list) : m_container(construct_from_init_list(list)) {}
		/// Constructor from symbol set.
		/**
		 * The key will be created with a number of variables equal to <tt>args.size()</tt>
		 * and filled with exponents constructed from the integral constant 0.
		 * 
		 * @param[in] args piranha::symbol_set used for construction.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the <tt>push_back()</tt> or <tt>reserve()</tt> methods of the underlying container,
		 * - the construction of instances of type \p value_type from the integral constant 0.
		 */
		explicit array_key(const symbol_set &args)
		{
			m_container.reserve(args.size());
			for (decltype(args.size()) i = 0u; i < args.size(); ++i) {
				push_back(value_type(0));
			}
		}
		/// Constructor from piranha::array_key parametrized on a generic type.
		/**
		 * Generic constructor for use in series, when inserting a term of different type.
		 * This template is activated only if \p U is an instance of piranha::array_key.
		 * 
		 * The internal container will be initialised with the
		 * contents of the internal container of \p x (possibly converting the individual contained values through a suitable converting constructor,
		 * if the values are of different type). If the size of \p x is different from the size of \p args, a runtime error will
		 * be produced.
		 * 
		 * @param[in] x construction argument.
		 * @param[in] args reference piranha::symbol_set.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the <tt>push_back()</tt> or <tt>reserve()</tt> methods of the underlying container,
		 * - construction of piranha::array_key::value_type from the value type of \p U.
		 * @throws std::invalid_argument if the sizes of \p x and \p args differ.
		 */
		template <typename U>
		explicit array_key(U &&x, const symbol_set &args,
			typename std::enable_if<std::is_base_of<detail::array_key_tag,typename std::decay<U>::type>::value>::type * = piranha_nullptr)
			:m_container(forward_for_construction(std::forward<U>(x),args))
		{
			piranha_assert(std::is_sorted(args.begin(),args.end()));
		}
		/// Trivial destructor.
		~array_key() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::ContainerElement<Derived>));
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other array to be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of the underlying container.
		 */
		array_key &operator=(const array_key &other)
		{
			if (likely(this != &other)) {
				array_key tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other assignment target.
		 * 
		 * @return reference to \p this.
		 */
		array_key &operator=(array_key &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				m_container = std::move(other.m_container);
			}
			return *this;
		}
		/// Begin iterator.
		/**
		 * @return iterator to the first element of the internal container.
		 */
		iterator begin()
		{
			return m_container.begin();
		}
		/// End iterator.
		/**
		 * @return iterator one past the last element of the internal container.
		 */
		iterator end()
		{
			return m_container.end();
		}
		/// Begin const iterator.
		/**
		 * @return const iterator to the first element of the internal container.
		 */
		const_iterator begin() const
		{
			return m_container.begin();
		}
		/// End const iterator.
		/**
		 * @return const iterator one past the last element of the internal container.
		 */
		const_iterator end() const
		{
			return m_container.end();
		}
		/// Size of the internal array container.
		/**
		 * @return size of the internal array container.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Resize the internal array container.
		/**
		 * Semantically and functionally analogous to <tt>std::vector::resize()</tt>. This method provides the basic exception
		 * safety guarantee.
		 * 
		 * @param[in] new_size desired new size for the internal container.
		 * 
		 * @throws unspecified any exception thrown by the <tt>resize()</tt> method of the underlying container.
		 */
		void resize(const size_type &new_size)
		{
			m_container.resize(new_size);
		}
		/// Element access.
		/**
		 * @param[in] i index of the element to be accessed.
		 * 
		 * @return reference to the element of the container at index \p i.
		 */
		value_type &operator[](const size_type &i)
		{
			piranha_assert(i < size());
			return m_container[i];
		}
		/// Const element access.
		/**
		 * @param[in] i index of the element to be accessed.
		 * 
		 * @return cosnt reference to the element of the container at index \p i.
		 */
		const value_type &operator[](const size_type &i) const
		{
			piranha_assert(i < size());
			return m_container[i];
		}
		/// Hash value.
		/**
		 * @return one of the following:
		 * - 0 if size() is 0,
		 * - the hash of the first element (via \p std::hash) if size() is 1,
		 * - in all other cases, the result of iteratively mixing via \p boost::hash_combine the hash
		 *   values of all the elements of the container, calculated via \p std::hash,
		 *   with the hash value of the first element as seed value.
		 * 
		 * @throws unspecified any exception thrown by <tt>operator()</tt> of \p std::hash of \p value_type.
		 * 
		 * @see http://www.boost.org/doc/libs/release/doc/html/hash/combine.html
		 */
		std::size_t hash() const
		{
			const auto size = m_container.size();
			switch (size) {
				case 0u:
					return 0u;
				case 1u:
				{
					std::hash<value_type> hasher;
					return hasher(m_container[0u]);
				}
				default:
				{
					std::hash<value_type> hasher;
					std::size_t retval = hasher(m_container[0u]);
					for (decltype(m_container.size()) i = 1u; i < size; ++i) {
						boost::hash_combine(retval,hasher(m_container[i]));
					}
					return retval;
				}
			}
		}
		/// Add element at the end.
		/**
		 * Move-add \p x at the end of the internal array.
		 * 
		 * @param[in] x element to be added to the internal array.
		 * 
		 * @throws unspecified any exception thrown by the <tt>push_back()</tt> method of the internal container.
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
		 * @throws unspecified any exception thrown by the <tt>push_back()</tt> method of the internal container.
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
		 * @throws unspecified any exception thrown by the equality operator of the internal container.
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
		 * @throws unspecified any exception thrown by the equality operator.
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
		friend std::ostream &operator<<(std::ostream &os, const array_key &a)
		{
			typedef decltype(a.m_container.size()) size_type;
			os << '[';
			for (size_type i = 0u; i < a.m_container.size(); ++i) {
				os << a[i];
				if (i != a.m_container.size() - static_cast<size_type>(1)) {
					os << ',';
				}
			}
			os << ']';
			return os;
		}
	protected:
		/// Merge arguments.
		/**
		 * Merge the new arguments set \p new_args into \p this, given the current reference arguments set
		 * \p orig_args. Arguments in \p new_args not appearing in \p orig_args will be inserted in the internal container
		 * after being constructed from the integral constant 0.
		 * 
		 * @param[in] orig_args current reference arguments set for \p this.
		 * @param[in] new_args new arguments set.
		 * 
		 * @return piranha::array_key resulting from merging \p new_args into \p this.
		 * 
		 * @throws std::invalid_argument in the following cases:
		 * - the size of \p this is different from the size of \p orig_args,
		 * - the size of \p new_args is not greater than the size of \p orig_args,
		 * - not all elements of \p orig_args are included in \p new_args.
		 * @throws unspecified any exception thrown by:
		 * - the <tt>push_back()</tt> or <tt>reserve()</tt> methods of the underlying container,
		 * - the construction of instances of type \p value_type from the integral constant 0.
		 */
		array_key base_merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			if (unlikely(m_container.size() != orig_args.size() || new_args.size() <= orig_args.size() ||
				!std::includes(new_args.begin(),new_args.end(),orig_args.begin(),orig_args.end())))
			{
				piranha_throw(std::invalid_argument,"invalid argument(s) for symbol set merging");
			}
			array_key retval;
			retval.m_container.reserve(new_args.size());
			piranha_assert(std::is_sorted(orig_args.begin(),orig_args.end()));
			piranha_assert(std::is_sorted(new_args.begin(),new_args.end()));
			auto it_new = new_args.begin();
			for (size_type i = 0u; i < m_container.size(); ++i, ++it_new) {
				while (*it_new != orig_args[i]) {
					retval.m_container.push_back(value_type(0));
					piranha_assert(it_new != new_args.end());
					++it_new;
					piranha_assert(it_new != new_args.end());
				}
				retval.m_container.push_back(m_container[i]);
			}
			// Fill up arguments at the tail of new_args but not in orig_args.
			for (; it_new != new_args.end(); ++it_new) {
				retval.m_container.push_back(value_type(0));
			}
			piranha_assert(retval.size() == new_args.size());
			return retval;
		}
		/// Vector sum.
		/**
		 * The elements of \p this will be assigned to \p retval, and then summed in-place
		 * with the elements of \p other.
		 * 
		 * If any error occurs, \p retval will be left in a valid but undefined state.
		 * 
		 * @param[out] retval piranha::array_key that will hold the result of the addition.
		 * @param[in] other piranha::array_key that will be added to \p this.
		 * 
		 * @throws std::invalid_argument if the sizes of \p this and \p other are different.
		 * @throws unspecified any exception thrown by:
		 * - resize(),
		 * - the assignment operator of \p T,
		 * - the in-place addition operator of \p T.
		 */
		template <typename U, typename Derived2>
		void add(array_key &retval, const array_key<U,Derived2> &other) const
		{
			if (unlikely(other.size() != size())) {
				piranha_throw(std::invalid_argument,"invalid array size");
			}
			const auto s = size();
			retval.resize(s);
			std::copy(begin(),end(),retval.begin());
			for (decltype(size()) i = 0u; i < s; ++i) {
				retval[i] += other[i];
			}
		}
	protected:
		/// Internal container.
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
	 * 
	 * @throws unspecified any exception thrown by piranha::array_key::hash().
	 */
	result_type operator()(const argument_type &a) const
	{
		return a.hash();
	}
};

}

#endif
