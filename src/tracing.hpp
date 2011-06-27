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

#ifndef PIRANHA_TRACING_HPP
#define PIRANHA_TRACING_HPP

#include <boost/any.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include "config.hpp"
#include "settings.hpp"
#include "threading.hpp"

namespace piranha
{

/// Tracing class.
/**
 * This class is used to trace events for analysis and statistical purposes.
 * It is comprised of static methods that allow to interact at runtime with a database
 * of events in which string descriptors are paired with arbitrary data.
 * 
 * All methods in this class are thread-safe and provide the strong
 * exception safety guarantee, unless otherwise specified. Methods
 * and data members of this class are static, hence this class provides trivial
 * move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class tracing
{
		typedef std::unordered_map<std::string,boost::any> container_type;
	public:
		/// Trace event.
		/**
		 * Trace an event uniquely identified by \p str, and apply the functor
		 * \p f to the data associated to it.
		 * 
		 * \p Functor is expected to be an unary function object taking
		 * as parameter the reference to a \p boost::any instance representing
		 * the data associated to the descriptor \p str. The first time
		 * a descriptor is used, the \p boost::any object passed to \p f will be empty.
		 * 
		 * If tracing is disabled in piranha::settings, this method will be a no-op.
		 * 
		 * @param[in] str the descriptor of the event being traced.
		 * @param[in] f the functor to be applied to the data associated to \p str.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy construction of \p std::string,
		 * - memory errors in standard containers,
		 * - failures in threading primitives,
		 * - the execution of functor \p f.
		 * 
		 * @see www.boost.org/doc/libs/release/libs/any
		 */
		template <typename Functor>
		static void trace(const std::string &str, const Functor &f)
		{
			if (likely(!settings::get_tracing())) {
				return;
			}
			lock_guard<mutex>::type lock(m_mutex);
			bool new_item = false;
			auto it = m_container.find(str);
			if (it == m_container.end()) {
				auto tmp = m_container.insert(std::make_pair(str,boost::any()));
				piranha_assert(tmp.second);
				it = tmp.first;
				new_item = true;
			}
			try {
				f(it->second);
			} catch (...) {
				if (new_item) {
					m_container.erase(it);
				}
				throw;
			}
		}
		static void dump(std::ostream & = std::cout);
	private:
		static container_type	m_container;
		static mutex		m_mutex;
};

}

#endif
