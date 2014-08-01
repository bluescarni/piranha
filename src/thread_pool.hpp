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

#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

#include "config.hpp"
#include "detail/mpfr.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "math.h"
#include "runtime_info.hpp"
#include "thread_management.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Task queue class. Inspired by:
// https://github.com/progschj/ThreadPool
class task_queue
{
		struct runner
		{
			runner(task_queue *ptr, unsigned n):m_ptr(ptr),m_n(n) {}
			void operator()() const
			{
				// Don't stop if we cannot bind.
				try {
					thread_management::bind_to_proc(m_n);
				} catch (...) {
					// NOTE: logging candidate.
				}
				try {
					while (true) {
						std::unique_lock<std::mutex> lock(m_ptr->m_mutex);
						while (!m_ptr->m_stop && m_ptr->m_tasks.empty()) {
							// Need to wait for something to happen only if the task
							// list is empty and we are not stopping.
							// NOTE: wait will be noexcept in C++14.
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
			task_queue	*m_ptr;
			const unsigned	m_n;
		};
	public:
		task_queue(unsigned n):m_stop(false)
		{
			m_thread.reset(::new std::thread(runner{this,n}));
		}
		~task_queue()
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
			// NOTE: notify_one is noexcept.
			m_cond.notify_one();
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

static inline std::vector<std::unique_ptr<task_queue>> get_initial_thread_queues()
{
	std::vector<std::unique_ptr<task_queue>> retval;
	const unsigned candidate = runtime_info::get_hardware_concurrency(), hc = (candidate > 0u) ? candidate : 1u;
	retval.reserve(static_cast<decltype(retval.size())>(hc));
	for (unsigned i = 0u; i < hc; ++i) {
		retval.emplace_back(::new task_queue(i));
	}
	return retval;
}

template <typename = int>
struct thread_pool_base
{
	static std::vector<std::unique_ptr<task_queue>>	s_queues;
	static std::mutex				s_mutex;
};

template <typename T>
std::vector<std::unique_ptr<task_queue>> thread_pool_base<T>::s_queues = get_initial_thread_queues();

template <typename T>
std::mutex thread_pool_base<T>::s_mutex;

}

/// Static thread pool.
/**
 * This class manages, via a set of static methods, a pool of threads created at program startup.
 * The number of threads created initially is equal to piranha::runtime_info::get_hardware_concurrency(),
 * and, if possible, each thread is bound to a different processor. If the hardware concurrency cannot be determined,
 * the size of the thread pool will be one.
 *
 * This class provides methods to enqueue arbitray tasks to the threads in the pool, query the size of the pool
 * and resize the pool. All methods, unless otherwise specified, are thread-safe, and they provide the strong
 * exception safety guarantee.
 *
 * \todo work around MSVC bug in destruction of statically allocated threads (if needed once we support MSVC), as per:
 * http://stackoverflow.com/questions/10915233/stdthreadjoin-hangs-if-called-after-main-exits-when-using-vs2012-rc
 * detach() and wait as a workaround?
 * \todo try to understand if we can suppress the future list class below in favour of STL-like algorithms.
 */
class thread_pool: private detail::thread_pool_base<>
{
		using base = detail::thread_pool_base<>;
	public:
		/// Append task
		/**
		 * \note
		 * This method is enabled only if the expression <tt>f(args...)</tt> is well-formed.
		 *
		 * This method will add a task to the <tt>n</tt>-th thread in the pool. The task is represented
		 * by a callable \p F and its arguments \p args, which will be copied into an execution queue
		 * consumed by the thread to which the task is assigned.
		 *
		 * @param[in] n index of the thread that will consume the task.
		 * @param[in] f callable object representing the task.
		 * @param[in] args arguments to \p f.
		 *
		 * @return an <tt>std::future</tt> that will store the result of <tt>f(args...)</tt>.
		 *
		 * @throws std::invalid_argument if the thread index is equal to or larger than the current pool size.
		 * @throws unspecified any exception thrown by:
		 * - threading primitives,
		 * - memory allocation errors,
		 * - the constructors of \p f or \p args.
		 */
		template <typename F, typename ... Args>
		static auto enqueue(unsigned n, F &&f, Args && ... args) ->
			decltype(base::s_queues[0u]->enqueue(std::forward<F>(f),std::forward<Args>(args)...))
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			using size_type = decltype(base::s_queues.size());
			if (n >= s_queues.size()) {
				piranha_throw(std::invalid_argument,"thread index is out of range");
			}
			return base::s_queues[static_cast<size_type>(n)]->enqueue(std::forward<F>(f),std::forward<Args>(args)...);
		}
		/// Size
		/**
		 * @return the number of threads in the pool.
		 *
		 * @throws unspecified any exception thrown by threading primitives.
		 */
		static unsigned size()
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			return static_cast<unsigned>(base::s_queues.size());
		}
		/// Pool resize
		/**
		 * This method will resize the internal pool to contain \p new_size threads.
		 * The method will first wait for the threads to consume all the pending tasks
		 * (while forbidding the addition of new tasks),
		 * and it will then create a new pool of size \p new_size. The new threads will be
		 * bound, if possible, to the processor corresponding to their index in the pool.
		 *
		 * @param[in] new_size the new size of the pool.
		 *
		 * @throws std::invalid_argument if \p new_size is larger than an implementation-defined value or zero.
		 * @throws unspecified any exception thrown by:
		 * - threading primitives,
		 * - memory allocation errors.
		 */
		static void resize(unsigned new_size)
		{
			if (unlikely(new_size == 0u)) {
				piranha_throw(std::invalid_argument,"cannot resize the thread pool to zero");
			}
			std::lock_guard<std::mutex> lock(s_mutex);
			using size_type = decltype(base::s_queues.size());
			const size_type new_s = static_cast<size_type>(new_size);
			if (unlikely(new_s != new_size)) {
				piranha_throw(std::invalid_argument,"invalid size");
			}
			std::vector<std::unique_ptr<detail::task_queue>> new_queues;
			new_queues.reserve(new_s);
			for (size_type i = 0u; i < new_s; ++i) {
				new_queues.emplace_back(::new detail::task_queue(static_cast<unsigned>(i)));
			}
			// NOTE: here the allocator is not swapped, as std::allocator won't propagate on swap.
			// Besides, all instances of std::allocator are equal, so the operation is well-defined.
			// http://en.cppreference.com/w/cpp/container/vector/swap
			// If an exception gets actually thrown, no big deal.
			new_queues.swap(base::s_queues);
		}
		/// Compute number of threads to use.
		/**
		 * \note
		 * This function is enabled only if \p Int is an unsigned integer type or piranha::integer.
		 * 
		 * This function computes the suggested number of threads to use, given an amount of total \p work_size units of work
		 * and a minimum amount of work units per thread \p min_work_per_thread.
		 * 
		 * The returned value will always be 1 if the calling thread is not the main thread; otherwise, a number of threads
		 * such that each thread has at least \p min_work_per_thread units of work to consume will be returned. In any case, the return
		 * value is always greater than zero.
		 * 
		 * @param[in] work_size total number of work units.
		 * @param[in] min_work_per_thread minimum number of work units to be consumed by a thread in the pool.
		 * 
		 * @return the suggested number of threads to be used, always greater than zero.
		 * 
		 * @throws std::invalid_argument if \p min_work_per_thread is zero.
		 * @throws unspecified any exception thrown by threading primitives.
		 */
		template <typename Int, typename = typename std::enable_if<(std::is_integral<Int>::value && std::is_unsigned<Int>::value) ||
			std::is_same<Int,integer>::value>::type>
		static unsigned use_threads(const Int &work_size, const Int &min_work_per_thread)
		{
			// Check input param.
			if (unlikely(math::is_zero(min_work_per_thread))) {
				piranha_throw(std::invalid_argument,"invalid value for minimum work per thread");
			}
			// Don't use threads if we are not in the main thread.
			if (std::this_thread::get_id() != runtime_info::get_main_thread_id()) {
				return 1u;
			}
			std::lock_guard<std::mutex> lock(s_mutex);
			const auto n_threads = static_cast<unsigned>(base::s_queues.size());
			piranha_assert(n_threads);
			if (work_size / n_threads >= min_work_per_thread) {
				// Enough work per thread, use them all.
				return n_threads;
			}
			// Return a number of threads such that each thread consumes at least min_work_per_thread.
			// Never return 0.
			return static_cast<unsigned>(std::max(Int(1),static_cast<Int>(work_size / min_work_per_thread)));
		}
};

/// Class to store a list of futures.
/**
 * This class is a minimal thin wrapper around an \p std::list of \p std::future objects.
 * The class provides convenience methods to interact with the set of futures in an exception-safe manner.
 *
 * \section type_requirements Type requirements
 *
 * \p F must be an instance of \p std::future.
 *
 * \todo provide method to retrieve future values from get_all() using a vector (in case the future type
 * is not void or a reference, in which case the get_all() method stays as it is).
 */
template <typename F>
class future_list
{
		PIRANHA_TT_CHECK(is_instance_of,F,std::future);
		// Wait on a valid future, or abort.
		static void wait_or_abort(const F &fut)
		{
			piranha_assert(fut.valid());
			try {
				fut.wait();
			} catch (...) {
				// TODO logging candidate, with info from exception.
				std::abort();
			}
		}
	public:
		/// Defaulted default constructor.
		/**
		 * This constructor will initialise an empty list of futures.
		 */
		future_list() = default;
		/// Deleted copy constructor.
		future_list(const future_list &) = delete;
		/// Deleted move constructor.
		future_list(future_list &&) = delete;
		/// Deleted copy assignment.
		future_list &operator=(const future_list &) = delete;
		/// Deleted move assignment.
		future_list &operator=(future_list &&) = delete;
		/// Destructor
		/**
		 * Will call wait_all().
		 */
		~future_list()
		{
			wait_all();
		}
		/// Move-insert a future.
		/**
		 * Will move-insert the input future \p f into the internal container.
		 * If the insertion fails due to memory allocation errors and \p f is a valid
		 * future, then the method will wait on \p f before throwing the exception.
		 *
		 * @param[in] f std::future to be move-inserted.
		 *
		 * @throws unspecified any exception thrown by memory allocation errors.
		 */
		void push_back(F &&f)
		{
			// Push back empty future.
			try {
				m_list.emplace_back();
			} catch (...) {
				// If we get some error here, we want to make sure we wait on the future
				// before escaping out.
				// NOTE: calling wait() on an invalid future is UB.
				if (f.valid()) {
					wait_or_abort(f);
				}
				throw;
			}
			// This cannot throw.
			m_list.back() = std::move(f);
		}
		/// Wait on all the futures.
		/**
		 * This method will call <tt>wait()</tt> on all the valid futures stored within the object.
		 */
		void wait_all() noexcept
		{
			for (auto &f: m_list) {
				if (f.valid()) {
					wait_or_abort(f);
				}
			}
		}
		/// Get all the futures.
		/**
		 * This method will call <tt>get()</tt> on all the valid futures stored within the object.
		 * The return values resulting from the calls to <tt>get()</tt> will be ignored.
		 *
		 * @throws unspecified an exception stored by a future.
		 */
		void get_all()
		{
			for (auto &f: m_list) {
				// NOTE: std::future's valid() method is noexcept.
				if (f.valid()) {
					(void)f.get();
				}
			}
		}
	private:
		std::list<F> m_list;
};

}

#endif
