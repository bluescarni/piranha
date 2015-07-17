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

#ifndef PIRANHA_DETAIL_ATOMIC_UTILS_HPP
#define PIRANHA_DETAIL_ATOMIC_UTILS_HPP

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>

#include "../config.hpp"
#include "../exceptions.hpp"
#include "../memory.hpp"

namespace piranha
{

namespace detail
{

// A simple RAII holder for an array of atomic flags. Bare minimum functionality.
struct atomic_flag_array
{
	using value_type = std::atomic_flag;
	// This constructor will init all the flags in the array to false.
	explicit atomic_flag_array(const std::size_t &size):m_ptr(nullptr),m_size(size)
	{
		if (unlikely(size > std::numeric_limits<std::size_t>::max() / sizeof(value_type))) {
			piranha_throw(std::bad_alloc,);
		}
		// NOTE: this throws if allocation fails, after this line everything is noexcept.
		m_ptr = static_cast<value_type *>(aligned_palloc(0u,size * sizeof(value_type)));
		for (std::size_t i = 0u; i < m_size; ++i) {
			::new (static_cast<void *>(m_ptr + i)) value_type(ATOMIC_FLAG_INIT);
		}
	}
	~atomic_flag_array()
	{
		for (std::size_t i = 0u; i < m_size; ++i) {
			(m_ptr + i)->~value_type();
		}
		aligned_pfree(0u,static_cast<void *>(m_ptr));
	}
	// Delete explicitly all other ctors/assignment operators.
	atomic_flag_array() = delete;
	atomic_flag_array(const atomic_flag_array &) = delete;
	atomic_flag_array(atomic_flag_array &&) = delete;
	atomic_flag_array &operator=(const atomic_flag_array &) = delete;
	atomic_flag_array &operator=(atomic_flag_array &&) = delete;
	value_type &operator[](const std::size_t &i)
	{
		return *(m_ptr + i);
	}
	const value_type &operator[](const std::size_t &i) const
	{
		return *(m_ptr + i);
	}
	// Data members.
	value_type		*m_ptr;
	const std::size_t	m_size;
};

// A simple spinlock built on top of std::atomic_flag. See for reference:
// http://en.cppreference.com/w/cpp/atomic/atomic_flag
// http://stackoverflow.com/questions/26583433/c11-implementation-of-spinlock-using-atomic
// The memory order specification is to squeeze out some extra performance with respect to the
// default behaviour of atomic types.
struct atomic_lock_guard
{
	explicit atomic_lock_guard(std::atomic_flag &af):m_af(af)
	{
		while (m_af.test_and_set(std::memory_order_acquire)) {}
	}
	~atomic_lock_guard()
	{
		m_af.clear(std::memory_order_release);
	}
	// Delete explicitly all other ctors/assignment operators.
	atomic_lock_guard() = delete;
	atomic_lock_guard(const atomic_lock_guard &) = delete;
	atomic_lock_guard(atomic_lock_guard &&) = delete;
	atomic_lock_guard &operator=(const atomic_lock_guard &) = delete;
	atomic_lock_guard &operator=(atomic_lock_guard &&) = delete;
	// Data members.
	std::atomic_flag &m_af;
};

}

}

#endif
