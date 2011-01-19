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
#include <mutex>
#include <string>
#include <utility>

#include "config.hpp"

namespace piranha
{

/// Literal symbol class.
/**
 * This class represents a symbolic variable, which is associated to a name and a numerical value. Symbol instances
 * are tracked in global list for the duration of the program, so that different instances of symbols with the same name are always
 * referring to the same underlying object.
 * 
 * The methods of this class, unless specified otherwise, are thread-safe.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
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
		 */
		explicit symbol(const std::string &name):m_it(get_iterator<false>(name,0.)) {}
		/// Constructor from name and numerical value.
		/**
		 * Construct a symbol with name \p name. The numerical value associated to \p this will be
		 * \p value, regardless if a symbol with the same name was created before.
		 * 
		 * @param[in] name name of the symbol.
		 * @param[in] value numerical value of the symbol.
		 */
		explicit symbol(const std::string &name, const double &value):m_it(get_iterator<true>(name,value)) {}
		/// Default copy constructor.
		symbol(const symbol &) = default;
		/// Deleted move constructor.
		symbol(symbol &&) = delete;
		/// Deleted copy assignment operator.
		symbol &operator=(const symbol &) = delete;
		/// Deleted move assignment operator.
		symbol &operator=(symbol &&) = delete;
		/// Default destructor.
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
		 * This method is thread-safe as long as the numerical value of \p this is not being modified
		 * concurrently (e.g., by constructing a symbol with the same name in a different thread).
		 * 
		 * @return numerical value of the symbol. 
		 */
		double get_value() const
		{
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
		/// Overload output stream operator for piranha::symbol.
		/**
		 * Will send to \p os a human-readable description of \p s.
		 * 
		 * @param[in,out] os output stream.
		 * @param[in] s piranha::symbol to be sent to stream.
		 * 
		 * @return reference to \p os.
		 */
		friend inline std::ostream &operator<<(std::ostream &os, const symbol &s)
		{
			os << "name = \"" << s.get_name() << "\", value = " << s.get_value();
			return os;
		}
	private:
		typedef std::map<std::string,double> container_type;
		template <bool Replace>
		static container_type::const_iterator get_iterator(const std::string &name, const double &value)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
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
		const container_type::const_iterator	m_it;
		static std::mutex			m_mutex;
		static container_type			m_symbol_list;
};

}

#endif
