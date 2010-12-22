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

#include <mutex>
#include <stdexcept>

#include "exceptions.hpp"
#include "thread_barrier.hpp"

namespace piranha
{

/// Constructor.
/**
 * Construct a barrier for \p count threads.
 * 
 * @param[in] count number of threads for which the barrier is configured.
 * 
 * @throws std::invalid_argument if <tt>count == 0</tt>.
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
thread_barrier::thread_barrier(unsigned count):m_threshold(count),m_count(count),m_generation(0)
{
	if (count == 0) {
		piranha_throw(std::invalid_argument,"count cannot be zero");
	}
}

/// Wait method.
/**
 * Block until \p count threads have called wait() on this. When the <tt>count</tt>-th thread calls
 * wait(), all waiting threads are unblocked, and the barrier is reset.
 * 
 * @return \p true for exactly one thread from each batch of waiting threads, \p false otherwise.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
bool thread_barrier::wait()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	unsigned gen = m_generation;
	if (--m_count == 0) {
		++m_generation;
		m_count = m_threshold;
		m_cond.notify_all();
		return true;
	}
	while (gen == m_generation) {
		m_cond.wait(lock);
	}
	return false;
}

}
