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

#ifndef PIRANHA_BASE_TERM_HPP
#define PIRANHA_BASE_TERM_HPP

#include <cstddef>
#include <type_traits>
#include <unordered_set>

#include "detail/base_term_fwd.hpp"
#include "math.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Base term class.
/**
 * Every term must derive from this base class, which is parametrised over a coefficient type \p Cf and a key type
 * \p Key. One mutable coefficient instance and one key instance are the only data members and they can be accessed directly.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Derived must derive from piranha::base_term of \p Cf, \p Key and \p Derived.
 * - \p Derived must satisfy piranha::is_term.
 * - \p Cf must satisfy piranha::is_cf.
 * - \p Key must satisfy piranha::is_key.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to its data members' move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo test move constructor in inherited term.
 */
template <typename Cf, typename Key, typename Derived>
class base_term
{
		PIRANHA_TT_CHECK(is_cf,Cf);
		PIRANHA_TT_CHECK(is_key,Key);
	public:
		/// Alias for coefficient type.
		typedef Cf cf_type;
		/// Alias for key type.
		typedef Key key_type;
		/// Default constructor.
		/**
		 * Will explicitly call the default constructors of <tt>Cf</tt> and <tt>Key</tt>.
		 * 
		 * @throws unspecified any exception thrown by the default constructors of \p Cf and \p Key.
		 */
		base_term():m_cf(),m_key() {}
		/// Default copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructors of \p Cf and \p Key.
		 */
		base_term(const base_term &) = default;
		/// Trivial move constructor.
		/**
		 * @param[in] other term used for construction.
		 */
		base_term(base_term &&other) noexcept : m_cf(std::move(other.m_cf)),m_key(std::move(other.m_key))
		{}
		/// Constructor from generic coefficient and key.
		/**
		 * \note
		 * This constructor is activated only if coefficient and key are constructible from \p T and \p U.
		 *
		 * This constructor will forward perfectly \p cf and \p key to construct base_term::m_cf and base_term::m_key.
		 * 
		 * @param[in] cf argument used for the construction of the coefficient.
		 * @param[in] key argument used for the construction of the key.
		 * 
		 * @throws unspecified any exception thrown by the constructors of \p Cf and \p Key.
		 */
		template <typename T, typename U, typename std::enable_if<
			std::is_constructible<Cf,T>::value && std::is_constructible<Key,U>::value,
			int>::type = 0>
		explicit base_term(T &&cf, U &&key):m_cf(std::forward<T>(cf)),m_key(std::forward<U>(key)) {}
		/// Trivial destructor.
		~base_term();
		/// Copy assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructors of \p Cf and \p Key.
		 */
		base_term &operator=(const base_term &other)
		{
			if (likely(this != &other)) {
				base_term tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Trivial move-assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		base_term &operator=(base_term &&other) noexcept
		{
			if (likely(this != &other)) {
				m_cf = std::move(other.m_cf);
				m_key = std::move(other.m_key);
			}
			return *this;
		}
		/// Equality operator.
		/**
		 * Equivalence of terms is defined by the equivalence of their keys.
		 * 
		 * @param[in] other comparison argument.
		 * 
		 * @return <tt>m_key == other.m_key</tt>.
		 * 
		 * @throws unspecified any exception thrown by the equality operators of \p Key.
		 */
		bool operator==(const base_term &other) const
		{
			return m_key == other.m_key;
		}
		/// Hash value.
		/**
		 * The term's hash value is given by its key's hash value.
		 * 
		 * @return hash value of \p m_key as calculated via a default-constructed instance of \p std::hash.
		 */
		std::size_t hash() const noexcept
		{
			return std::hash<key_type>()(m_key);
		}
		/// Compatibility test.
		/**
		 * @param[in] args reference arguments set.
		 * 
		 * @return the key's <tt>is_compatible()</tt> method's return value.
		 */
		bool is_compatible(const symbol_set &args) const noexcept
		{
			// NOTE: if this (and is_ignorable) are made re-implementable at a certain point in derived term classes,
			// we must take care of asserting noexcept on the corresponding methods in the derived class.
			return m_key.is_compatible(args);
		}
		/// Ignorability test.
		/**
		 * Note that this method is not allowed to throw, so any exception thrown by calling piranha::math::is_zero() on the coefficient
		 * will result in the termination of the program.
		 * 
		 * @param[in] args reference arguments set.
		 * 
		 * @return \p true if either the key's <tt>is_ignorable()</tt> method or piranha::math::is_zero() on the coefficient return \p true,
		 * \p false otherwise.
		 */
		bool is_ignorable(const symbol_set &args) const noexcept
		{
			return (math::is_zero(m_cf) || m_key.is_ignorable(args));
		}
		/// Coefficient member.
		mutable Cf	m_cf;
		/// Key member.
		Key		m_key;
};

namespace detail
{

template <typename T, typename = void>
struct is_term_impl
{
	static const bool value = false;
};

// NOTE: put the check on container element here in order to preempt trying to access the cf/key typedefs inside references below.
template <typename T>
struct is_term_impl<T,typename std::enable_if<is_instance_of<T,base_term>::value && is_container_element<T>::value>::type>
{
	typedef typename T::cf_type cf_type;
	typedef typename T::key_type key_type;
	static const bool value = std::is_constructible<T,cf_type const &,key_type const &>::value;
};

}

/// Type trait to detect term types.
/**
 * This type trait will be true if the decay type of \p T satisfies the following conditions:
 * - it is an instance of piranha::base_term,
 * - it satisfies piranha::is_container_element,
 * - it is constructible from two const references, one to an instance of the coefficient type,
 *   the other to an instance of the key type.
 */
template <typename T>
class is_term
{
	public:
		/// Value of the type trait.
		static const bool value = detail::is_term_impl<T>::value;
};

template <typename T>
const bool is_term<T>::value;

template <typename Cf, typename Key, typename Derived>
inline base_term<Cf,Key,Derived>::~base_term()
{
	PIRANHA_TT_CHECK(std::is_base_of,base_term,Derived);
	PIRANHA_TT_CHECK(is_term,Derived);
}

}

#endif
