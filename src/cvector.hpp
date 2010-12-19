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
#include <cstddef>
#include <exception>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "config.hpp" // For piranha_assert.
#include "runtime_info.hpp"
#include "settings.hpp"
#include "thread_group.hpp"

// TODO: remove.
#include <iostream>

namespace piranha {

/// Concurrent vector class.
/**
 * This class is a minimal vector class which can use multiple threads during construction,
 * destruction and resize.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo Affinity settings.
 */
template <class T, class Allocator = std::allocator<T>>
class cvector
{
	public:
		/// Value type.
		typedef T value_type;
		/// Size type.
		typedef std::size_t size_type;
	private:
		// Allocator type.
		typedef typename Allocator::template rebind<value_type>::other allocator_type;
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
// std::cout << "n_threads: " << n_threads << '\n';
			std::mutex mutex;
			std::vector<std::exception_ptr> exceptions;
			// If we have just one thread or we are already being called by a thread separate from
			// the main one, do not open new threads.
			if (n_threads == 1 || std::this_thread::get_id() != runtime_info::get_main_thread_id()) {
				f(size,0,1,&mutex,&exceptions,std::forward<Args>(params)...);
			} else {
				thread_group tg;
				size_type i = 0;
				for (; i < n_threads - static_cast<size_type>(1); ++i) {
					tg.create_thread(std::forward<Functor>(f),work_size,i * work_size,n_threads,&mutex,&exceptions,std::forward<Args>(params)...);
				}
				// Last thread might have more work to do.
				tg.create_thread(std::forward<Functor>(f),size - work_size * i,i * work_size,n_threads,&mutex,&exceptions,std::forward<Args>(params)...);
				tg.join_all();
			}
			// Rethrow the first exception encountered.
			for (std::vector<std::exception_ptr>::size_type i = 0; i < exceptions.size(); ++i) {
				if (exceptions[i] != piranha_nullptr) {
// std::cout << "gonna rethrow\n";
					std::rethrow_exception(exceptions[i]);
				}
			}
			
		}
		struct default_ctor
		{
			// Trivial construction via assignment.
			void impl(const size_type &work_size, const size_type &offset, const size_type &,
				std::mutex *mutex, std::vector<std::exception_ptr> *exceptions, value_type *begin, const std::true_type &) const
			{
// std::cout << "trivial!\n";
				try {
					const value_type tmp = value_type();
					value_type *current = begin + offset;
					for (size_type i = 0; i < work_size; ++i, ++current) {
						*current = tmp;
					}
				} catch (...) {
					// No need to do any rolling back for trivial objects. Just store the exception.
					std::lock_guard<std::mutex> lock(*mutex);
					exceptions->push_back(std::current_exception());
				}
			}
			// Non-trivial construction.
			void impl(const size_type &work_size, const size_type &offset, const size_type &,
				std::mutex *mutex, std::vector<std::exception_ptr> *exceptions, value_type *begin, const std::false_type &) const
			{
// std::cout << "nontrivial!\n";
				size_type i = 0;
				try {
					allocator_type a;
					value_type *current = begin + offset;
					const value_type tmp = value_type();
					for (; i < work_size; ++i, ++current) {
						a.construct(current,tmp);
					}
				} catch (...) {
// std::cout << "rolling back\n";
					allocator_type a;
					value_type *current = begin + offset;
					// Roll back the construction.
					for (size_type j = 0; j < i; ++j, ++current) {
						a.destroy(current);
					}
					// Store the exception.
					std::lock_guard<std::mutex> lock(*mutex);
					exceptions->push_back(std::current_exception());
				}
			}
			void operator()(const size_type &work_size, const size_type &offset, const size_type &n_threads,
				std::mutex *mutex, std::vector<std::exception_ptr> *exceptions, value_type *begin) const
			{
				// NOTE: replace with is_trivially_copyable? At the moment it seems not to be present
				// in GCC 4.5.
				impl(work_size,offset,n_threads,mutex,exceptions,begin,std::is_trivial<value_type>());
			}
		};
		struct destructor
		{
			// Trivial destructor: do not do anything, simple deallocation is fine.
			void impl(const size_type &, const size_type &, const size_type &,
				std::mutex *, std::vector<std::exception_ptr> *, value_type *, const std::true_type &) const
			{}
			void impl(const size_type &work_size, const size_type &offset, const size_type &,
				std::mutex *, std::vector<std::exception_ptr> *, value_type *begin, const std::false_type &) const
			{
				allocator_type a;
				value_type *current = begin + offset;
				for (size_type i = 0; i < work_size; ++i, ++current) {
					a.destroy(current);
				}
			}
			void operator()(const size_type &work_size, const size_type &offset, const size_type &n_threads,
				std::mutex *mutex, std::vector<std::exception_ptr> *exceptions, value_type *begin) const
			{
				impl(work_size,offset,n_threads,mutex,exceptions,begin,std::is_trivial<value_type>());
			}
		};
		struct copy_ctor
		{
			// Trivial copy construction via memcpy.
			void impl(const size_type &work_size, const size_type &offset, const size_type &,
				std::mutex *, std::vector<std::exception_ptr> *, value_type *dest_begin, const value_type *src_begin, const std::true_type &) const
			{
				std::memcpy(static_cast<void *>(dest_begin + offset),static_cast<void const *>(src_begin + offset),sizeof(value_type) * work_size);
			}
			// Non-trivial copy construction.
			void impl(const size_type &work_size, const size_type &offset, const size_type &,
				std::mutex *mutex, std::vector<std::exception_ptr> *exceptions, value_type *dest_begin, const value_type *src_begin, const std::false_type &) const
			{
				size_type i = 0;
				try {
					allocator_type a;
					value_type *dest_current = dest_begin + offset;
					value_type const *src_current = src_begin + offset;
					for (; i < work_size; ++i, ++dest_current, ++src_current) {
						a.construct(dest_current,*src_current);
					}
				} catch (...) {
					// Roll back.
					allocator_type a;
					value_type *dest_current = dest_begin + offset;
					for (size_type j = 0; j < i; ++j, ++dest_current) {
						a.destroy(dest_current);
					}
					// Store the exception.
					std::lock_guard<std::mutex> lock(*mutex);
					exceptions->push_back(std::current_exception());
				}
			}
			void operator()(const size_type &work_size, const size_type &offset, const size_type &n_threads,
				std::mutex *mutex, std::vector<std::exception_ptr> *exceptions, value_type *dest_begin, const value_type *src_begin) const
			{
				impl(work_size,offset,n_threads,mutex,exceptions,dest_begin,src_begin,std::is_trivial<value_type>());
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
		cvector(cvector &&other) piranha_noexcept:m_data(other.m_data),m_size(other.m_size)
		{
// std::cout << "Move construct!\n";
			// Empty the other cvector.
			other.m_data = piranha_nullptr;
			other.m_size = 0;
		}
		/// Move assignment.
		/**
		 * @param[in] other vector that will be moved.
		 * 
		 * @return reference to this.
		 */
		cvector &operator=(cvector &&other) piranha_noexcept
		{
// std::cout << "Move assign!\n";
			destroy_and_deallocate();
			m_data = other.m_data;
			m_size = other.m_size;
			// Empty other.
			other.m_data = piranha_nullptr;
			other.m_size = 0;
			return *this;
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
		/// Size getter.
		/**
		 * @return the size of the vector.
		 */
		size_type size() const
		{
			return m_size;
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
		
		
		
		
		
		
		
		
		
		
// 		template <class Functor, class Value>
// 		static void executor(Functor &f, Value *begin, const size_type &size)
// 		{
// 			size_type nthread = boost::numeric_cast<size_type>(settings::get_nthread());
// 			while (size / nthread < 50) {
// 				nthread /= 2;
// 				if (nthread <= 1) {
// 					nthread = 1;
// 					break;
// 				}
// 			}
// 			piranha_assert(nthread >= 1);
// 			if (!ForceSingleThread && nthread > 1) {
// 				boost::thread_group tg;
// 				const size_type sub_size = size / nthread;
// 				size_type offset = 0;
// 				for (size_type i = 0; i < nthread - static_cast<size_type>(1); ++i) {
// 					tg.create_thread(boost::bind(f + offset, begin + offset, sub_size));
// 					offset += sub_size;
// 				}
// 				tg.create_thread(boost::bind(f + offset, begin + offset, size - offset));
// 				tg.join_all();
// 			} else {
// 				f(begin,size);
// 			}
// 		}
// 		template <class Derived>
// 		struct base_functor
// 		{
// 			typedef void result_type;
// 			Derived operator+(const size_type &) const
// 			{
// 				return *(static_cast<Derived const *>(this));
// 			}
// 		};
// 		struct default_ctor: base_functor<default_ctor>
// 		{
// 			// Trivial construction via assignment.
// 			void impl(value_type *begin, const size_type &size, const boost::true_type &) const
// 			{
// 				const value_type tmp = value_type();
// 				for (size_type i = 0; i < size; ++i, ++begin) {
// 					*begin = tmp;
// 				}
// 			}
// 			// Non-trivial construction.
// 			void impl(value_type *begin, const size_type &size, const boost::false_type &) const
// 			{
// 				allocator_type a;
// 				const value_type tmp = value_type();
// 				for (size_type i = 0; i < size; ++i, ++begin) {
// 					a.construct(begin,tmp);
// 				}
// 			}
// 			void operator()(value_type *begin, const size_type &size) const
// 			{
// 				impl(begin,size,boost::is_pod<value_type>());
// 			}
// 		};
// 		struct copy_ctor: base_functor<copy_ctor>
// 		{
// 			copy_ctor(const value_type *start):m_start(start) {}
// 			copy_ctor operator+(const size_type &offset) const
// 			{
// 				copy_ctor retval(*this);
// 				retval.m_start += offset;
// 				return retval;
// 			}
// 			// Trivial copy construction via memcpy.
// 			void impl(value_type *begin, const size_type &size, const boost::true_type &)
// 			{
// 				std::memcpy(static_cast<void *>(begin),static_cast<void const *>(m_start),sizeof(value_type) * size);
// 			}
// 			// Non-trivial copy construction.
// 			void impl(value_type *begin, const size_type &size, const boost::false_type &)
// 			{
// 				allocator_type a;
// 				for (size_type i = 0; i < size; ++i, ++begin) {
// 					a.construct(begin,*m_start);
// 					++m_start;
// 				}
// 			}
// 			void operator()(value_type *begin, const size_type &size)
// 			{
// 				impl(begin,size,boost::is_pod<value_type>());
// 			}
// 			value_type const *m_start;
// 		};
// 		struct assigner: base_functor<assigner>
// 		{
// 			assigner(const value_type *start):m_start(start) {}
// 			assigner operator+(const size_type &offset) const
// 			{
// 				assigner retval(*this);
// 				retval.m_start += offset;
// 				return retval;
// 			}
// 			// Trivial assignment via memcpy.
// 			void impl(value_type *begin, const size_type &size, const boost::true_type &)
// 			{
// 				std::memcpy(static_cast<void *>(begin),static_cast<void const *>(m_start),sizeof(value_type) * size);
// 			}
// 			// Non-trivial assignment.
// 			void impl(value_type *begin, const size_type &size, const boost::false_type &)
// 			{
// 				for (size_type i = 0; i < size; ++i, ++begin) {
// 					*begin = *m_start;
// 					++m_start;
// 				}
// 			}
// 			void operator()(value_type *begin, const size_type &size)
// 			{
// 				impl(begin,size,boost::is_pod<value_type>());
// 			}
// 			value_type const *m_start;
// 		};
// 		struct destructor: base_functor<destructor>
// 		{
// 			// Trivial destructor.
// 			void impl(value_type *, const size_type &, const boost::true_type &) const
// 			{}
// 			// Non-trivial destructor.
// 			void impl(value_type *begin, const size_type &size, const boost::false_type &) const
// 			{
// 				allocator_type a;
// 				for (size_type i = 0; i < size; ++i, ++begin) {
// 					a.destroy(begin);
// 				}
// 			}
// 			void operator()(value_type *begin, const size_type &size) const
// 			{
// 				impl(begin,size,boost::is_pod<value_type>());
// 			}
// 		};
// 	public:
// 		typedef value_type * iterator;
// 		typedef value_type const * const_iterator;
// 		cvector(): m_data(0),m_size(0) {}
// 		explicit cvector(const size_type &size)
// 		{
// 			allocator_type a;
// 			m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(size));
// 			default_ctor f;
// 			executor(f,m_data,size);
// 			m_size = size;
// 		}
// 		cvector(const cvector &v)
// 		{
// 			allocator_type a;
// 			m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(v.m_size));
// 			copy_ctor c(v.m_data);
// 			executor(c,m_data,v.m_size);
// 			m_size = v.m_size;
// 		}
// 		cvector &operator=(const cvector &v)
// 		{
// 			if (this == &v) {
// 				return *this;
// 			}
// 			if (m_size == v.m_size) {
// 				// If the two sizes match, we can just assign member by member.
// 				assigner a(v.m_data);
// 				executor(a,m_data,m_size);
// 			} else {
// 				// If sizes do not match, destroy and copy-rebuild.
// 				allocator_type a;
// 				// Destroy old data.
// 				destructor d;
// 				executor(d,m_data,m_size);
// 				// Deallocate and allocate new space.
// 				a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
// 				m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(v.m_size));
// 				// Copy over data from v into the new space.
// 				copy_ctor c(v.m_data);
// 				executor(c,m_data,v.m_size);
// 				m_size = v.m_size;
// 			}
// 			return *this;
// 		}
// 		~cvector()
// 		{
// 			destructor d;
// 			executor(d,m_data,m_size);
// 			allocator_type a;
// 			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
// 		}
// 		const_iterator begin() const
// 		{
// 			return m_data;
// 		}
// 		iterator begin()
// 		{
// 			return m_data;
// 		}
// 		const_iterator end() const
// 		{
// 			return m_data + m_size;
// 		}
// 		iterator end()
// 		{
// 			return m_data + m_size;
// 		}
// 		void swap(cvector &v)
// 		{
// 			std::swap(m_data,v.m_data);
// 			std::swap(m_size,v.m_size);
// 		}
// 		void resize(const size_type &new_size)
// 		{
// 			if (m_size == new_size) {
// 				return;
// 			}
// 			allocator_type a;
// 			// Allocate the new data.
// 			value_type *new_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(new_size));
// 			// Copy over old data, to the minimum of old and new sizes.
// 			copy_ctor c(m_data);
// 			executor(c,new_data,std::min<size_type>(m_size,new_size));
// 			if (new_size > m_size) {
// 				// Default construct remaining data, if any.
// 				default_ctor f;
// 				executor(f,new_data + m_size, new_size - m_size);
// 			}
// 			// Destroy and deallocate old data.
// 			destructor d;
// 			executor(d,m_data,m_size);
// 			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
// 			// Final assignment.
// 			m_data = new_data;
// 			m_size = new_size;
// 		}
// 		value_type &operator[](const size_type &n)
// 		{
// 			return m_data[n];
// 		}
// 		const value_type &operator[](const size_type &n) const
// 		{
// 			return m_data[n];
// 		}
// 		size_type size() const
// 		{
// 			return m_size;
// 		}
// 		void clear()
// 		{
// 			destructor d;
// 			executor(d,m_data,m_size);
// 			allocator_type a;
// 			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
// 			m_data = 0;
// 			m_size = 0;
// 		}
// 	private:
// 		value_type	*m_data;
// 		size_type	m_size;
};

}

#endif
