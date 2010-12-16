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

// #include <algorithm>
// #include <boost/bind.hpp>
#include <boost/numeric/conversion/cast.hpp>
// #include <boost/thread.hpp>
// #include <boost/type_traits/integral_constant.hpp>
// #include <boost/type_traits/is_pod.hpp>
#include <cstddef>
// #include <cstring>
#include <memory>

// #include "exceptions.h"
#include "runtime_info.hpp"
#include "settings.hpp"

namespace piranha {

/// Concurrent vector class.
/**
 * This class is a minimal vector class which can use multiple threads during construction,
 * destruction and resize.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <class T, class Allocator = std::allocator<T>>
class cvector
{
	public:
		/// Value type.
		typedef T value_type;
		/// Size type.
		typedef std::size_t size_type;
		/// Allocator type.
		typedef typename Allocator::template rebind<value_type>::other allocator_type;
	private:
		template <class Functor, class Value>
		static void executor(Functor &f, Value *begin, const size_type &size)
		{
			size_type nthread = boost::numeric_cast<size_type>(settings::get_nthread());
			while (size / nthread < 50) {
				nthread /= 2;
				if (nthread <= 1) {
					nthread = 1;
					break;
				}
			}
			piranha_assert(nthread >= 1);
			if (!ForceSingleThread && nthread > 1) {
				boost::thread_group tg;
				const size_type sub_size = size / nthread;
				size_type offset = 0;
				for (size_type i = 0; i < nthread - static_cast<size_type>(1); ++i) {
					tg.create_thread(boost::bind(f + offset, begin + offset, sub_size));
					offset += sub_size;
				}
				tg.create_thread(boost::bind(f + offset, begin + offset, size - offset));
				tg.join_all();
			} else {
				f(begin,size);
			}
		}
		template <class Derived>
		struct base_functor
		{
			typedef void result_type;
			Derived operator+(const size_type &) const
			{
				return *(static_cast<Derived const *>(this));
			}
		};
		struct default_ctor: base_functor<default_ctor>
		{
			// Trivial construction via assignment.
			void impl(value_type *begin, const size_type &size, const boost::true_type &) const
			{
				const value_type tmp = value_type();
				for (size_type i = 0; i < size; ++i, ++begin) {
					*begin = tmp;
				}
			}
			// Non-trivial construction.
			void impl(value_type *begin, const size_type &size, const boost::false_type &) const
			{
				allocator_type a;
				const value_type tmp = value_type();
				for (size_type i = 0; i < size; ++i, ++begin) {
					a.construct(begin,tmp);
				}
			}
			void operator()(value_type *begin, const size_type &size) const
			{
				impl(begin,size,boost::is_pod<value_type>());
			}
		};
		struct copy_ctor: base_functor<copy_ctor>
		{
			copy_ctor(const value_type *start):m_start(start) {}
			copy_ctor operator+(const size_type &offset) const
			{
				copy_ctor retval(*this);
				retval.m_start += offset;
				return retval;
			}
			// Trivial copy construction via memcpy.
			void impl(value_type *begin, const size_type &size, const boost::true_type &)
			{
				std::memcpy(static_cast<void *>(begin),static_cast<void const *>(m_start),sizeof(value_type) * size);
			}
			// Non-trivial copy construction.
			void impl(value_type *begin, const size_type &size, const boost::false_type &)
			{
				allocator_type a;
				for (size_type i = 0; i < size; ++i, ++begin) {
					a.construct(begin,*m_start);
					++m_start;
				}
			}
			void operator()(value_type *begin, const size_type &size)
			{
				impl(begin,size,boost::is_pod<value_type>());
			}
			value_type const *m_start;
		};
		struct assigner: base_functor<assigner>
		{
			assigner(const value_type *start):m_start(start) {}
			assigner operator+(const size_type &offset) const
			{
				assigner retval(*this);
				retval.m_start += offset;
				return retval;
			}
			// Trivial assignment via memcpy.
			void impl(value_type *begin, const size_type &size, const boost::true_type &)
			{
				std::memcpy(static_cast<void *>(begin),static_cast<void const *>(m_start),sizeof(value_type) * size);
			}
			// Non-trivial assignment.
			void impl(value_type *begin, const size_type &size, const boost::false_type &)
			{
				for (size_type i = 0; i < size; ++i, ++begin) {
					*begin = *m_start;
					++m_start;
				}
			}
			void operator()(value_type *begin, const size_type &size)
			{
				impl(begin,size,boost::is_pod<value_type>());
			}
			value_type const *m_start;
		};
		struct destructor: base_functor<destructor>
		{
			// Trivial destructor.
			void impl(value_type *, const size_type &, const boost::true_type &) const
			{}
			// Non-trivial destructor.
			void impl(value_type *begin, const size_type &size, const boost::false_type &) const
			{
				allocator_type a;
				for (size_type i = 0; i < size; ++i, ++begin) {
					a.destroy(begin);
				}
			}
			void operator()(value_type *begin, const size_type &size) const
			{
				impl(begin,size,boost::is_pod<value_type>());
			}
		};
	public:
		typedef value_type * iterator;
		typedef value_type const * const_iterator;
		cvector(): m_data(0),m_size(0) {}
		explicit cvector(const size_type &size)
		{
			allocator_type a;
			m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(size));
			default_ctor f;
			executor(f,m_data,size);
			m_size = size;
		}
		cvector(const cvector &v)
		{
			allocator_type a;
			m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(v.m_size));
			copy_ctor c(v.m_data);
			executor(c,m_data,v.m_size);
			m_size = v.m_size;
		}
		cvector &operator=(const cvector &v)
		{
			if (this == &v) {
				return *this;
			}
			if (m_size == v.m_size) {
				// If the two sizes match, we can just assign member by member.
				assigner a(v.m_data);
				executor(a,m_data,m_size);
			} else {
				// If sizes do not match, destroy and copy-rebuild.
				allocator_type a;
				// Destroy old data.
				destructor d;
				executor(d,m_data,m_size);
				// Deallocate and allocate new space.
				a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
				m_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(v.m_size));
				// Copy over data from v into the new space.
				copy_ctor c(v.m_data);
				executor(c,m_data,v.m_size);
				m_size = v.m_size;
			}
			return *this;
		}
		~cvector()
		{
			destructor d;
			executor(d,m_data,m_size);
			allocator_type a;
			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
		}
		const_iterator begin() const
		{
			return m_data;
		}
		iterator begin()
		{
			return m_data;
		}
		const_iterator end() const
		{
			return m_data + m_size;
		}
		iterator end()
		{
			return m_data + m_size;
		}
		void swap(cvector &v)
		{
			std::swap(m_data,v.m_data);
			std::swap(m_size,v.m_size);
		}
		void resize(const size_type &new_size)
		{
			if (m_size == new_size) {
				return;
			}
			allocator_type a;
			// Allocate the new data.
			value_type *new_data = a.allocate(boost::numeric_cast<typename allocator_type::size_type>(new_size));
			// Copy over old data, to the minimum of old and new sizes.
			copy_ctor c(m_data);
			executor(c,new_data,std::min<size_type>(m_size,new_size));
			if (new_size > m_size) {
				// Default construct remaining data, if any.
				default_ctor f;
				executor(f,new_data + m_size, new_size - m_size);
			}
			// Destroy and deallocate old data.
			destructor d;
			executor(d,m_data,m_size);
			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
			// Final assignment.
			m_data = new_data;
			m_size = new_size;
		}
		value_type &operator[](const size_type &n)
		{
			return m_data[n];
		}
		const value_type &operator[](const size_type &n) const
		{
			return m_data[n];
		}
		size_type size() const
		{
			return m_size;
		}
		void clear()
		{
			destructor d;
			executor(d,m_data,m_size);
			allocator_type a;
			a.deallocate(m_data,boost::numeric_cast<typename allocator_type::size_type>(m_size));
			m_data = 0;
			m_size = 0;
		}
	private:
		value_type	*m_data;
		size_type	m_size;
};

}

#endif
