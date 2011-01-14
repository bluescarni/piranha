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

#ifndef PIRANHA_CVECTOR_HPP
#define PIRANHA_CVECTOR_HPP

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/utility.hpp>
#include <cstddef>
#include <exception>
#include <cstring>
#include <memory>
#include <mutex>
#include <new> // For std::bad_alloc.
#include <thread>
#include <type_traits>
#include <vector>

#include "config.hpp" // For piranha_assert.
#include "exceptions.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "thread_barrier.hpp"
#include "thread_group.hpp"
#include "thread_management.hpp"
#include "type_traits.hpp"

namespace piranha {

/// Concurrent vector class.
/**
 * This class is a minimal vector class which can use multiple threads during construction,
 * destruction and resize. Whether or not actual threads are spawned depends on the return value of
 * piranha::settings::get_n_threads() and the number of elements in the container. I.e., if
 * the user requests the use of a single thread or if the number of elements in the container is lower than an
 * implementation-defined value, no new threads will be opened. No new threads will be opened also in case
 * the vector instance is used from a thread different from the main one.
 * 
 * \section exception_safety Exception safety guarantees
 * 
 * The class provides the strong exception safety guarantee: both in single-thread and multi-thread mode,
 * any exception will be caught and re-thrown after the original state of the object has been restored. In case of multi-thread
 * mode, exceptions thrown in the separate threads will be stored and re-thrown in the main thread. At most one exception per thread
 * will be generated: which exception is re-thrown in the main thread is dependent on the thread scheduling, and it is therefore
 * undefined (and most likely nondeterministic).
 *
 * The following important <b>caveat</b> applies: any exception thrown
 * from a thread separate from the main one and resulting from an error condition in the thread-related primitives
 * used in the internal implementation (e.g., locks, mutexes, condition variables), will result in an <tt>std::system_error</tt> exception
 * begin thrown from the thread and subsequent program termination. This limitation is related to the use of locking primitives
 * in the <tt>catch (...)</tt> blocks within each thread, and might be lifted in the future with the adoption of lock-free data
 * structures for transporting exceptions.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo Affinity settings.
 * \todo Performance tuning on the minimum work size. Make it template parameter with default value?
 * \todo Fix minimum work size!
 */
template <typename T>
class cvector
{
	public:
		/// Value type.
		typedef T value_type;
		/// Size type.
		typedef std::size_t size_type;
	private:
		// Allocator type.
		typedef std::allocator<T> allocator_type;
		// Structure used internally for thread handling.
		struct thread_control
		{
			const size_type			work_size;
			const size_type			offset;
			const size_type			n_threads;
			size_type			*n_started_threads;
			thread_barrier			*barrier;
			std::mutex			*mutex;
			std::vector<std::exception_ptr>	*exceptions;
		};
		template <typename Functor, typename... Args>
		static void thread_runner(Functor &&f, const size_type &size, Args && ... params)
		{
			piranha_assert(size > 0);
			size_type n_threads;
			// If we are already being called by a thread separate from
			// the main one, force a single thread.
			if (std::this_thread::get_id() != runtime_info::get_main_thread_id()) {
				n_threads = 1;
			} else {
				n_threads = boost::numeric_cast<size_type>(settings::get_n_threads());
			}
			piranha_assert(n_threads > 0);
			// Make sure that every thread has a minimum amount of work to do. If necessary, reduce the number of threads.
			const size_type min_work = 100000;
			n_threads = (size / n_threads >= min_work) ? n_threads : std::max<size_type>(static_cast<size_type>(1),size / min_work);
			const size_type work_size = size / n_threads;
			piranha_assert(n_threads > 0);
			// Variables to control the thread(s).
			std::mutex mutex;
			std::vector<std::exception_ptr> exceptions;
			thread_barrier barrier(n_threads);
			// Reserve enough space to store exceptions, so that we avoid potential problems
			// when pushing back the exceptions in the catch(...) blocks.
			exceptions.reserve(boost::numeric_cast<std::vector<std::exception_ptr>::size_type>(n_threads));
			if (exceptions.capacity() < n_threads) {
				throw std::bad_alloc();
			}
			// If we have just one thread, do not actually open new threads.
			if (n_threads == 1) {
				thread_control tc = {size,0,1,piranha_nullptr,&barrier,&mutex,&exceptions};
				f(tc,std::forward<Args>(params)...);
			} else {
				thread_group tg;
				size_type i = 0, n_started_threads = 0;
				{
					std::lock_guard<std::mutex> lock(mutex);
					for (; i < n_threads - static_cast<size_type>(1); ++i) {
						thread_control tc = {work_size,i * work_size,n_threads,&n_started_threads,&barrier,&mutex,&exceptions};
						try {
							tg.create_thread(std::forward<Functor>(f),tc,std::forward<Args>(params)...);
							++n_started_threads;
						} catch (...) {
							exceptions.push_back(std::current_exception());
						}
					}
					// Last thread might have more work to do.
					thread_control tc = {size - work_size * i,i * work_size,n_threads,&n_started_threads,&barrier,&mutex,&exceptions};
					try {
						tg.create_thread(std::forward<Functor>(f),tc,std::forward<Args>(params)...);
						++n_started_threads;
					} catch (...) {
						exceptions.push_back(std::current_exception());
					}
				}
				tg.join_all();
			}
			piranha_assert(exceptions.size() <= n_threads);
			// Rethrow the first exception encountered.
			for (std::vector<std::exception_ptr>::size_type i = 0; i < exceptions.size(); ++i) {
				if (exceptions[i] != piranha_nullptr) {
					std::rethrow_exception(exceptions[i]);
				}
			}
			
		}
		// Helper function to detect if all threads could be started in the thread runner.
		static bool is_thread_ready(thread_control &tc)
		{
			piranha_assert(tc.n_threads > 0);
			if (tc.n_threads > 1) {
				piranha_assert(tc.n_started_threads && tc.mutex);
				std::lock_guard<std::mutex> lock(*tc.mutex);
				if (*tc.n_started_threads != tc.n_threads) {
					return false;
				} else {
					return true;
				}
			} else {
				return true;
			}
		}
		// Helper function to wait on the barrier used for rollback operations.
		static void barrier_wait(thread_control &tc)
		{
			piranha_assert(tc.n_threads > 0);
			if (tc.n_threads > 1) {
				piranha_assert(tc.barrier);
				tc.barrier->wait();
			}
		}
		// Helper function to store exceptions in threads.
		static void store_exception(thread_control &tc)
		{
			piranha_assert(tc.n_threads > 0);
			piranha_assert(tc.exceptions);
			if (tc.n_threads > 1) {
				piranha_assert(tc.mutex && tc.exceptions);
				std::lock_guard<std::mutex> lock(*tc.mutex);
				tc.exceptions->push_back(std::current_exception());
			} else {
				tc.exceptions->push_back(std::current_exception());
			}
		}
		struct default_ctor
		{
			// Construction via copy constructor of a default-constructed instance.
			void impl(thread_control &tc, value_type *begin) const
			{
				size_type i = 0;
				try {
					const value_type tmp = value_type();
					value_type *current = begin + tc.offset;
					for (; i < tc.work_size; ++i, ++current) {
						new ((void *)current) value_type(tmp);
					}
				} catch (...) {
					// Store the exception.
					store_exception(tc);
				}
				barrier_wait(tc);
				if (tc.exceptions->size() != 0) {
					// Do the rollback if any exception was thrown in any thread.
					value_type *current = begin + tc.offset;
					for (size_type j = 0; j < i; ++j, ++current) {
						current->~value_type();
					}
				}
			}
			void operator()(thread_control &tc, value_type *begin) const
			{
				thread_management::binder b;
				if (!is_thread_ready(tc)) {
					return;
				}
				impl(tc,begin);
			}
		};
		struct destructor
		{
			// Trivial destructor: do not do anything, simple deallocation is fine.
			void impl(thread_control &, value_type *, const std::true_type &) const
			{}
			void impl(thread_control &tc, value_type *begin, const std::false_type &) const
			{
				value_type *current = begin + tc.offset;
				for (size_type i = 0; i < tc.work_size; ++i, ++current) {
					current->~value_type();
				}
			}
			void operator()(thread_control &tc, value_type *begin) const
			{
				thread_management::binder b;
				if (!is_thread_ready(tc)) {
					return;
				}
				impl(tc,begin,std::has_trivial_destructor<value_type>());
			}
		};
		struct copy_ctor
		{
			// Trivial copy construction via memcpy.
			void impl(thread_control &tc, value_type *dest_begin, const value_type *src_begin, const std::true_type &) const
			{
				std::memcpy(static_cast<void *>(dest_begin + tc.offset),static_cast<void const *>(src_begin + tc.offset),sizeof(value_type) * tc.work_size);
			}
			// Non-trivial copy construction.
			void impl(thread_control &tc, value_type *dest_begin, const value_type *src_begin, const std::false_type &) const
			{
				size_type i = 0;
				try {
					value_type *dest_current = dest_begin + tc.offset;
					value_type const *src_current = src_begin + tc.offset;
					for (; i < tc.work_size; ++i, ++dest_current, ++src_current) {
						new ((void *)dest_current) value_type(*src_current);
					}
				} catch (...) {
					// Store the exception.
					store_exception(tc);
				}
				barrier_wait(tc);
				if (tc.exceptions->size() != 0) {
					// Roll back.
					value_type *dest_current = dest_begin + tc.offset;
					for (size_type j = 0; j < i; ++j, ++dest_current) {
						dest_current->~value_type();
					}
				}
			}
			void operator()(thread_control &tc, value_type *dest_begin, const value_type *src_begin) const
			{
				thread_management::binder b;
				if (!is_thread_ready(tc)) {
					return;
				}
				impl(tc,dest_begin,src_begin,is_trivially_copyable<value_type>());
			}
		};
		struct mover
		{
			void impl(thread_control &tc, value_type *dest_begin, value_type *src_begin, const std::true_type &tt) const
			{
				// Moving a trivial object means simply to copy it.
				copy_ctor().impl(tc,dest_begin,src_begin,tt);
			}
			void impl(thread_control &tc, value_type *dest_begin, value_type *src_begin, const std::false_type &) const
			{
				size_type i = 0;
				try {
					value_type *dest_current = dest_begin + tc.offset, *src_current = src_begin + tc.offset;
					for (; i < tc.work_size; ++i, ++dest_current, ++src_current) {
						new ((void *)dest_current) value_type(piranha_move_if_noexcept(*src_current));
					}
				} catch (...) {
					store_exception(tc);
				}
				barrier_wait(tc);
				if (tc.exceptions->size() != 0) {
					value_type *dest_current = dest_begin + tc.offset;
					for (size_type j = 0; j < i; ++j, ++dest_current) {
						dest_current->~value_type();
					}
				}
			}
			void operator()(thread_control &tc, value_type *dest_begin, value_type *src_begin) const
			{
				thread_management::binder b;
				if (!is_thread_ready(tc)) {
					return;
				}
				// NOTE: possibly here it should really be: move it if it has a move ctor
				// and it is not trivially copyable otherwise fall back to copying.
				impl(tc,dest_begin,src_begin,is_trivially_copyable<value_type>());
			}
		};
	public:
		/// Iterator type.
		typedef T * iterator;
		/// Const iterator type.
		typedef const T * const_iterator;
		/// Default constructor.
		/**
		 * Will build an empty vector.
		 */
		cvector() piranha_noexcept(true) : m_data(piranha_nullptr),m_size(0) {}
		/// Copy constructor.
		/**
		 * @param[in] other vector that will be copied.
		 * 
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 * @throws std::bad_alloc in case of memory allocation failure(s).
		 * @throws boost::numeric::bad_numeric_cast in case of over/under-flow in numeric conversions.
		 * @throws unspecified any exception thrown by copy-constructing objects of type \p T.
		 */
		cvector(const cvector &other):m_data(piranha_nullptr),m_size(0)
		{
			if (other.size() == 0) {
				return;
			}
			m_data = m_allocator.allocate(other.size());
			copy_ctor c;
			try {
				thread_runner(c,other.size(),m_data,other.m_data);
			} catch (...) {
				m_allocator.deallocate(m_data,other.size());
				throw;
			}
			m_size = other.size();
		}
		/// Move constructor.
		/**
		 * After the move, \p other will be equivalent to a default-constructed vector.
		 * 
		 * @param[in] other vector that will be moved.
		 */
		cvector(cvector &&other) piranha_noexcept(true) : m_data(other.m_data),m_size(other.m_size)
		{
			// Empty the other cvector.
			other.m_data = piranha_nullptr;
			other.m_size = 0;
		}
		/// Constructor from size.
		/**
		 * Will build a vector of default-constructed elements.
		 * 
		 * @param[in] size desired vector size.
		 *
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 * @throws std::bad_alloc in case of memory allocation failure(s).
		 * @throws boost::numeric::bad_numeric_cast in case of over/under-flow in numeric conversions.
		 * @throws unspecified any exception thrown by default-constructing objects of type \p T.
		 */
		explicit cvector(const size_type &size):m_data(piranha_nullptr),m_size(0),m_allocator()
		{
			if (size == 0) {
				return;
			}
			m_data = m_allocator.allocate(boost::numeric_cast<typename allocator_type::size_type>(size));
			default_ctor f;
			try {
				thread_runner(f,size,m_data);
			} catch (...) {
				// Any construction has been rolled back, just de-allocate and re-throw.
				m_allocator.deallocate(m_data,size);
				throw;
			}
			m_size = size;
		}
		/// Destructor.
		/**
		 * Will destroy all elements and de-allocate the internal memory.
		 */
		~cvector()
		{
			assert((m_size == 0 && m_data == piranha_nullptr) || (m_size != 0 && m_data != piranha_nullptr));
			// Here throwing can happen, e.g., in thread starting or copying arguments for the thread functions.
			try {
				destroy_and_deallocate();
			} catch (...) {
				// If anything goes wrong, do a bare destruction function
				// that will not throw.
				for (size_type i = 0; i < m_size; ++i) {
					(m_data + i)->~value_type();
				}
				// No need for numeric cast, as we were able to construct the object in the first place.
				m_allocator.deallocate(m_data,m_size);
			}
		}
		/// Move assignment operator.
		/**
		 * After the move, \p other will be equivalent to a default-constructed vector.
		 * 
		 * @param[in] other vector that will be moved.
		 * 
		 * @return reference to this.
		 */
		cvector &operator=(cvector &&other) piranha_noexcept(true)
		{
			destroy_and_deallocate();
			m_data = other.m_data;
			m_size = other.m_size;
			// Empty other.
			other.m_data = piranha_nullptr;
			other.m_size = 0;
			return *this;
		}
		/// Assignment operator.
		/**
		 * @param[in] other vector to be copied.
		 * 
		 * @return reference to this.
		 * 
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 * @throws std::bad_alloc in case of memory allocation failure(s).
		 * @throws boost::numeric::bad_numeric_cast in case of over/under-flow in numeric conversions.
		 * @throws unspecified any exception thrown by copy-constructing objects of type \p T.
		 */
		cvector &operator=(const cvector &other)
		{
			if (this == boost::addressof(other)) {
				return *this;
			}
			if (other.size() == 0) {
				resize(0);
			} else {
				value_type *new_data = m_allocator.allocate(other.size());
				copy_ctor c;
				try {
					thread_runner(c,other.size(),new_data,other.m_data);
				} catch (...) {
					m_allocator.deallocate(new_data,other.size());
					throw;
				}
				// Erase previous content.
				destroy_and_deallocate();
				// Final assignment.
				m_data = new_data;
				m_size = other.size();
			}
			return *this;
		}
		/// Const begin iterator.
		/**
		 * @return cvector::const_iterator to the first element, or end() if the vector is empty.
		 */
		const_iterator begin() const
		{
			return m_data;
		}
		/// Const end iterator.
		/**
		 * @return cvector::const_iterator to the position after the last element.
		 */
		const_iterator end() const
		{
			return m_data + m_size;
		}
		/// Begin iterator.
		/**
		 * @return cvector::iterator to the first element, or end() if the vector is empty.
		 */
		iterator begin()
		{
			return m_data;
		}
		/// End iterator.
		/**
		 * @return cvector::iterator to the position after the last element.
		 */
		iterator end()
		{
			return m_data + m_size;
		}
		/// Size getter.
		/**
		 * @return the size of the vector.
		 */
		size_type size() const piranha_noexcept(true)
		{
			return m_size;
		}
		/// Resize.
		/**
		 * Resize the vector preserving the existing content. New elements will be default-constructed.
		 * 
		 * @param[in] size new size.
		 * 
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 * @throws std::bad_alloc in case of memory allocation failure(s).
		 * @throws boost::numeric::bad_numeric_cast in case of over/under-flow in numeric conversions.
		 * @throws unspecified any exception thrown by default-constructing or copying objects of type \p T.
		 */
		void resize(const size_type &size)
		{
			// Do nothing if size is the same.
			if (size == m_size) {
				return;
			}
			// For resize(0), destroy and zero out the data members.
			if (size == 0) {
				destroy_and_deallocate();
				m_data = piranha_nullptr;
				m_size = 0;
				return;
			}
			// Allocate the new data.
			value_type *new_data = m_allocator.allocate(boost::numeric_cast<typename allocator_type::size_type>(size));
			// Default-construct the new data, if any.
			if (size > m_size) {
				default_ctor f;
				try {
					thread_runner(f,size - m_size,new_data + m_size);
				} catch (...) {
					// Default constructions have been rolled back, deallocate and throw.
					m_allocator.deallocate(new_data,size);
					throw;
				}
			}
			// Move the old data, to the minimum of old and new size.
			if (m_size) {
				mover m;
				try {
					thread_runner(m,std::min<size_type>(m_size,size),new_data,m_data);
				} catch (...) {
					// We reach here only if we did not actually move, but copied. The copied
					// objects have been rolled back by the mover, now we need to destroy the default
					// constructed objects from above (if any) and deallocate the new data.
					if (size > m_size) {
						// Here we could have problems starting threads, catch any error and
						// perform manual destruction.
						try {
							destructor d;
							thread_runner(d,size - m_size,new_data + m_size);
						} catch (...) {
							for (size_type i = 0; i < size - m_size; ++i) {
								(new_data + m_size + i)->~value_type();
							}
						}
					}
					m_allocator.deallocate(new_data,size);
					throw;
				}
			}
			// Destroy and deallocate old data.
			destroy_and_deallocate();
			// Final assignment of new data.
			m_data = new_data;
			m_size = size;
		}
		/// Mutating accessor.
		/**
		 * @param[in] n element index.
		 * 
		 * @return reference to the <tt>n</tt>-th element of the container.
		 */
		value_type &operator[](const size_type &n) piranha_noexcept(true)
		{
			piranha_assert(n < m_size);
			return m_data[n];
		}
		/// Non-mutating accessor.
		/**
		 * @param[in] n element index.
		 * 
		 * @return const reference to the <tt>n</tt>-th element of the container.
		 */
		const value_type &operator[](const size_type &n) const piranha_noexcept(true)
		{
			piranha_assert(n < m_size);
			return m_data[n];
		}
	private:
		void destroy_and_deallocate()
		{
			if (m_size == 0) {
				return;
			}
			destructor d;
			thread_runner(d,m_size,m_data);
			// No need for numeric cast, as we were able to construct the object in the first place.
			m_allocator.deallocate(m_data,m_size);
		}
	private:
		value_type	*m_data;
		size_type	m_size;
		allocator_type	m_allocator;
};

}

#endif
