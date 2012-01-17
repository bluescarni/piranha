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
#include <functional>
#include <memory>
#include <vector>

#include "config.hpp"
#include "threading.hpp"

namespace piranha
{

/// Task group class.
/**
 * This class represents a group of tasks run asynchrously in separate threads. Tasks must consist
 * of a nullary function returning \p void, and they are added to the group via the add_task() method.
 * 
 * The wait_all() method can be used to wait for the completion of all tasks, and the get_all() to retrieve
 * exceptions thrown by tasks.
 * 
 * Access to this class is thread-safe only in read mode.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo the current design is a compromise to work-around compiler bugs. In the next versions of this class,
 * the use of shared pointers will be dropped in favour of move semantics and the add_task() method will be generalised.
 */
class task_group
{
		typedef promise<void>::type p_type;
		typedef future<void>::type f_type;
		typedef std::vector<std::shared_ptr<f_type>> container_type;
	public:
		/// Size type.
		/**
		 * Unsigned integer type used to represent the number of tasks.
		 */
		typedef typename container_type::size_type size_type;
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
		 * The function \p f will be invoked in a separate thread of execution. Any exception
		 * thrown by \p f will be stored within the object and can be retrieved later by the get_all() method.
		 * 
		 * This method offers the following exception safety guarantee: in case of errors, the object is left
		 * in the same state as it was before the call to add_task(). In face of exceptions, it's not specified
		 * if the function \p f has been invoked or not.
		 * 
		 * @param[in] f nullary function to be invoked.
		 * 
		 * @throws unspecified any exception thrown by threading primitives or by memory allocation errors
		 * in standard containers.
		 */
		void add_task(std::function<void()> f)
		{
			const auto size = m_container.size();
			try {
				// First let's try to append an empty item.
				m_container.push_back(std::shared_ptr<f_type>{});
				// Create the promise and the future.
				auto p_ptr = std::make_shared<p_type>();
				auto f_ptr = std::make_shared<f_type>(p_ptr->get_future());
				auto functor = [f](std::shared_ptr<p_type> pp) {
					try {
						f();
						pp->set_value();
					} catch (...) {
						// NOTE: here piranha:: is necessary otherwise ADL becomes
						// ambiguous with boost/std::copy_exception.
						pp->set_exception(piranha::copy_exception(current_exception()));
					}
				};
				// Launch the thread.
				// NOTE: functor and p_ptr will be copied in the internal thread storage. Functor, in
				// turn, will be holding a copy of f.
				thread thr(functor,p_ptr);
				piranha_assert(thr.joinable());
				try {
					thr.detach();
				} catch (...) {
					// Last-ditch effort: try join()ing before re-throwing.
					thr.join();
					throw;
				}
				// If everything went ok, move in the real stuff.
				m_container.back().swap(f_ptr);
			} catch (...) {
				// If the error happened *after* the empty item was added,
				// then we have to remove it as it is invalid.
				if (m_container.size() != size) {
					m_container.pop_back();
				}
				throw;
			}
		}
		/// Wait for completion of all tasks.
		void wait_all()
		{
			std::for_each(m_container.begin(),m_container.end(),[](std::shared_ptr<f_type> &fp) {fp->wait();});
		}
		/// Get an exception thrown by a task.
		/**
		 * @throws unspecified an exception thrown by a task.
		 */
		void get_all()
		{
			std::for_each(m_container.begin(),m_container.end(),[](std::shared_ptr<f_type> &fp) {fp->get();});
		}
		/// Number of tasks.
		/**
		 * @return the number of tasks added via add_task().
		 */
		size_type size() const
		{
			return m_container.size();
		}
	private:
		container_type m_container;
};

}

#endif
