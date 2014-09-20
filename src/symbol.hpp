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

#include <cstddef>
#include <functional>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include "config.hpp"

namespace piranha
{

namespace detail
{

template <typename = int>
struct base_symbol
{
	static std::mutex		m_mutex;
	static std::set<std::string>	m_symbol_list;
};

template <typename T>
std::mutex base_symbol<T>::m_mutex;

template <typename T>
std::set<std::string> base_symbol<T>::m_symbol_list;

}

/// Literal symbol class.
/**
 * This class represents a symbolic variable uniquely identified by its name. Symbol instances
 * are tracked in a global list for the duration of the program, so that different instances of symbols with the same name are always
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
 * Move construction and move assignment will not alter the original object.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class symbol: private detail::base_symbol<>
{
	public:
		/// Constructor from name.
		/**
		 * Construct a symbol with name \p name.
		 * 
		 * @param[in] name name of the symbol.
		 * 
		 * @throws unspecified any exception thrown by <tt>std::set::insert()</tt>.
		 */
		explicit symbol(const std::string &name):m_ptr(get_pointer(name)) {}
		/// Defaulted copy constructor.
		symbol(const symbol &) = default;
		/// Defaulted move constructor.
		symbol(symbol &&) = default;
		/// Defaulted copy assignment operator.
		symbol &operator=(const symbol &) = default;
		/// Defaulted move assignment operator.
		symbol &operator=(symbol &&) = default;
		/// Defaulted destructor.
		~symbol() = default;
		/// Name getter.
		/**
		 * @return const reference to the name of the symbol. 
		 */
		const std::string &get_name() const
		{
			return *m_ptr;
		}
		/// Equality operator.
		/**
		 * @param[in] other equality argument.
		 * 
		 * @return true if \p other refers to the same underlying object as \p this, false otherwise.
		 */
		bool operator==(const symbol &other) const
		{
			return m_ptr == other.m_ptr;
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
		/// Hash value.
		/**
		 * @return a hash value for the symbol.
		 */
		std::size_t hash() const
		{
			return std::hash<std::string const *>()(m_ptr);
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
			os << "name = \'" << s.get_name() << "\'";
			return os;
		}
	private:
		static std::string const *get_pointer(const std::string &name)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_symbol_list.find(name);
			if (it == m_symbol_list.end()) {
				// If symbol is not present in the list, add it.
				auto retval = m_symbol_list.insert(name);
				piranha_assert(retval.second);
				it = retval.first;
			}
			// Final check, just for peace of mind.
			piranha_assert(it != m_symbol_list.end());
			// Extract and return the pointer.
			// NOTE: this is legal, as std::set is a standard container:
			// http://www.gotw.ca/gotw/050.htm
			return &*it;
		}
		std::string const		*m_ptr;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::symbol.
template <>
struct hash<piranha::symbol>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::symbol argument_type;
	/// Hash operator.
	/**
	 * @param[in] s piranha::symbol whose hash value will be returned.
	 * 
	 * @return piranha::symbol::hash().
	 */
	result_type operator()(const argument_type &s) const noexcept
	{
		return s.hash();
	}
};

}

#endif
