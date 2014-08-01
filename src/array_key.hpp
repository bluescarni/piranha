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
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "debug_access.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "small_vector.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Type trait for types suitable for use as values in piranha::array_key.
/**
 * The requisites for type \p T are the following:
 * - it must satisfy piranha::is_container_element,
 * - it must be constructible from \p int,
 * - references must be assignable from \p T,
 * - it must be equality-comparable and less-than comparable,
 * - it must be addable and subtractable (both in binary and unary form),
 * - it must satisfy piranha::is_ostreamable,
 * - it must satisfy piranha::has_is_zero,
 * - it must satisfy piranha::is_hashable.
 */
template <typename T>
class is_array_key_value_type
{
	public:
		// NOTE: these requirements are not all used in the implementation of array_key,
		// they are here to offer a comfortable base to implement array keys without too much hassle.
		/// Value of the type trait.
		static const bool value = is_container_element<T>::value &&
					  std::is_constructible<T,int>::value &&
					  std::is_assignable<T &,T>::value &&
					  is_equality_comparable<T>::value &&
					  is_less_than_comparable<T>::value &&
					  is_addable<T>::value && is_addable_in_place<T>::value &&
					  is_subtractable<T>::value && is_subtractable_in_place<T>::value &&
					  is_ostreamable<T>::value && has_is_zero<T>::value &&
					  is_hashable<T>::value;
};

template <typename T>
const bool is_array_key_value_type<T>::value;

/// Array key.
/**
 * Key type represented by an array-like sequence of instances of type \p T. Interface and semantics
 * mimic those of \p std::vector. The underlying container used to store the elements is piranha::small_vector.
 * The template argument \p S is passed down as second argument to piranha::small_vector in the definition
 * of the internal container.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p T must satisfy piranha::is_array_key_value_type,
 * - \p Derived must derive from piranha::array_key of \p T and \p Derived,
 * - \p Derived must satisfy the piranha::is_container_element type trait.
 * - \p S must be suitable as second template argument to piranha::small_vector.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to the move semantics of piranha::small_vector.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo think about introducing range-checking in element access not only in debug mode, to make it completely
 * safe to use.
 */
template <typename T, typename Derived, typename S = std::integral_constant<std::size_t,0u>>
class array_key
{
		PIRANHA_TT_CHECK(is_array_key_value_type,T);
		template <typename U>
		friend class debug_access;
		using container_type = small_vector<T,S>;
	public:
		/// Value type.
		using value_type = typename container_type::value_type;
		/// Iterator type.
		using iterator = typename container_type::iterator;
		/// Const iterator type.
		using const_iterator = typename container_type::const_iterator;
		/// Size type.
		using size_type = typename container_type::size_type;
		/// Defaulted default constructor.
		array_key() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::small_vector.
		 */
		array_key(const array_key &) = default;
		/// Defaulted move constructor.
		array_key(array_key &&) = default;
		/// Constructor from initializer list.
		/**
		 * \note
		 * This constructor is enabled only if piranha::small_vector is constructible from \p list.
		 *
		 * \p list will be forwarded to construct the internal piranha::small_vector.
		 * 
		 * @param[in] list initializer list.
		 * 
		 * @throws unspecified any exception thrown by the corresponding constructor of piranha::small_vector.
		 */
		template <typename U, typename = typename std::enable_if<std::is_constructible<container_type,
			std::initializer_list<U>>::value>::type>
		explicit array_key(std::initializer_list<U> list) : m_container(list) {}
		/// Constructor from symbol set.
		/**
		 * The key will be created with a number of variables equal to <tt>args.size()</tt>
		 * and filled with elements constructed from the integral constant 0.
		 * 
		 * @param[in] args piranha::symbol_set used for construction.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::small_vector::push_back(),
		 * - the construction of instances of type \p value_type from the integral constant 0.
		 */
		explicit array_key(const symbol_set &args)
		{
			for (decltype(args.size()) i = 0u; i < args.size(); ++i) {
				m_container.push_back(value_type(0));
			}
		}
		/// Constructor from piranha::array_key parametrized on a generic type.
		/**
		 * \note
		 * This constructor is enabled if \p value_type is constructible from \p U.
		 *
		 * Generic constructor for use in series, when inserting a term of different type.
		 * The internal container will be initialised with the
		 * contents of the internal container of \p x (possibly converting the individual contained values through a suitable
		 * converting constructor,
		 * if the values are of different type). If the size of \p x is different from the size of \p args, a runtime error will
		 * be produced.
		 * 
		 * @param[in] other construction argument.
		 * @param[in] args reference piranha::symbol_set.
		 *
		 * @throws std::invalid_argument if the sizes of \p x and \p args differ.
		 * @throws unspecified any exception thrown by:
		 * - piranha::small_vector::push_back(),
		 * - construction of piranha::array_key::value_type from \p U.
		 */
		template <typename U, typename Derived2, typename S2, typename = typename std::enable_if<
			std::is_constructible<value_type,U const &>::value
			>::type>
		explicit array_key(const array_key<U,Derived2,S2> &other, const symbol_set &args)
		{
			if (unlikely(other.size() != args.size())) {
				piranha_throw(std::invalid_argument,"inconsistent sizes in generic array_key constructor");
			}
			std::transform(other.begin(),other.end(),
				std::back_inserter(m_container),[](const U &x) {return value_type(x);});
		}
		/// Trivial destructor.
		~array_key()
		{
			PIRANHA_TT_CHECK(is_container_element,Derived);
			PIRANHA_TT_CHECK(std::is_base_of,array_key,Derived);
		}
		/// Defaulted copy assignment operator.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::small_vector.
		 *
		 * @return reference to \p this.
		 */
		array_key &operator=(const array_key &) = default;
		/// Defaulted move assignment operator.
		array_key &operator=(array_key &&) = default;
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
		/// Size.
		/**
		 * @return number of elements stored within the container.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Resize the internal array container.
		/**
		 * Equivalent to piranha::small_vector::resize().
		 * 
		 * @param[in] new_size desired new size for the internal container.
		 * 
		 * @throws unspecified any exception thrown by piranha::small_vector::resize().
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
		 * @return const reference to the element of the container at index \p i.
		 */
		const value_type &operator[](const size_type &i) const
		{
			piranha_assert(i < size());
			return m_container[i];
		}
		/// Hash value.
		/**
		 * @return hash value of the key, computed via piranha::small_vector::hash().
		 */
		std::size_t hash() const noexcept
		{
			return m_container.hash();
		}
		/// Move-add element at the end.
		/**
		 * Move-add \p x at the end of the internal container.
		 * 
		 * @param[in] x element to be added to the internal container.
		 * 
		 * @throws unspecified any exception thrown by piranha::small_vector::push_back().
		 */
		void push_back(value_type &&x)
		{
			m_container.push_back(std::move(x));
		}
		/// Copy-add element at the end.
		/**
		 * Copy-add \p x at the end of the internal container.
		 * 
		 * @param[in] x element to be added to the internal container.
		 * 
		 * @throws unspecified any exception thrown by piranha::small_vector::push_back().
		 */
		void push_back(const value_type &x)
		{
			m_container.push_back(x);
		}
		/// Equality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return the result of piranha::small_vector::operator==().
		 * 
		 * @throws unspecified any exception thrown by piranha::small_vector::operator==().
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
		/// Identify symbols that can be trimmed.
		/**
		 * This method is used in piranha::series::trim(). The input parameter \p candidates
		 * contains a set of symbols that are candidates for elimination. The method will remove
		 * from \p candidates those symbols whose elements in \p this is not zero.
		 * 
		 * @param[in] candidates set of candidates for elimination.
		 * @param[in] args reference arguments set.
		 * 
		 * @throws std::invalid_argument if the size of \p this differs from the size of \p args.
		 * @throws unspecified any exception thrown by piranha::math::is_zero() or piranha::symbol_set::remove().
		 */
		void trim_identify(symbol_set &candidates, const symbol_set &args) const
		{
			if (unlikely(m_container.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid arguments set for trim_identify()");
			}
			for (size_type i = 0u; i < m_container.size(); ++i) {
				if (!math::is_zero(m_container[i]) && std::binary_search(candidates.begin(),candidates.end(),args[i])) {
					candidates.remove(args[i]);
				}
			}
		}
		/// Trim.
		/**
		 * This method will return a copy of \p this with the elements associated to the symbols
		 * in \p trim_args removed.
		 * 
		 * @param[in] trim_args arguments whose elements will be removed.
		 * @param[in] orig_args original arguments set.
		 * 
		 * @return trimmed copy of \p this.
		 * 
		 * @throws std::invalid_argument if the size of \p this differs from the size of \p args.
		 * @throws unspecified any exception thrown by push_back().
		 */
		Derived trim(const symbol_set &trim_args, const symbol_set &orig_args) const
		{
			if (unlikely(m_container.size() != orig_args.size())) {
				piranha_throw(std::invalid_argument,"invalid arguments set for trim()");
			}
			Derived retval;
			for (size_type i = 0u; i < m_container.size(); ++i) {
				if (!std::binary_search(trim_args.begin(),trim_args.end(),orig_args[i])) {
					retval.push_back(m_container[i]);
				}
			}
			return retval;
		}
	protected:
		// NOTE: the reason for this to be protected is for future implementations of trig monomial based
		// on this class.
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
		 * - piranha::small_vector::push_back(),
		 * - the construction of instances of type \p value_type from the integral constant 0.
		 */
		array_key base_merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			// NOTE: here and elsewhere (i.e., kronecker keys) the check on new_args.size() <= orig_args.size()
			// is not redundant with the std::includes check; indeed it actually checks that the new args are
			// _more_ than the old args (whereas with just the std::includes check identical orig_args and new_args
			// would be allowed).
			if (unlikely(m_container.size() != orig_args.size() || new_args.size() <= orig_args.size() ||
				!std::includes(new_args.begin(),new_args.end(),orig_args.begin(),orig_args.end())))
			{
				piranha_throw(std::invalid_argument,"invalid argument(s) for symbol set merging");
			}
			array_key retval;
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
		 * \note
		 * This method is enabled only if the call to piranha::small_vector::add()
		 * is well-formed.
		 *
		 * Equivalent to calling piranha::small_vector::add() on the internal containers of \p this
		 * and of the arguments.
		 * 
		 * @param[out] retval piranha::array_key that will hold the result of the addition.
		 * @param[in] other piranha::array_key that will be added to \p this.
		 * 
		 * @throws unspecified amy exception thrown by piranha::small_vector::add().
		 *
		 * @return the return value of piranha::small_vector::add().
		 */
		template <typename U = container_type>
		auto add(array_key &retval, const array_key &other) const -> decltype(
			std::declval<U const &>().add(std::declval<U &>(),std::declval<U const &>()))
		{
			m_container.add(retval.m_container,other.m_container);
		}
	private:
		// Internal container.
		container_type m_container;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::array_key.
template <typename T, typename Derived, typename S>
struct hash<piranha::array_key<T,Derived,S>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::array_key<T,Derived,S> argument_type;
	/// Hash operator.
	/**
	 * @param[in] a piranha::array_key whose hash value will be returned.
	 * 
	 * @return piranha::array_key::hash().
	 */
	result_type operator()(const argument_type &a) const noexcept
	{
		return a.hash();
	}
};

}

#endif
