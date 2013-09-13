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

#ifndef PIRANHA_THREAD_POOL_HPP
#define PIRANHA_THREAD_POOL_HPP

#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#include "detail/mpfr.hpp"
#include "exceptions.hpp"

namespace piranha
{

namespace detail
{

class task_queue
{
		struct runner
		{
			runner(task_queue *ptr):m_ptr(ptr) {}
			void operator()() const
			{
				try {
					while (true) {
						std::unique_lock<std::mutex> lock(m_ptr->m_mutex);
						while (!m_ptr->m_stop && m_ptr->m_tasks.empty()) {
							// Need to wait for something to happen only if the task
							// list is empty and we are not stopping.
							m_ptr->m_cond.wait(lock);
						}
						if (m_ptr->m_stop && m_ptr->m_tasks.empty()) {
							// If the stop flag was set, and we do not have more tasks,
							// just exit.
							break;
						}
						// NOTE: move constructor of std::function could throw, unfortunately.
						std::function<void()> task(std::move(m_ptr->m_tasks.front()));
						m_ptr->m_tasks.pop();
						lock.unlock();
						task();
					}
				} catch (...) {
					// The errors we could get here are:
					// - threading primitives,
					// - move-construction of std::function,
					// - queue popping (I guess unlikely, as the destructor of std::function
					//   is noexcept).
					// In any case, not much that can be done to recover from this, better to abort.
					// NOTE: logging candidate.
					std::abort();
				}
				// Free the MPFR caches.
				::mpfr_free_cache();
			}
			task_queue *m_ptr;
		};
	public:
		task_queue():m_stop(false)
		{
			m_thread.reset(new std::thread(runner{this}));
		}
		~task_queue() noexcept
		{
			// NOTE: logging candidate (catch any exception,
			// log it and abort as there is not much we can do).
			try {
				stop();
			} catch (...) {
				std::abort();
			}
		}
		template <typename F, typename ... Args>
		auto enqueue(F &&f, Args && ... args) -> std::future<decltype(f(args...))>
		{
			using f_ret_type = decltype(f(args...));
			using p_task_type = std::packaged_task<f_ret_type()>;
			auto task = std::make_shared<p_task_type>(std::bind(std::forward<F>(f),std::forward<Args>(args)...));
			std::future<f_ret_type> res = task->get_future();
			{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_stop) {
				// Enqueueing is not allowed if the queue is stopped.
				piranha_throw(std::runtime_error,"cannot enqueue task while task queue is stopping");
			}
			m_tasks.push([task](){(*task)();});
			}
			// NOTE: if an exception is thrown here, the thread might not get notified.
			try {
				m_cond.notify_one();
			} catch (...) {
				// NOTE: logging candidate.
				std::abort();
			}
			return res;
		}
		// NOTE: we call this only from dtor, it is here in order to be able to test it.
		// So the exception handling in dtor will suffice, keep it in mind if things change.
		void stop()
		{
			{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_stop) {
				// Already stopped.
				return;
			}
			m_stop = true;
			}
			// Notify the thread that queue has been stopped, wait for it
			// to consume the remaining tasks and exit.
			m_cond.notify_one();
			m_thread->join();
		}
	private:
		bool					m_stop;
		std::condition_variable			m_cond;
		std::mutex				m_mutex;
		std::queue<std::function<void()>>	m_tasks;
		std::unique_ptr<std::thread>		m_thread;
};

}

}

#endif
