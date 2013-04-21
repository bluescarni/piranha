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

#ifndef PIRANHA_THREADING_HPP
#define PIRANHA_THREADING_HPP

/** \file threading.hpp
 * \brief Threading primitives.
 * 
 * The functions, classes and typedefs in this file provide an abstraction layer to threading primitives on the host platform.
 * If standard C++ threading support is available, it will be used, otherwise Boost.Thread will be used.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread.html
 * 
 * \todo review the semantics of boost stuff vs std c++, we know for instance that future::get() is a bit different, check
 * there are no other surprises.
 */

#include <exception>
#include <functional>
#include <type_traits>

#include "config.hpp"
#include "detail/mpfr.hpp"

#if defined(PIRANHA_USE_BOOST_THREAD)
	#include <boost/exception_ptr.hpp>
	#include <boost/thread/condition_variable.hpp>
	#include <boost/thread/future.hpp>
	#include <boost/thread/locks.hpp>
	#include <boost/thread/mutex.hpp>
	#include <boost/thread/thread.hpp>
#else
	#include <condition_variable>
	#include <exception>
	#include <future>
	#include <mutex>
	#include <thread>
#endif

namespace piranha
{

namespace detail
{

typedef
#if defined(PIRANHA_USE_BOOST_THREAD)
	boost::thread base_thread;
#else
	std::thread base_thread;
#endif

}

/// Thread class.
/**
 * Basic thread class deriving from either <tt>std::thread</tt> or <tt>boost::thread</tt>. Only a basic common interface
 * to both implementations is provided.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/thread_management.html#thread.thread_management.thread
 */
 class thread: private detail::base_thread
{
		typedef detail::base_thread base;
		template <typename Callable, typename... Args>
		static void wrapper(Callable &func, Args & ... args)
		{
			try {
				func(args...);
			} catch (...) {
				::mpfr_free_cache();
				throw;
			}
			::mpfr_free_cache();
		}
	public:
		/// Default constructor.
		/**
		 * Will construct a thread object without an associated thread of execution.
		 */
		thread():base() {}
		/// Thread-launching constructor.
		/**
		 * Will construct a thread object that will run the supplied function \p func with arguments \p args.
		 * The function \p func will be encapsulated in a wrapper that will call the MPFR function <tt>mpfr_free_cache()</tt>
		 * upon the completion of \p func, in order to make it safe to use the piranha::real class across different threads.
		 * 
		 * @param[in] func function that will be called in the thread.
		 * @param[in] args arguments to \p func.
		 * 
		 * @throws unspecified any exception thrown by the base constructor or by copying \p func or \p args.
		 * 
		 * @see http://www.mpfr.org/mpfr-current/mpfr.html#Memory-Handling
		 */
		template <typename Callable, typename... Args>
		explicit thread(Callable &&func, Args && ... args):
			base(std::bind(wrapper<typename std::decay<Callable>::type,
			typename std::decay<Args>::type...>,std::forward<Callable>(func),std::forward<Args>(args)...)) {}
		/// Deleted copy constructor.
		thread(const thread &) = delete;
		/// Defaulted move constructor.
		thread(thread &&) = default;
		/// Destructor.
		/**
		 * If \p this is associated to a thread of execution, <tt>std::terminate()</tt> will be called.
		 */
		~thread()
		{
			if (joinable()) {
				std::terminate();
			}
		}
		/// Deleted copy assignment operator.
		thread &operator=(const thread &) = delete;
		/// Trivial move assignment operator.
		/**
		 * Will forward the call to the corresponding move assignment in the base class.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		thread &operator=(thread &&other) noexcept(true)
		{
			if (this != &other) {
				base::operator=(std::move(other));
			}
			return *this;
		}
		/// Thread join.
		/**
		 * Wait for the thread of execution associated with \p this to finish. The method will check
		 * if the thread is joinable before calling the base join method, hence it is safe to call this method
		 * multiple times on the same thread object.
		 * 
		 * @throws unspecified any exception thrown by the base method.
		 */
		void join()
		{
			if (joinable()) {
				base::join();
			}
		}
		/// Thread detach.
		/**
		 * Detach the thread of execution associated with \p this to finish. The method will check
		 * if the thread is joinable before calling the base detach method, hence it is safe to call this method
		 * multiple times on the same thread object.
		 * 
		 * @throws unspecified any exception thrown by the base method.
		 */
		void detach()
		{
			if (joinable()) {
				base::detach();
			}
		}
		/// Joinable flag.
		/**
		 * Query whether or not \p this has an associated thread of execution.
		 * 
		 * @return joinable flag.
		 */
		bool joinable() const
		{
			return base::joinable();
		}
};

/// Condition variable type.
/**
 * Typedef for either <tt>std::condition_variable</tt> or <tt>boost::condition_variable</tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
 */
#if defined(PIRANHA_USE_BOOST_THREAD)
typedef boost::condition_variable condition_variable;
#else
typedef std::condition_variable condition_variable;
#endif

/// Mutex type.
/**
 * Typedef for either <tt>std::mutex</tt> or <tt>boost::mutex</tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/interprocess/synchronization_mechanisms.html
 */
#if defined(PIRANHA_USE_BOOST_THREAD)
typedef boost::mutex mutex;
#else
typedef std::mutex mutex;
#endif

/// Exception pointer type.
/**
 * Typedef for either <tt>std::exception_ptr</tt> or <tt>boost::exception_ptr</tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/libs/exception/doc/exception_ptr.html
 */
#if defined(PIRANHA_USE_BOOST_THREAD)
typedef boost::exception_ptr exception_ptr;
#else
typedef std::exception_ptr exception_ptr;
#endif

/// Current exception.
/**
 * Wrapper around either <tt>std::current_exception()</tt> or <tt>boost::current_exception()</tt>
 * 
 * @return a piranha::exception_ptr that refers to the currently handled exception or a copy of the currently handled exception.
 * If the function needs to allocate memory and the attempt fails, it returns a piranha::exception_ptr that refers to an instance of <tt>std::bad_alloc</tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/libs/exception/doc/current_exception.html
 */
inline exception_ptr current_exception()
{
#if defined(PIRANHA_USE_BOOST_THREAD)
	return boost::current_exception();
#else
	return std::current_exception();
#endif
}

/// Lock guard selector.
/**
 * Provides a typedef for either <tt>std::lock_guard<Lockable></tt> or <tt>boost::lock_guard<Lockable></tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.locks.lock_guard
 * 
 * \todo replace with template typedef, once it gets implemented.
 */
template <typename Lockable>
struct lock_guard
{
	/// Type definition.
#if defined(PIRANHA_USE_BOOST_THREAD)
	typedef boost::lock_guard<Lockable> type;
#else
	typedef std::lock_guard<Lockable> type;
#endif
};

/// Unique lock selector.
/**
 * Provides a typedef for either <tt>std::unique_lock<Lockable></tt> or <tt>boost::unique_lock<Lockable></tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.locks.unique_lock
 * 
 * \todo replace with template typedef, once it gets implemented.
 */
template <typename Lockable>
struct unique_lock
{
	/// Type definition.
#if defined(PIRANHA_USE_BOOST_THREAD)
	typedef boost::unique_lock<Lockable> type;
#else
	typedef std::unique_lock<Lockable> type;
#endif
};

/// Thread ID.
/**
 * Typedef for either <tt>std::thread::id</tt> or <tt>boost::thread::id</tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/thread_management.html#thread.thread_management.thread.id
 */
#if defined(PIRANHA_USE_BOOST_THREAD)
typedef boost::thread::id thread_id;
#else
typedef std::thread::id thread_id;
#endif

/// Future selector.
/**
 * Provides a typedef for either <tt>std::future<T></tt> or <tt>boost::unique_future<T></tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.futures
 */
template <typename T>
struct future
{
	/// Type definition.
#if defined(PIRANHA_USE_BOOST_THREAD)
	typedef boost::unique_future<T> type;
#else
	typedef std::future<T> type;
#endif
};

/// Promise selector.
/**
 * Provides a typedef for either <tt>std::promise<T></tt> or <tt>boost::promise<T></tt>.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.futures
 */
template <typename T>
struct promise
{
	/// Type definition.
#if defined(PIRANHA_USE_BOOST_THREAD)
	typedef boost::promise<T> type;
#else
	typedef std::promise<T> type;
#endif
};

/// Calling thread namespace.
/**
 * The functions in this namespace operate on the calling thread.
 * This namespace is meant to be equivalent to either <tt>std::this_thread</tt> or <tt>boost::this_thread</tt>.
 */
namespace this_thread
{

/// Get thread ID.
/**
 * @return piranha::thread_id of the calling thread.
 */
inline thread_id get_id()
{
#if defined(PIRANHA_USE_BOOST_THREAD)
	return boost::this_thread::get_id();
#else
	return std::this_thread::get_id();
#endif
}

}

}

#endif
