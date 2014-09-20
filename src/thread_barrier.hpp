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

#ifndef PIRANHA_THREAD_BARRIER_HPP
#define PIRANHA_THREAD_BARRIER_HPP

#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <stdexcept>

#include "exceptions.hpp"
#include "thread_barrier.hpp"

namespace piranha
{

// This class has been minimally adapted from the barrier class in the Boost libraries 1.45.0.
// Original copyright notice follows:

// Copyright (C) 2002-2003
// David Moore, William E. Kempf
// Copyright (C) 2007-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// Thread barrier.
/**
 * Also known as a rendezvous, a barrier is a synchronization point between multiple threads.
 * The barrier is configured for a particular number of threads \p n, and as threads reach the barrier
 * they must wait until all \p n threads have arrived. Once the <tt>n</tt>-th thread has reached the barrier,
 * all the waiting threads can proceed, and the barrier is reset.
 * 
 * This class has been minimally adapted from the barrier class available from the Boost libraries.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class thread_barrier
{
	public:
		/// Constructor.
		/**
		 * Construct a barrier for \p count threads.
		 *
		 * @param[in] count number of threads for which the barrier is configured.
		 *
		 * @throws std::invalid_argument if <tt>count == 0</tt>.
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 */
		explicit thread_barrier(unsigned count):m_mutex(),m_cond(),m_threshold(count),m_count(count),m_generation(0)
		{
			if (count == 0) {
				piranha_throw(std::invalid_argument,"count cannot be zero");
			}
		}
		/// Deleted copy constructor.
		thread_barrier(const thread_barrier &) = delete;
		/// Deleted move constructor.
		thread_barrier(thread_barrier &&) = delete;
		/// Deleted assignment operator.
		thread_barrier &operator=(const thread_barrier &) = delete;
		/// Deleted move assignment operator.
		thread_barrier &operator=(thread_barrier &&) = delete;
		/// Default destructor.
		/**
		 * No threads must be waiting on this when the destructor is called, otherwise the program will abort.
		 */
		~thread_barrier()
		{
			try {
				std::unique_lock<std::mutex> lock(m_mutex);
				if (m_count != m_threshold) {
					// NOTE: logging candidate.
					std::abort();
				}
			} catch (...) {
				// NOTE: logging candidate.
				std::abort();
			}
		}
		/// Wait method.
		/**
		 * Block until \p count threads have called wait() on this. When the <tt>count</tt>-th thread calls
		 * wait(), all waiting threads are unblocked, and the barrier is reset.
		 *
		 * @return \p true for exactly one thread from each batch of waiting threads, \p false otherwise.
		 */
		bool wait()
		{
			try {
				std::unique_lock<std::mutex> lock(m_mutex);
				unsigned gen = m_generation;
				if (--m_count == 0) {
					// This is the last thread, update generation count
					// and reset the count to the initial value, then
					// notify the other threads.
					++m_generation;
					m_count = m_threshold;
					// NOTE: this is noexcept.
					m_cond.notify_all();
					return true;
				}
				// This is not the last thread, wait for the other threads
				// to clear the barrier.
				while (gen == m_generation) {
					// NOTE: this will be noexcept in C++14.
					m_cond.wait(lock);
				}
				return false;
			} catch (...) {
				// NOTE: logging candidate.
				std::abort();
			}
		}
	private:
		std::mutex		m_mutex;
		std::condition_variable	m_cond;
		const unsigned		m_threshold;
		unsigned		m_count;
		unsigned		m_generation;
};

}

#endif
