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

#include <boost/concept/assert.hpp>
#include <cstddef>
#include <type_traits>
#include <unordered_set>

#include "concepts/coefficient.hpp"
#include "concepts/crtp.hpp"
#include "concepts/key.hpp"
#include "concepts/term.hpp"
#include "config.hpp"
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
 * - \p Derived must be a model of piranha::concept::CRTP, with piranha::base_term
 *   of \p Cf, \p Key and \p Derived as base class.
 * - \p Derived must be a model of piranha::concept::Term.
 * - \p Cf must be a model of piranha::concept::Coefficient.
 * - \p Key must be a model of piranha::concept::Key.
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
class base_term: detail::base_term_tag
{
		BOOST_CONCEPT_ASSERT((concept::CRTP<base_term<Cf,Key,Derived>,Derived>));
		BOOST_CONCEPT_ASSERT((concept::Coefficient<Cf>));
		BOOST_CONCEPT_ASSERT((concept::Key<Key>));
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
		/// Move constructor.
		/**
		 * @param[in] other term to move from.
		 */
		base_term(base_term &&other) piranha_noexcept_spec(true) : m_cf(std::move(other.m_cf)),m_key(std::move(other.m_key)) {}
		/// Constructor from generic coefficient and key.
		/**
		 * Will forward perfectly \p cf and \p key to construct base_term::m_cf and base_term::m_key.
		 * 
		 * @param[in] cf argument used for the construction of the coefficient.
		 * @param[in] key argument used for the construction of the key.
		 * 
		 * @throws unspecified any exception thrown by the constructors of \p Cf and \p Key.
		 */
		template <typename T, typename U>
		explicit base_term(T &&cf, U &&key):m_cf(std::forward<T>(cf)),m_key(std::forward<U>(key)) {}
		/// Trivial destructor.
		~base_term() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Term<Derived>));
		}
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
		/// Move-assignment operator.
		/**
		 * Will move <tt>other</tt>'s coefficient and key.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		base_term &operator=(base_term &&other) piranha_noexcept_spec(true)
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
		 * 
		 * @throws unspecified any exception thrown by the specialisation of \p std::hash for \p Key.
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
		bool is_compatible(const symbol_set &args) const piranha_noexcept_spec(true)
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
		bool is_ignorable(const symbol_set &args) const piranha_noexcept_spec(true)
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
