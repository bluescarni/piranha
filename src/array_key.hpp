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
#include "symbol.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Array key.
/**
 * Key type represented by an array-like sequence of instances of type \p T. Interface and semantics
 * mimic those of \p std::vector.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must be a model of piranha::concept::ArrayKeyValueType.
 * - \p Derived must be a model of piranha::concept::CRTP, with piranha::array_key
 *   of \p Term and \p Derived as base class.
 * - \p Derived must be a model of piranha::concept::ContainerElement.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as \p std::vector.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to <tt>std::vector</tt>'s move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T, typename Derived>
class array_key: detail::array_key_tag
{
		BOOST_CONCEPT_ASSERT((concept::ArrayKeyValueType<T>));
		BOOST_CONCEPT_ASSERT((concept::CRTP<array_key<T,Derived>,Derived>));
		template <typename U>
		friend class debug_access;
	public:
		/// Internal container type.
		typedef std::vector<T> container_type;
		/// Value type.
		typedef T value_type;
	private:
		template <typename U>
		static container_type forward_for_construction(U &&x, const std::vector<symbol> &args,
			typename std::enable_if<std::is_same<T,typename strip_cv_ref<U>::type::value_type>::value &&
			is_nonconst_rvalue_ref<U &&>::value>::type * = piranha_nullptr)
		{
			if (unlikely(args.size() != x.size())) {
				piranha_throw(std::invalid_argument,"inconsistent sizes in generic array_key constructor");
			}
			return container_type(std::move(x.m_container));
		}
		template <typename U>
		static container_type forward_for_construction(U &&x, const std::vector<symbol> &args,
			typename std::enable_if<std::is_same<T,typename strip_cv_ref<U>::type::value_type>::value &&
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
			for (decltype(x.size()) i = 0; i < x.size(); ++i) {
				retval.push_back(value_type(std::move(x[i])));
			}
		}
		template <typename U>
		static void fill_for_construction(container_type &retval, U &&x,
			typename std::enable_if<!is_nonconst_rvalue_ref<U &&>::value>::type * = piranha_nullptr)
		{
			for (decltype(x.size()) i = 0; i < x.size(); ++i) {
				retval.push_back(value_type(x[i]));
			}
		}
		template <typename U>
		static container_type forward_for_construction(U &&x, const std::vector<symbol> &args,
			typename std::enable_if<!std::is_same<T,typename strip_cv_ref<U>::type::value_type>::value>::type * = piranha_nullptr)
		{
			if (unlikely(args.size() != x.size())) {
				piranha_throw(std::invalid_argument,"inconsistent sizes in generic array_key constructor");
			}
			container_type retval;
			if (std::is_same<decltype(x.size()),size_type>::value) {
				retval.reserve(x.size());
			}
			fill_for_construction(retval,std::forward<U>(x));
			return retval;
		}
	public:
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
		/// Constructor from vector of arguments.
		/**
		 * The key will be created with a number of variables equal to <tt>args.size()</tt>
		 * and filled with exponents constructed from the integral constant 0.
		 * 
		 * @param[in] args vector of piranha::symbol used for construction.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - <tt>std::vector::push_back()</tt> or <tt>std::vector::reserve()</tt>,
		 * - the construction of instances of type \p T from the integral constant 0.
		 */
		array_key(const std::vector<symbol> &args)
		{
			if (std::is_same<std::vector<symbol>::size_type,size_type>::value) {
				m_container.reserve(args.size());
			}
			for (decltype(args.size()) i = 0; i < args.size(); ++i) {
				push_back(T(0));
			}
		}
		/// Generic constructor.
		/**
		 * Generic constructor for use in series. This template is activated only if \p U is an instance of piranha::array_key.
		 * 
		 * The internal container will be initialised with the
		 * contents of the internal container of \p x (possibly converting the individual contained values through a suitable converting constructor,
		 * if the values are of different type). If the size of \p x is different from the size of \p args, a runtime error will
		 * be produced.
		 * 
		 * @param[in] x construction argument.
		 * @param[in] args reference vector of symbolic arguments.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - <tt>std::vector::push_back()</tt> or <tt>std::vector::reserve()</tt>,
		 * - construction of piranha::array_key::value_type from the value type of \p U.
		 * @throws std::invalid_argument if the sizes of \p x and \p args differ.
		 */
		template <typename U>
		explicit array_key(U &&x, const std::vector<symbol> &args,
			typename std::enable_if<std::is_base_of<detail::array_key_tag,typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
			:m_container(forward_for_construction(std::forward<U>(x),args))
		{}
		/// Trivial destructor.
		~array_key() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::ContainerElement<Derived>));
		}
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
		 * @return 0 if size() is 0, otherwise the result of iteratively mixing via \p boost::hash_combine the hash
		 * values of the elements of the container, calculated via a default-constructed \p std::hash instance
		 * and with 0 as initial seed value.
		 * 
		 * @throws unspecified any exception thrown by <tt>operator()</tt> of \p std::hash of \p T.
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
	protected:
		/// Merge arguments.
		/**
		 * Merge the new arguments vector \p new_args into \p this, given the current reference arguments vector
		 * \p orig_args. Arguments in \p new_args not appearing in \p orig_args will be inserted in the internal container
		 * after being constructed from the integral constant 0.
		 * 
		 * @param[in] orig_args current reference arguments vector for \p this.
		 * @param[in] new_args new arguments vector.
		 * 
		 * @return piranha::array_key resulting from merging \p new_args into \p this.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - <tt>std::vector::reserve()</tt>,
		 * - <tt>std::vector::push_back()</tt>,
		 * - the construction of instances of type \p T from the integral constant 0.
		 */
		array_key base_merge_args(const std::vector<symbol> &orig_args, const std::vector<symbol> &new_args) const
		{
			array_key retval;
			// Reserve space if we are sure that the types are the same (i.e., no risky type conversions).
			if (std::is_same<size_type,decltype(new_args.size())>::value) {
				retval.m_container.reserve(new_args.size());
			}
			piranha_assert(m_container.size() == orig_args.size());
			piranha_assert(new_args.size() > orig_args.size());
			piranha_assert(std::is_sorted(orig_args.begin(),orig_args.end()));
			piranha_assert(std::is_sorted(new_args.begin(),new_args.end()));
			auto it_new = new_args.begin();
			for (size_type i = 0; i < m_container.size(); ++i, ++it_new) {
				while (*it_new != orig_args[i]) {
					retval.m_container.push_back(value_type(0));
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
