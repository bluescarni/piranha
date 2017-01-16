/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_DETAIL_ATOMIC_LOCK_GUARD_HPP
#define PIRANHA_DETAIL_ATOMIC_LOCK_GUARD_HPP

#include <atomic>

namespace piranha
{

namespace detail
{

// A simple spinlock built on top of std::atomic_flag. See for reference:
// http://en.cppreference.com/w/cpp/atomic/atomic_flag
// http://stackoverflow.com/questions/26583433/c11-implementation-of-spinlock-using-atomic
// The memory order specification is to squeeze out some extra performance with respect to the
// default behaviour of atomic types.
struct atomic_lock_guard {
    explicit atomic_lock_guard(std::atomic_flag &af) : m_af(af)
    {
        while (m_af.test_and_set(std::memory_order_acquire)) {
        }
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
