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

#ifndef PIRANHA_SYMBOL_SET_HPP
#define PIRANHA_SYMBOL_SET_HPP

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "symbol.hpp"

namespace piranha
{

/// Symbol set.
/**
 * This class represents an ordered set of piranha::symbol. The individual piranha::symbol instances
 * can be accessed via iterators or the index operator.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object in a state equivalent to a
 * default-constructed object.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class symbol_set
{
		bool check() const
		{
			// Check for sorted range.
			if (!std::is_sorted(begin(),end())) {
				return false;
			}
			if (size() < 2u) {
				return true;
			}
			// Check for duplicates.
			for (size_type i = 0u; i < size() - 1u; ++i) {
				if (m_values[i] == m_values[i + 1u]) {
					return false;
				}
			}
			return true;
		}
	public:
		/// Size type.
		typedef std::vector<symbol>::size_type size_type;
		/// Const iterator.
		typedef std::vector<symbol>::const_iterator const_iterator;
		/// Defaulted default constructor.
		/**
		 * Will construct an empty set.
		 */
		symbol_set() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector.
		 */
		symbol_set(const symbol_set &) = default;
		/// Defaulted move constructor.
		symbol_set(symbol_set &&) = default;
		/// Defaulted copy assignment operator.
		/**
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector.
		 */
		symbol_set &operator=(const symbol_set &) = default;
		/// Move operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		symbol_set &operator=(symbol_set &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				m_values = std::move(other.m_values);
			}
			return *this;
		}
		/// Trivial destructor.
		~symbol_set() piranha_noexcept_spec(true)
		{
			piranha_assert(check());
		}
		/// Index operator.
		/**
		 * @param[in] n index of the element to be accessed.
		 * 
		 * @return const reference to the element at index \p n.
		 */
		const symbol &operator[](const size_type &n) const
		{
			piranha_assert(n < size());
			return m_values[n];
		}
		/// Begin const iterator.
		/**
		 * @return const iterator to the first element of the set, or end() if the set is empty.
		 */
		const_iterator begin() const
		{
			return m_values.begin();
		}
		/// End const iterator.
		/**
		 * @return const iterator to the element past the last element of the set.
		 */
		const_iterator end() const
		{
			return m_values.end();
		}
		/// Add symbol to the set.
		/**
		 * The insertion of \p s will preserve the order of the set.
		 * 
		 * @param[in] s piranha::symbol to be inserted.
		 * 
		 * @throws std::invalid_argument if \p s is already present in the set.
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector.
		 */
		void add(const symbol &s)
		{
			// Copy it to provide strong exception safety.
			std::vector<symbol> new_values;
			new_values.reserve(size() + size_type(1u));
			std::copy(begin(),end(),std::back_insert_iterator<std::vector<symbol>>(new_values));
			const auto it = std::lower_bound(new_values.begin(),new_values.end(),s);
			if (unlikely(it != new_values.end() && *it == s)) {
				piranha_throw(std::invalid_argument,"symbol already present in this set");
			}
			new_values.insert(it,s);
			// Move in the new args vector.
			m_values = std::move(new_values);
			piranha_assert(check());
		}
		/// Set size.
		/**
		 * @return the number of elements in the set.
		 */
		size_type size() const
		{
			return m_values.size();
		}
		/// Merge with other set.
		/**
		 * @param[in] other merge argument.
		 * 
		 * @return a new set containing the union of the elements present in \p this and \p other.
		 * 
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector.
		 */
		symbol_set merge(const symbol_set &other) const
		{
			symbol_set retval;
			retval.m_values.reserve(other.size() + size());
			auto bi_it = std::back_insert_iterator<std::vector<symbol>>(retval.m_values);
			std::set_union(begin(),end(),other.begin(),other.end(),bi_it);
			piranha_assert(retval.check());
			return retval;
		}
		/// Equality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return \p true if \p this and \p other contain exactly the same symbols, \p false otherwise.
		 */
		bool operator==(const symbol_set &other) const
		{
			const size_type s = size();
			if (s != other.size()) {
				return false;
			}
			for (size_type i = 0u; i < s; ++i) {
				if (m_values[i] != other[i]) {
					return false;
				}
			}
			return true;
		}
		/// Inequality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return opposite of operator==().
		 */
		bool operator!=(const symbol_set &other) const
		{
			return !(*this == other);
		}
	private:
		std::vector<symbol> m_values;
};

}

#endif
