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

#include "threading.hpp"

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
		thread_barrier(unsigned);
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
		 * No threads must be waiting on this when the destructor is called.
		 */
		~thread_barrier() = default;
		bool wait();
	private:
		mutex			m_mutex;
		condition_variable	m_cond;
		const unsigned		m_threshold;
		unsigned		m_count;
		unsigned		m_generation;
};

}

#endif
