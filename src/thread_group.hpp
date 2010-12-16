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

#include <list>
#include <memory>
#include <mutex>
#include <thread>

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
	public:
		/// Default constructor.
		thread_group() = default;
		thread_group(const thread_group &) = delete;
		thread_group &operator=(const thread_group &) = delete;
		thread_group(thread_group &&) = delete;
		thread_group &operator=(thread_group &&) = delete;
		~thread_group();
		/// Add thread to the group.
		/**
		 * The thread is created, immediately started, and finally added to the group. The function
		 * launched by the thread is:
		 * \code
		 * f(params)
		 * \endcode
		 * 
		 * @param[in] f functor used as argument for thread creation.
		 * @param[in] params parameters to be passed to the functor upon invocation.
		 */
		template <typename Functor, typename... Args>
		void create_thread(Functor &&f, Args && ... params)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_threads.push_back(std::unique_ptr<std::thread>(new std::thread(std::forward<Functor>(f),std::forward<Args>(params)...)));
		}
		void join_all();
	private:
		std::list<std::unique_ptr<std::thread>>	m_threads;
		std::mutex				m_mutex;
};

}

#endif
