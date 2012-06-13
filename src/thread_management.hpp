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

#ifndef PIRANHA_THREAD_MANAGEMENT_HPP
#define PIRANHA_THREAD_MANAGEMENT_HPP

#include <unordered_set>
#include <utility>

#include "threading.hpp"

namespace piranha
{

/// Thread management.
/**
 * This class contains static methods to manage thread behaviour. The methods of this class, unless otherwise
 * specified, are thread-safe.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class PIRANHA_PUBLIC thread_management
{
	public:
		static void bind_to_proc(unsigned);
		static std::pair<bool,unsigned> bound_proc();
		/// Scoped binder.
		/**
		 * All instances of this class share a common list \p L of used processors, which, at program startup, is empty.
		 * Upon construction this class will attempt to bind the calling thread to the first processor \p n
		 * in the range [0,settings::get_n_threads()[ not in \p L, and, in case of success,
		 * will add \p n to \p L. Upon destruction, processor \p n will be removed from \p L.
		 *
		 * If the call to thread_management::bind_to_proc() generates an error, or all processors are already in use,
		 * or the calling thread is the main thread, then no action will be taken in the constructor or in the destructor.
		 * 
		 * \todo different binding strategies depending on the topology of the multicore architecture? E.g., avoid hyperthreading
		 * cores, distribute among different processores according to cache memory hierarchies, etc.
		 * \todo think of throwing on destruction if locking the mutex fails, probably we want to know something is wrong
		 * instead of failing silently. For the constructor no need, as long as in the destructor we check if thread
		 * was bound *before* locking.
		 */
		class PIRANHA_PUBLIC binder
		{
			public:
				binder();
				~binder();
				/// Deleted copy constructor.
				binder(const binder &) = delete;
				/// Deleted move constructor.
				binder(binder &&) = delete;
				/// Deleted copy assignment.
				binder &operator=(const binder &) = delete;
				/// Deleted move assignment.
				binder &operator=(binder &&) = delete;
			private:
				std::pair<bool,unsigned>		m_result;
				static mutex				m_binder_mutex;
				static std::unordered_set<unsigned>	m_used_procs;
		};
	private:
		static mutex m_mutex;
};

}

#endif