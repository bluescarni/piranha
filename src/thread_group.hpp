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

#ifndef PIRANHA_THREAD_GROUP_HPP
#define PIRANHA_THREAD_GROUP_HPP

#include <boost/integer_traits.hpp>
#include <new>
#include <system_error>
#include <vector>

#include "exceptions.hpp"
#include "threading.hpp"

namespace piranha
{

/// Thread group.
/**
 * This class allows to create and manage groups of threads. All methods are thread-safe.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class thread_group
{
		typedef std::vector<thread> container_type;
		typedef container_type::size_type size_type;
	public:
		/// Default constructor.
		/**
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 */
		thread_group() = default;
		/// Deleted copy constructor.
		thread_group(const thread_group &) = delete;
		/// Deleted assignment operator.
		thread_group &operator=(const thread_group &) = delete;
		/// Deleted move constructor.
		thread_group(thread_group &&) = delete;
		/// Deleted move assignment operator.
		thread_group &operator=(thread_group &&) = delete;
		~thread_group();
		/// Add thread to the group.
		/**
		 * The thread is created, immediately started, and finally added to the group. The function
		 * launched by the thread is <tt>f(params)</tt>.
		 * If any exception is thrown during this operation, no thread will be added and the
		 * internal state will be identical to the state before the operation was attempted.
		 * 
		 * @param[in] f functor used as argument for thread creation.
		 * @param[in] params parameters to be passed to the functor upon invocation.
		 * 
		 * @throws std::bad_alloc if storage allocation for the new thread object fails.
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 * @throws unspecified any exception thrown by copying the functor or its arguments into the
		 * thread's internal storage.
		 */
		template <typename Functor, typename... Args>
		void create_thread(Functor &&f, Args && ... params)
		{
			lock_guard<mutex>::type lock(m_mutex);
			if (m_threads.size() == boost::integer_traits<size_type>::const_max) {
				throw std::bad_alloc();
			}
			// Make space for the new thread.
			const size_type new_size = m_threads.size() + static_cast<size_type>(1);
			m_threads.reserve(new_size);
			if (m_threads.capacity() < new_size) {
				throw std::bad_alloc();
			}
			m_threads.push_back(thread(std::forward<Functor>(f),std::forward<Args>(params)...));
		}
		void join_all();
	private:
		container_type	m_threads;
		mutex		m_mutex;
};

}

#endif
