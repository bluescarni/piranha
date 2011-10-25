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

#ifndef PIRANHA_SYMBOL_HPP
#define PIRANHA_SYMBOL_HPP

#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "config.hpp"
#include "threading.hpp"

namespace piranha
{

/// Literal symbol class.
/**
 * This class represents a symbolic variable, which is associated to a name and a numerical value. Symbol instances
 * are tracked in global list for the duration of the program, so that different instances of symbols with the same name are always
 * referring to the same underlying object.
 * 
 * The methods of this class, unless specified otherwise, are thread-safe: it is possible to create, access and operate on objects of this class
 * concurrently from multiple threads
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
 * 
 * \todo drop the value, does not make much sense anymore.
 */
class symbol
{
	public:
		/// Constructor from name.
		/**
		 * Construct a symbol with name \p name. If the name has been used before to create another symbol,
		 * use its numerical value for \p this. Otherwise, the numerical value will be 0.
		 * 
		 * @param[in] name name of the symbol.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::map::insert()</tt>.
		 */
		explicit symbol(const std::string &name):m_it(get_iterator<false>(name,0.)) {}
		/// Constructor from name and numerical value.
		/**
		 * Construct a symbol with name \p name. The numerical value associated to \p this will be
		 * \p value, regardless if a symbol with the same name was created before.
		 * 
		 * @param[in] name name of the symbol.
		 * @param[in] value numerical value of the symbol.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::map::insert()</tt>.
		 */
		explicit symbol(const std::string &name, const double &value):m_it(get_iterator<true>(name,value)) {}
		/// Defaulted copy constructor.
		symbol(const symbol &) = default;
		/// Move constructor.
		/**
		 * Equivalent to the copy constructor.
		 * 
		 * @param[in] other symbol to be copied.
		 */
		symbol(symbol &&other) piranha_noexcept_spec(true) : m_it(other.m_it) {}
		/// Defaulted copy assignment operator.
		symbol &operator=(const symbol &) = default;
		/// Move assignment operator.
		/**
		 * Equivalent to the copy assignment operator.
		 * 
		 * @param[in] other symbol to be copied.
		 * 
		 * @return reference to \p this.
		 */
		symbol &operator=(symbol &&other) piranha_noexcept_spec(true)
		{
			return operator=(other);
		}
		/// Defaulted destructor.
		~symbol() = default;
		/// Name getter.
		/**
		 * @return const reference to the name of the symbol. 
		 */
		const std::string &get_name() const
		{
			return m_it->first;
		}
		/// Value getter.
		/**
		 * @return numerical value of the symbol. 
		 */
		double get_value() const
		{
			lock_guard<mutex>::type lock(m_mutex);
			return m_it->second;
		}
		/// Equality operator.
		/**
		 * @param[in] other equality argument.
		 * 
		 * @return true if \p other refers to the same underlying object of \p this, false otherwise.
		 */
		bool operator==(const symbol &other) const
		{
			return m_it == other.m_it;
		}
		/// Inequality operator.
		/**
		 * @param[in] other inequality argument.
		 * 
		 * @return negation of operator==().
		 */
		bool operator!=(const symbol &other) const
		{
			return !(*this == other);
		}
		/// Less-than comparison.
		/**
		 * Will compare lexicographically the names of the two symbols.
		 * 
		 * @param[in] other comparison argument.
		 * 
		 * @return <tt>this->get_name() < other.get_name()</tt>.
		 */
		bool operator<(const symbol &other) const
		{
			return get_name() < other.get_name();
		}
		/// Overload output stream operator for piranha::symbol.
		/**
		 * Will direct to \p os a human-readable description of \p s.
		 * 
		 * @param[in,out] os output stream.
		 * @param[in] s piranha::symbol to be sent to stream.
		 * 
		 * @return reference to \p os.
		 */
		friend std::ostream &operator<<(std::ostream &os, const symbol &s)
		{
			os << "name = \"" << s.get_name() << "\", value = " << s.get_value();
			return os;
		}
	private:
		typedef std::map<std::string,double> container_type;
		template <bool Replace>
		static container_type::const_iterator get_iterator(const std::string &name, const double &value)
		{
			lock_guard<mutex>::type lock(m_mutex);
			auto it = m_symbol_list.find(name);
			if (it == m_symbol_list.end()) {
				// If symbol is not present in the list, add it.
				auto retval = m_symbol_list.insert(std::make_pair(name,value));
				piranha_assert(retval.second);
				it = retval.first;
			} else if (Replace) {
				// If the symbol already exists and a replace was requested, perform it.
				it->second = value;
			}
			// Final check, just for peace of mind.
			piranha_assert(it != m_symbol_list.end());
			return it;
		}
		container_type::const_iterator	m_it;
		static mutex			m_mutex;
		static container_type		m_symbol_list;
};

}

#endif
