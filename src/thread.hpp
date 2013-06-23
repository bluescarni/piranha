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

#ifndef PIRANHA_THREAD_HPP
#define PIRANHA_THREAD_HPP

#include <exception>
#include <thread>
#include <type_traits>

#include "detail/mpfr.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Thread class.
/**
 * Basic thread class encapsulating <tt>std::thread</tt>.
 */
class thread
{
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
		/// Deleted default constructor.
		thread() = delete;
		/// Constructor from function object.
		/**
		 * \note
		 * This constructor is enabled if \p Callable is a nullary function object returning \p void,
		 * and if it is move-constructible and constructible from <tt>Callable &&</tt>.
		 *
		 * The input callable \p c will be wrapped in a function object that will call the MPFR function <tt>mpfr_free_cache()</tt>
		 * after invoking the original callable \p c (so that it is safe to use MPFR functions from multiple thread objects). The wrapped
		 * function object is used as argument for the construction of the internal thread object.
		 *
		 * @param[in] c function object that will be used to construct the internal thread object.
		 *
		 * @throws unspecified any exception thrown by the move/copy constructor of \p c or by the constructor of the internal thread object.
		 *
		 * @see http://www.mpfr.org/mpfr-current/mpfr.html#Memory-Handling
		 */
		template <typename Callable, typename = typename std::enable_if<is_function_object<
			typename std::decay<Callable>::type,void>::value &&
			std::is_move_constructible<typename std::decay<Callable>::type>::value &&
			std::is_constructible<typename std::decay<Callable>::type,Callable &&>::value>::type>
		explicit thread(Callable &&c):m_thread(wrapper<typename std::decay<Callable>::type>(std::forward<Callable>(c))) {}
		/// Deleted copy constructor.
		thread(const thread &) = delete;
		/// Defaulted move constructor.
		thread(thread &&) = default;
		/// Deleted copy assignment operator.
		thread &operator=(const thread &) = delete;
		/// Defaulted move assignment operator.
		thread &operator=(thread &&) = default;
		/// Destructor.
		/**
		 * If \p this is associated to a thread of execution, <tt>std::terminate()</tt> will be called.
		 */
		~thread() noexcept(true)
		{
			if (joinable()) {
				std::terminate();
			}
		}
		/// Joinable flag.
		/**
		 * Query whether or not \p this has an associated thread of execution.
		 *
		 * @return joinable flag.
		 */
		bool joinable() const noexcept(true)
		{
			return m_thread.joinable();
		}
		/// Thread join.
		/**
		 * Wait for the thread of execution associated with \p this to finish. The method will check
		 * if the thread is joinable before calling the internal join method, hence it is safe to call this method
		 * multiple times on the same thread object.
		 *
		 * @throws unspecified any exception thrown by the corresponding method of the internal thread object.
		 */
		void join()
		{
			if (joinable()) {
				m_thread.join();
			}
		}
		/// Thread detach.
		/**
		 * Detach the thread of execution associated with \p this. The method will check
		 * if the thread is joinable before calling the internal detach method, hence it is safe to call this method
		 * multiple times on the same thread object.
		 *
		 * @throws unspecified any exception thrown by the corresponding method of the internal thread object.
		 */
		void detach()
		{
			if (joinable()) {
				m_thread.detach();
			}
		}
	private:
		std::thread m_thread;
};

}

#endif
