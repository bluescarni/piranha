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

#ifndef PIRANHA_RUNTIME_INFO_HPP
#define PIRANHA_RUNTIME_INFO_HPP

#include "threading.hpp"

namespace piranha
{

/// Runtime information.
/**
 * This class allows to query information about the runtime environment.
 * All methods are thread-safe, unless otherwise specified.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo think about adding a way to set the cache line size at runtime, in case we cannot detect it.
 */
class runtime_info
{
	public:
		/// Main thread ID.
		/**
		 * @return const reference to an instance of the ID of the main thread of execution.
		 */
		static const thread_id &get_main_thread_id()
		{
			return m_main_thread_id;
		}
		/** @name Getters
		 * The values returned by these methods are static constants determined at startup
		 * using the <tt>determine_</tt> methods. Their purpose is to provide fast, thread-safe
		 * information on the runtime environment.
		 */
		//@{
		static unsigned get_hardware_concurrency();
		static unsigned get_cache_line_size();
		//@}
		/** @name Runtime query primitives
		 * These methods explicitly query the runtime environment using low-level platform-specific
		 * primitives. These methods are not thread-safe.
		 */
		//@{
		static unsigned determine_cache_line_size();
		static unsigned determine_hardware_concurrency();
		//@}
	private:
		static const thread_id	m_main_thread_id;
		static const unsigned	m_hardware_concurrency;
		static const unsigned	m_cache_line_size;
};

}

#endif
