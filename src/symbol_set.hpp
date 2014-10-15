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
#include <initializer_list>
#include <iterator>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/symbol_set_fwd.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "symbol.hpp"
#include "type_traits.hpp"

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
		// Enabler for ctor from iterator.
		template <typename Iterator, typename Symbol>
		using it_ctor_enabler = typename std::enable_if<is_input_iterator<Iterator>::value &&
			std::is_constructible<Symbol,decltype(*(std::declval<const Iterator &>()))>::value,int>::type;
	public:
		/// Size type.
		typedef std::vector<symbol>::size_type size_type;
		/// Positions class.
		/**
		 * This is a small utility class that can be used to determine the positions,
		 * in a piranha::symbol_set \p a, of the symbols in the piranha::symbol_set \p b.
		 * The positions are stored internally in an \p std::vector which is guaranteed
		 * to be sorted in ascending order and which can be accessed via the iterators
		 * provided by this class. If a symbol which is present in \p b is not present in
		 * \p a, then it will be ignored.
		 *
		 * For instance, if the set \p a contains the symbols <tt>[B,C,D,E]</tt> and \p b
		 * contains <tt>[A,B,D,F]</tt>, then an instance of this class constructed passing
		 * \p a and \p b as parameters will contain the vector <tt>[0,2]</tt>.
		 */
		class positions
		{
			public:
				/// Const iterator.
				using const_iterator = std::vector<size_type>::const_iterator;
				/// Value type.
				/**
				 * The positions are represented using the size type of piranha::symbol_set.
				 */
				using value_type = size_type;
				explicit positions(const symbol_set &, const symbol_set &);
				/// Deleted copy constructor.
				positions(const positions &) = delete;
				/// Defaulted move constructor.
				positions(positions &&) = default;
				/// Deleted copy assignment operator.
				positions &operator=(const positions &) = delete;
				/// Deleted move assignment operator.
				positions &operator=(positions &&) = delete;
				/// Begin iterator.
				/**
				 * @return an iterator to the begin of the internal vector.
				 */
				const_iterator begin() const
				{
					return m_values.begin();
				}
				/// End iterator.
				/**
				 * @return an iterator to the end of the internal vector.
				 */
				const_iterator end() const
				{
					return m_values.end();
				}
				/// Last element.
				/**
				 * @return a const reference to the last element.
				 */
				const size_type &back() const
				{
					piranha_assert(m_values.size());
					return m_values.back();
				}
				/// Size.
				/**
				 * @return the size of the internal vector.
				 */
				std::vector<size_type>::size_type size() const
				{
					return m_values.size();
				}
			private:
				std::vector<size_type> m_values;
		};
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
		/// Constructor from initializer list of piranha::symbol.
		/**
		 * Each symbol in the list will be added via add() to the set.
		 * 
		 * @param[in] l list of symbols used for construction.
		 * 
		 * @throws unspecified any exception thrown by add().
		 */
		explicit symbol_set(std::initializer_list<symbol> l)
		{
			// NOTE: for these types of initialisations from other containers
			// it might make sense eventually to avoid all these adds, and do
			// the sorting and elimination of duplicates in one pass to lower
			// algorithmic complexity.
			for (const auto &s: l) {
				add(s);
			}
		}
		/// Constructor from range.
		/**
		 * \note
		 * This constructor is enabled only if \p Iterator is an input iterator and
		 * piranha::symbol is constructible from its value type.
		 *
		 * The set will be initialised with symbols constructed from the elements of the range.
		 *
		 * @param[in] begin begin iterator.
		 * @param[in] end end iterator.
		 *
		 * @throws unspecified any exception thrown by operations on standard containers or by
		 * the invoked constructor of piranha::symbol.
		 */
		// NOTE: the templated Symbol here is apparently necessary to prevent an ICE in the intel
		// compiler. It should go eventually.
		template <typename Iterator, typename Symbol = symbol, it_ctor_enabler<Iterator,Symbol> = 0>
		explicit symbol_set(const Iterator &begin, const Iterator &end)
		{
			// NOTE: this is one possible way of doing this, probably a sorted vector
			// with std::unique will be faster. Also, this can be shared with the ctor
			// from init list which can be furtherly generalised.
			std::set<symbol> s_set;
			for (Iterator it = begin; it != end; ++it) {
				s_set.emplace(*it);
			}
			std::copy(s_set.begin(),s_set.end(),std::back_inserter(m_values));
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other set to be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector.
		 */
		symbol_set &operator=(const symbol_set &other)
		{
			if (likely(this != &other)) {
				symbol_set tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		symbol_set &operator=(symbol_set &&other) noexcept
		{
			// NOTE: here the idea is that in principle we want to be able to move-assign self,
			// and we don't want to rely on std::vector to support this. Hence, the explicit check.
			if (likely(this != &other)) {
				m_values = std::move(other.m_values);
			}
			return *this;
		}
		/// Trivial destructor.
		~symbol_set()
		{
			// NOTE: here we should replace with bidirectional tt, if we ever implement it.
			PIRANHA_TT_CHECK(is_forward_iterator,const_iterator);
			piranha_assert(run_destruction_checks());
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
			std::copy(begin(),end(),std::back_inserter(new_values));
			const auto it = std::lower_bound(new_values.begin(),new_values.end(),s);
			if (unlikely(it != new_values.end() && *it == s)) {
				piranha_throw(std::invalid_argument,"symbol already present in this set");
			}
			new_values.insert(it,s);
			// Move in the new args vector.
			m_values = std::move(new_values);
			piranha_assert(check());
		}
		/// Add symbol to the set.
		/**
		 * Equivalent to constructing a piranha::symbol from \p name and then invoking the other overload of this method.
		 * 
		 * @param[in] name name of the piranha::symbol to be inserted.
		 * 
		 * @throws unspecified any exception thrown by the other overload of this method or by the construction
		 * of piranha::symbol from \p std::string.
		 */
		void add(const std::string &name)
		{
			add(symbol(name));
		}
		/// Remove symbol from the set.
		/**
		 * The removal of \p s will preserve the order of the set.
		 * 
		 * @param[in] s piranha::symbol to be removed.
		 * 
		 * @throws std::invalid_argument if \p s is not present in the set.
		 * @throws unspecified any exception thrown by memory allocation errors in \p std::vector.
		 */
		void remove(const symbol &s)
		{
			// Operate on a copy to provide strong exception safety.
			std::vector<symbol> new_values;
			std::remove_copy_if(begin(),end(),std::back_inserter(new_values),
				[&s](const symbol &sym) {return sym == s;});
			if (new_values.size() == size()) {
				piranha_throw(std::invalid_argument,"symbol is not present in this set");
			}
			m_values = std::move(new_values);
			piranha_assert(check());
		}
		/// Remove symbol from the set.
		/**
		 * Equivalent to constructing a piranha::symbol from \p name and then invoking the other overload of this method.
		 * 
		 * @param[in] name name of the piranha::symbol to be removed.
		 * 
		 * @throws unspecified any exception thrown by the other overload of this method or by the construction
		 * of piranha::symbol from \p std::string.
		 */
		void remove(const std::string &name)
		{
			remove(symbol(name));
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
		 * @throws unspecified any exception thrown by \p std::vector::push_back().
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
		/// Set difference.
		/**
		 * @param[in] other difference argument.
		 * 
		 * @return a new set containing the elements of \p this which are not present in \p other.
		 * 
		 * @throws unspecified any exception thrown by \p std::vector::push_back().
		 */
		symbol_set diff(const symbol_set &other) const
		{
			symbol_set retval;
			std::set_difference(begin(),end(),other.begin(),
				other.end(),std::back_inserter(retval.m_values));
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
			return m_values == other.m_values;
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
		bool run_destruction_checks() const
		{
			// Run destruction checks only if we are not in the shutdown phase.
			if (environment::shutdown()) {
				return true;
			}
			return check();
		}
	private:
		std::vector<symbol> m_values;
};

/// Constructor from sets.
/**
 * The internal positions vector will contain the positions in \p a of the elements
 * of \p b appearing in the set \p a.
 *
 * @param[in] a first set.
 * @param[in] b second set.
 *
 * @throws unspecified any exception thrown by memory errors in standard containers.
 */
inline symbol_set::positions::positions(const symbol_set &a, const symbol_set &b)
{
	size_type ia = 0u, ib = 0u;
	while (true) {
		if (ia == a.size() || ib == b.size()) {
			break;
		}
		if (a[ia] == b[ib]) {
			m_values.push_back(ia);
			++ia;
			++ib;
		} else if (a[ia] < b[ib]) {
			++ia;
		} else {
			++ib;
		}
	}
}

}

#endif
