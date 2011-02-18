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
#include <iostream>
#include <type_traits>
#include <unordered_set>

#include "detail/base_term_fwd.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Base term class.
/**
 * Every term must derive from this base class, which is parametrised over a coefficient type \p Cf and a key type
 * \p Key. One coefficient instance and one key instance are the only data members and they can be accessed directly.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Cf and \p Key must not be pointer types, reference types or cv-qualified, otherwise a static assertion will fail.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo test move constructor in inherited term.
 * \todo exception safety specifications and implementation of move assignment that queries for has_move_assignment and otherwise copies.
 */
template <typename Cf, typename Key, typename Derived = void>
class base_term: public detail::base_term_tag
{
	private:
		static_assert(!std::is_pointer<Cf>::value && !std::is_pointer<Key>::value,"Cf and Key must not be pointer types.");
		static_assert(!is_cv_or_ref<Cf>::value && !is_cv_or_ref<Key>::value,"Cf and Key must not be reference types or cv-qualified.");
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
		/// Default move constructor.
		/**
		 * @throws unspecified any exception thrown by the move constructors of \p Cf and \p Key.
		 */
		base_term(base_term &&) = default;
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
		/// Copy constructor from different piranha::base_term.
		/**
		 * Will use <tt>other</tt>'s coefficient and key to copy-construct base_term::m_cf and base_term::m_key.
		 * 
		 * @param[in] other piranha::base_term used for copy construction.
		 * 
		 * @throws unspecified any exception thrown by the constructors of \p Cf and \p Key.
		 */
		template <typename Cf2, typename Key2, typename Derived2>
		explicit base_term(const base_term<Cf2,Key2,Derived2> &other):m_cf(other.m_cf),m_key(other.m_key) {}
		/// Move constructor from different piranha::base_term.
		/**
		 * Will use <tt>other<tt>'s coefficient and key to move-construct base_term::m_cf and base_term::m_key.
		 * 
		 * @param[in] other piranha::base_term used for move construction.
		 * 
		 * @throws unspecified any exception thrown by the constructors of \p Cf and \p Key.
		 */
		template <typename Cf2, typename Key2, typename Derived2>
		explicit base_term(base_term<Cf2,Key2,Derived2> &&other):m_cf(std::move(other.m_cf)),m_key(std::move(other.m_key)) {}
		/// Default destructor.
		/**
		 * @throws unspecified any exception thrown by the destructors of \p Cf and \p Key.
		 */
		~base_term() = default;
		/// Default copy assignment operator.
		/**
		 * @throws unspecified any exception thrown by the copy assignment operators of \p Cf and \p Key.
		 */
		base_term &operator=(const base_term &) = default;
		/// Move-assignment operator.
		/**
		 * Will move <tt>other<tt>'s coefficient and key.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the move assignment operators of \p Cf and \p Key.
		 */
		base_term &operator=(base_term &&other)
		{
			m_cf = std::move(other.m_cf);
			m_key = std::move(other.m_key);
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
		 * This method can be re-implemented in derived terms.
		 * 
		 * @return hash value of \p m_key as calculated via a default-constructed instance of \p std::hash.
		 * 
		 * @throws unspecified any exception thrown by the specialisation of \p std::hash for \p Key.
		 */
		std::size_t hash() const
		{
			return std::hash<key_type>()(m_key);
		}
		/// Overload of stream operator for piranha::base_term.
		/**
		 * Will print to stream a human-readable representation of the term.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] t piranha::base_term that will be directed to \p os.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by directing to the stream the term's
		 * coefficient and/or key.
		 */
		friend inline std::ostream &operator<<(std::ostream &os, const base_term<Cf,Key,Derived> &t)
		{
			os << t.m_cf << " - " << t.m_key;
			return os;
		}
		/// Coefficient member.
		mutable Cf	m_cf;
		/// Key member.
		Key		m_key;
};

}

#endif
