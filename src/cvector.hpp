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
#include "thread_group.hpp"

namespace piranha {

/// Concurrent vector class.
/**
 * This class is a minimal vector class which can use multiple threads during construction,
 * destruction and resize.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo Affinity settings.
 * \todo Performance tuning on the minimum work size. Make it template parameter with default value?
 */
template <class T>
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
			size_type						work_size;
			size_type						offset;
			size_type						n_threads;
			std::mutex						*mutex;
			std::vector<std::exception_ptr>				*exceptions;
		};
		template <typename Functor, typename... Args>
		static void thread_runner(Functor &&f, const size_type &size, Args && ... params)
		{
			piranha_assert(size > 0);
			size_type n_threads = boost::numeric_cast<size_type>(settings::get_n_threads());
			piranha_assert(n_threads > 0);
			// Make sure that every thread has a minimum amount of work to do. If necessary, reduce the number of threads.
			const size_type min_work = 50;
			n_threads = (size / n_threads >= min_work) ? n_threads : std::max<size_type>(static_cast<size_type>(1),size / min_work);
			const size_type work_size = size / n_threads;
			piranha_assert(n_threads > 0);
			std::mutex mutex;
			std::vector<std::exception_ptr> exceptions;
			// Reserve enough space to store exceptions, so that we avoid potential problems
			// when pushing back the exceptions in the catch(...) blocks.
			exceptions.reserve(boost::numeric_cast<std::vector<std::exception_ptr>::size_type>(n_threads));
			if (exceptions.capacity() < boost::numeric_cast<std::vector<std::exception_ptr>::size_type>(n_threads)) {
				throw std::bad_alloc();
			}
			// If we have just one thread or we are already being called by a thread separate from
			// the main one, do not open new threads.
			if (n_threads == 1 || std::this_thread::get_id() != runtime_info::get_main_thread_id()) {
				thread_control tc = {size,0,1,&mutex,&exceptions};
				f(tc,std::forward<Args>(params)...);
			} else {
				thread_group tg;
				size_type i = 0;
				for (; i < n_threads - static_cast<size_type>(1); ++i) {
					thread_control tc = {work_size,i * work_size,n_threads,&mutex,&exceptions};
					tg.create_thread(std::forward<Functor>(f),tc,std::forward<Args>(params)...);
				}
				// Last thread might have more work to do.
				thread_control tc = {size - work_size * i,i * work_size,n_threads,&mutex,&exceptions};
				tg.create_thread(std::forward<Functor>(f),tc,std::forward<Args>(params)...);
				tg.join_all();
			}
			// Rethrow the first exception encountered.
			for (std::vector<std::exception_ptr>::size_type i = 0; i < exceptions.size(); ++i) {
				if (exceptions[i] != piranha_nullptr) {
					std::rethrow_exception(exceptions[i]);
				}
			}
			
		}
		struct default_ctor
		{
			// Trivial construction via assignment.
			void impl(thread_control &tc, value_type *begin, const std::true_type &) const
			{
				try {
					const value_type tmp = value_type();
					value_type *current = begin + tc.offset;
					for (size_type i = 0; i < tc.work_size; ++i, ++current) {
						*current = tmp;
					}
				} catch (...) {
					// No need to do any rolling back for trivial objects. Just store the exception.
					// NOTE: is it even possible for trivial objects to throw on default construction and/or
					// assignment?
					std::lock_guard<std::mutex> lock(*tc.mutex);
					tc.exceptions->push_back(std::current_exception());
				}
			}
			// Non-trivial construction.
			void impl(thread_control &tc, value_type *begin, const std::false_type &) const
			{
				size_type i = 0;
				try {
					allocator_type a;
					value_type *current = begin + tc.offset;
					const value_type tmp = value_type();
					for (; i < tc.work_size; ++i, ++current) {
						a.construct(current,tmp);
					}
				} catch (...) {
					allocator_type a;
					value_type *current = begin + tc.offset;
					// Roll back the construction.
					for (size_type j = 0; j < i; ++j, ++current) {
						a.destroy(current);
					}
					// Store the exception.
					std::lock_guard<std::mutex> lock(*tc.mutex);
					tc.exceptions->push_back(std::current_exception());
				}
			}
			void operator()(thread_control &tc, value_type *begin) const
			{
				// NOTE: replace with is_trivially_copyable? At the moment it seems not to be present
				// in GCC 4.5.
				impl(tc,begin,std::is_trivial<value_type>());
			}
		};
		struct destructor
		{
			// Trivial destructor: do not do anything, simple deallocation is fine.
			void impl(thread_control &, value_type *, const std::true_type &) const
			{}
			void impl(thread_control &tc, value_type *begin, const std::false_type &) const
			{
				allocator_type a;
				value_type *current = begin + tc.offset;
				for (size_type i = 0; i < tc.work_size; ++i, ++current) {
					a.destroy(current);
				}
			}
			void operator()(thread_control &tc, value_type *begin) const
			{
				impl(tc,begin,std::is_trivial<value_type>());
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
					allocator_type a;
					value_type *dest_current = dest_begin + tc.offset;
					value_type const *src_current = src_begin + tc.offset;
					for (; i < tc.work_size; ++i, ++dest_current, ++src_current) {
						a.construct(dest_current,*src_current);
					}
				} catch (...) {
					// Roll back.
					allocator_type a;
					value_type *dest_current = dest_begin + tc.offset;
					for (size_type j = 0; j < i; ++j, ++dest_current) {
						a.destroy(dest_current);
					}
					// Store the exception.
					std::lock_guard<std::mutex> lock(*tc.mutex);
					tc.exceptions->push_back(std::current_exception());
				}
			}
			void operator()(thread_control &tc, value_type *dest_begin, const value_type *src_begin) const
			{
				impl(tc,dest_begin,src_begin,std::is_trivial<value_type>());
			}
		};
		struct mover
		{
			void impl(thread_control &tc, value_type *dest_begin, value_type *src_begin, const std::true_type &) const
			{
				// Moving a trivial object means simply to copy it.
				copy_ctor()(tc,dest_begin,src_begin);
			}
			void impl(thread_control &tc, value_type *dest_begin, value_type *src_begin, const std::false_type &) const
			{
				size_type i = 0;
				try {
					allocator_type a;
					value_type *dest_current = dest_begin + tc.offset, *src_current = src_begin + tc.offset;
					for (; i < tc.work_size; ++i, ++dest_current, ++src_current) {
						a.construct(dest_current,piranha_move_if_noexcept(*src_current));
					}
				} catch (...) {
					allocator_type a;
					value_type *dest_current = dest_begin + tc.offset;
					for (size_type j = 0; j < i; ++j, ++dest_current) {
						a.destroy(dest_current);
					}
					std::lock_guard<std::mutex> lock(*tc.mutex);
					tc.exceptions->push_back(std::current_exception());
				}
			}
			void operator()(thread_control &tc, value_type *dest_begin, value_type *src_begin) const
			{
				impl(tc,dest_begin,src_begin,std::is_trivial<value_type>());
			}
		};
	public:
		/// Default constructor.
		/**
		 * Will build an empty vector.
		 */
		cvector():m_data(piranha_nullptr),m_size(0) {}
		/// Copy constructor.
		/**
		 * @param[in] other vector that will be copied.
		 */
		cvector(const cvector &other):m_data(piranha_nullptr),m_size(0)
		{
			if (other.size() == 0) {
				return;
			}
			allocator_type a;
			m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(other.size()));
			copy_ctor c;
			try {
				thread_runner(c,other.size(),m_data,other.m_data);
			} catch (...) {
				a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(other.size()));
				throw;
			}
			m_size = other.size();
		}
		/// Move constructor.
		/**
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
		 */
		explicit cvector(const size_type &size):m_data(piranha_nullptr),m_size(0)
		{
			if (size == 0) {
				return;
			}
			allocator_type a;
			m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(size));
			default_ctor f;
			try {
				thread_runner(f,size,m_data);
			} catch (...) {
				// Any construction has been rolled back, just de-allocate and re-throw.
				a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
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
			destroy_and_deallocate();
		}
		/// Move assignment operator.
		/**
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
		 */
		cvector &operator=(const cvector &other)
		{
			if (this == boost::addressof(other)) {
				return *this;
			}
			if (other.size() == 0) {
				resize(0);
			} else {
				allocator_type a;
				value_type *new_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(other.size()));
				copy_ctor c;
				try {
					thread_runner(c,other.size(),new_data,other.m_data);
				} catch (...) {
					a.deallocate(new_data,boost::numeric_cast<typename allocator_type::size_type>(other.size()));
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
		/// Size getter.
		/**
		 * @return the size of the vector.
		 */
		size_type size() const
		{
			return m_size;
		}
		/// Resize.
		/**
		 * Resize the vector preserving the existing content.
		 * 
		 * @param[in] size new size.
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
			allocator_type a;
			// Allocate the new data.
			value_type *new_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(size));
			// Default-construct the new data, if any.
			if (size > m_size) {
				default_ctor f;
				try {
					thread_runner(f,size - m_size,new_data + m_size);
				} catch (...) {
					// Default constructions have been rolled back, deallocate and throw.
					a.deallocate(new_data,boost::numeric_cast<typename allocator_type::size_type>(size));
					throw;
				}
			}
			// Move the old data, to the minimum of old and new size.
			mover m;
			try {
				thread_runner(m,std::min<size_type>(m_size,size),new_data,m_data);
			} catch (...) {
				// We reach here only if we did not actually move, but copied. The copied
				// objects have been rolled back by the mover, now we need to destroy the default
				// constructed objects from above (if any) and deallocate the new data.
				if (size > m_size) {
					destructor d;
					thread_runner(d,size - m_size,new_data + m_size);
				}
				allocator_type a;
				a.deallocate(new_data,boost::numeric_cast<typename allocator_type::size_type>(size));
				throw;
			}
			// Destroy and deallocate old data.
			destroy_and_deallocate();
			// Final assignment of new data.
			m_data = new_data;
			m_size = size;
		}
	private:
		void destroy_and_deallocate()
		{
			if (m_size == 0) {
				return;
			}
			destructor d;
			thread_runner(d,m_size,m_data);
			allocator_type a;
			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
		}
	private:
		value_type	*m_data;
		size_type	m_size;
};

}

#endif
