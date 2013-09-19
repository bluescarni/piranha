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
#include <forward_list>
#include <future>

#include "config.hpp"
#include "detail/mpfr.hpp"
#include "type_traits.hpp"

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
		typedef std::future<void> f_type;
		typedef std::forward_list<f_type> container_type;
		// Simple wrapper that will call the callable and afterwards will
		// clear up the MPFR caches.
		template <typename Callable>
		struct wrapper
		{
			wrapper() = delete;
			template <typename T>
			explicit wrapper(T &&c):m_c(std::forward<T>(c)) {}
			wrapper(const wrapper &) = delete;
			wrapper(wrapper &&) = default;
			wrapper &operator=(const wrapper &) = delete;
			wrapper &operator=(wrapper &&) = delete;
			void operator()()
			{
				try {
					m_c();
				} catch (...) {
					::mpfr_free_cache();
					throw;
				}
				::mpfr_free_cache();
			}
			Callable m_c;
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
		 * This method is enabled if the decay type of \p Callable is a function object returning \p void, move-constructible
		 * and constructible from <tt>Callable &&</tt>.
		 *
		 * The nullary function object \p c will be invoked in a separate thread of execution. Any exception
		 * thrown by \p c will be stored and can be retrieved later by the get_all() method.
		 * 
		 * This method will encapsulate \p c in a wrapper that will take care of calling the MPFR function <tt>mpfr_free_cache()</tt>
		 * from within the thread after the execution of \p c has completed (even in case of exceptions). This means that it is safe
		 * to use the MPFR API within \p c (provided that a thread-safe MPFR version is being used).
		 *
		 * This method offers the following exception safety guarantee: in case of errors, the object is left
		 * in the same state as it was before the call to add_task(). In face of exceptions, it is not specified
		 * if \p c has been invoked or not.
		 *
		 * @param[in] c function object to be invoked.
		 *
		 * @throws unspecified any exception thrown by:
		 * - threading primitives,
		 * - memory allocation errors in standard containers,
		 * - the move/copy constructor of \p c.
		 * 
		 * @see http://www.mpfr.org/mpfr-current/mpfr.html#Memory-Handling
		 */
		template <typename Callable, typename = typename std::enable_if<is_function_object<
			typename std::decay<Callable>::type,void>::value &&
			std::is_move_constructible<typename std::decay<Callable>::type>::value &&
			std::is_constructible<typename std::decay<Callable>::type,Callable &&>::value>::type>
		void add_task(Callable &&c)
		{
			// First let's try to append an empty future. If an error happens here, no big deal.
			m_container.emplace_front();
			try {
				// Create the functor.
				wrapper<typename std::decay<Callable>::type> tw(std::forward<Callable>(c));
				// Assign the future to the back of m_container and launch the thread.
				m_container.front() = std::async(std::launch::async,std::move(tw));
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
			std::for_each(m_container.begin(),m_container.end(),[](f_type &f) {
				if (f.valid()) {
					f.wait();
				}
			});
		}
		/// Get an exception thrown by a task.
		/**
		 * @throws unspecified an exception thrown by a task.
		 */
		void get_all()
		{
			std::for_each(m_container.begin(),m_container.end(),[](f_type &f) {
				if (f.valid()) {
					f.get();
				}
			});
		}
	private:
		container_type m_container;
};

}

#endif
