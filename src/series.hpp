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

#ifndef PIRANHA_SERIES_HPP
#define PIRANHA_SERIES_HPP

#include <boost/concept/assert.hpp>
#include <cstddef>

#include "concepts/container_element.hpp"
#include "concepts/crtp.hpp"
#include "concepts/term.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "hash_set.hpp"
#include "series_multiplier.hpp"
#include "symbol_set.hpp"

namespace piranha
{

namespace detail
{

// Hash functor for term type in series.
template <typename Term>
struct term_hasher
{
	std::size_t operator()(const Term &term) const
	{
		return term.hash();
	}
};

}

template <typename Term, typename Derived>
class series
{
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		BOOST_CONCEPT_ASSERT((concept::CRTP<series<Term,Derived>,Derived>));
		// Container type for terms.
		typedef hash_set<Term,detail::term_hasher<Term>> container_type;
		// Make friend with debugging class.
		template <typename T>
		friend class debug_access;
		// Make friend with series multiplier class.
		template <typename Series1, typename Series2, typename Enable>
		friend class series_multiplier;
		// Make friend with other series types.
		template <typename Term2, typename Derived2>
		friend class series;
	public:
		/// Size type.
		/**
		 * Used to represent the number of terms in the series. Unsigned integer type equivalent to piranha::hash_set::size_type.
		 */
		typedef typename container_type::size_type size_type;
		/// Defaulted default constructor.
		/**
		 * Will construct a series with zero terms and no arguments.
		 */
		series() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::hash_set or piranha::symbol_set.
		 */
		series(const series &) = default;
		/// Defaulted move constructor.
		series(series &&) = default;
		/// Trivial destructor.
		~series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::ContainerElement<Derived>));
		}
		// TODO: fix this for exception safety.
		/// Defaulted copy-assignment operator.
		/**
		 * @throws unspecified any exception thrown by the copy assignment operator of piranha::hash_set or piranha::symbol_set.
		 * 
		 * @return reference to \p this.
		 */
		series &operator=(const series &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		series &operator=(series &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				m_sset = std::move(other.m_sset);
				m_container = std::move(other.m_container);
			}
			return *this;
		}
		/// Series size.
		/**
		 * @return the number of terms in the series.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Empty test.
		/**
		 * @return \p true if size() is nonzero, \p false otherwise.
		 */
		bool empty() const
		{
			return !size();
		}
	private:
		symbol_set	m_sset;
		container_type	m_container;
};

}

#endif
