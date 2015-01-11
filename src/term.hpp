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

#ifndef PIRANHA_TERM_HPP
#define PIRANHA_TERM_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include "is_cf.hpp"
#include "is_key.hpp"
#include "math.hpp"
#include "serialization.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Term class.
/**
 * This class represents series terms, which are parametrised over a coefficient type \p Cf and a key type
 * \p Key. One mutable coefficient instance and one key instance are the only data members and they can be accessed directly.
 * 
 * ## Type requirements ##
 * 
 * - \p Cf must satisfy piranha::is_cf.
 * - \p Key must satisfy piranha::is_key.
 * 
 * ## Exception safety guarantee ##
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * ## Move semantics ##
 * 
 * Move semantics is equivalent to its data members' move semantics.
 *
 * ## Serialization ##
 *
 * This class supports serialization if the coefficient and key types support it.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Cf, typename Key>
class term
{
		PIRANHA_TT_CHECK(is_cf,Cf);
		PIRANHA_TT_CHECK(is_key,Key);
		// Serialization support.
		friend class boost::serialization::access;
		template <class Archive>
		void save(Archive &ar, unsigned int) const
		{
			ar & m_cf;
			ar & m_key;
		}
		template <class Archive>
		void load(Archive &ar, unsigned int)
		{
			// NOTE: the requirement on def-constructability here is implied
			// by is_cf and is_key.
			Cf cf;
			Key key;
			ar & cf;
			ar & key;
			m_cf = std::move(cf);
			m_key = std::move(key);
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
		// Enabler for the generic binary ctor.
		template <typename T, typename U>
		using binary_ctor_enabler = typename std::enable_if<
			std::is_constructible<Cf,T &&>::value && std::is_constructible<Key,U &&>::value,
			int>::type;
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
		term():m_cf(),m_key() {}
		/// Default copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructors of \p Cf and \p Key.
		 */
		term(const term &) = default;
		/// Defaulted move constructor.
		term(term &&) = default;
		/// Constructor from generic coefficient and key.
		/**
		 * \note
		 * This constructor is activated only if coefficient and key are constructible from \p T and \p U.
		 *
		 * This constructor will forward perfectly \p cf and \p key to construct coefficient and key.
		 * 
		 * @param[in] cf argument used for the construction of the coefficient.
		 * @param[in] key argument used for the construction of the key.
		 * 
		 * @throws unspecified any exception thrown by the constructors of \p Cf and \p Key.
		 */
		template <typename T, typename U, binary_ctor_enabler<T,U> = 0>
		explicit term(T &&cf, U &&key):m_cf(std::forward<T>(cf)),m_key(std::forward<U>(key)) {}
		/// Trivial destructor.
		~term()
		{
			PIRANHA_TT_CHECK(is_container_element,term);
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructors of \p Cf and \p Key.
		 */
		term &operator=(const term &other)
		{
			if (likely(this != &other)) {
				term tmp(other);
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
		term &operator=(term &&other) noexcept
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
		bool operator==(const term &other) const
		{
			return m_key == other.m_key;
		}
		/// Hash value.
		/**
		 * The term's hash value is given by its key's hash value.
		 * 
		 * @return hash value of \p m_key as calculated via a default-constructed instance of \p std::hash.
		 *
		 * @throws unspecified any exception thrown by the hash functor of \p Key.
		 */
		std::size_t hash() const
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

}

#endif
