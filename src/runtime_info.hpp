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
		static unsigned hardware_concurrency();
		static unsigned get_cache_line_size();
	private:
		static const thread_id	m_main_thread_id;
		static mutex		m_mutex;
};

}

#endif
