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

#ifndef PIRANHA_TASK_GROUP_HPP
#define PIRANHA_TASK_GROUP_HPP

#include <algorithm>
#include <exception>
#include <forward_list>
#include <future>
#include <iterator>
#include <utility>

#include "config.hpp"
#include "thread.hpp"

namespace piranha
{

/// Task group class.
/**
 * This class represents a group of tasks run asynchrously in separate threads. Tasks must consist
 * of a nullary function object returning \p void, and they are added to the group via the add_task() method.
 *
 * The wait_all() method can be used to wait for the completion of all tasks, and the get_all() to retrieve
 * exceptions thrown by tasks.
 *
 * Access to this class is thread-safe only in read mode.
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class task_group
{
		// Various typedefs.
		typedef std::promise<void> p_type;
		typedef std::future<void> f_type;
		typedef std::pair<p_type,f_type> pair_type;
		typedef std::forward_list<pair_type> container_type;
		// Task wrapper.
		template <typename Callable>
		struct task_wrapper
		{
			template <typename T>
			explicit task_wrapper(T &&c, pair_type &pair):m_c(std::forward<T>(c)),m_pair(pair)
			{
				// NOTE: this is noexcept.
				m_pair.second = m_pair.first.get_future();
			}
			void operator()()
			{
				try {
					m_c();
					m_pair.first.set_value();
				} catch (...) {
					m_pair.first.set_exception(std::current_exception());
				}
			}
			Callable	m_c;
			pair_type	&m_pair;
		};
	public:
		/// Defaulted default constructor.
		/**
		 * Will initialise an empty task group.
		 */
		task_group() = default;
		/// Destructor.
		/**
		 * Will invoke wait_all().
		 */
		~task_group()
		{
			wait_all();
		}
		/// Deleted copy constructor.
		task_group(const task_group &) = delete;
		/// Deleted move constructor.
		task_group(task_group &&) = delete;
		/// Deleted copy assignment operator.
		task_group &operator=(const task_group &) = delete;
		/// Deleted move assignment operator.
		task_group &operator=(task_group &&) = delete;
		/// Add task.
		/**
		 * \note
		 * This method is enabled if piranha::thread is constructible from <tt>Callable &&</tt>.
		 *
		 * The nullary function object \p c will be invoked in a separate thread of execution. Any exception
		 * thrown by \p c will be stored within the object and can be retrieved later by the get_all() method.
		 *
		 * This method offers the following exception safety guarantee: in case of errors, the object is left
		 * in the same state as it was before the call to add_task(). In face of exceptions, it's not specified
		 * if the function \p c has been invoked or not.
		 *
		 * @param[in] c function object to be invoked.
		 *
		 * @throws unspecified any exception thrown by threading primitives or by memory allocation errors
		 * in standard containers or when copying \p c into the thread object.
		 */
		template <typename Callable, typename = typename std::enable_if<std::is_constructible<thread,Callable &&>::value>::type>
		void add_task(Callable &&c)
		{
			// First let's try to append an empty pair. If an error happens here, no big deal.
			m_container.emplace_front(p_type{},f_type{});
			try {
				// Create the functor.
				task_wrapper<typename std::decay<Callable>::type> tw(std::forward<Callable>(c),m_container.front());
				// Now the future has been assigned to the back of m_container. Launch the thread.
				thread thr(std::move(tw));
				piranha_assert(thr.joinable());
				// Try detaching.
				// NOTE: it is not clear here what kind of guarantees we can offer, could be a good candidate for fatal error
				// logging, or maybe terminate().
				try {
					thr.detach();
				} catch (...) {
					// Last-ditch effort: try join()ing before re-throwing.
					thr.join();
					throw;
				}
			} catch (...) {
				// The error happened *after* the empty item was added,
				// we have to remove it as it is invalid.
				m_container.pop_front();
				throw;
			}
		}
		/// Wait for completion of all tasks.
		/**
		 * It is safe to call this method multiple times, even if get_all() has been called before.
		 */
		void wait_all()
		{
			std::for_each(m_container.begin(),m_container.end(),[this](pair_type &p) {
				if (p.second.valid()) {
					p.second.wait();
				}
			});
		}
		/// Get an exception thrown by a task.
		/**
		 * Depending on the threading model (C++ vs Boost.Thread), calling this method multiple times
		 * can either re-throw the same exception or be a no-op.
		 * 
		 * @throws unspecified an exception thrown by a task.
		 */
		void get_all()
		{
			std::for_each(m_container.begin(),m_container.end(),[this](pair_type &p) {
				if (p.second.valid()) {
					p.second.get();
				}
			});
		}
	private:
		container_type m_container;
};

}

#endif
