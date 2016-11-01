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

#ifndef PIRANHA_DETAIL_ATOMIC_FLAG_ARRAY_HPP
#define PIRANHA_DETAIL_ATOMIC_FLAG_ARRAY_HPP

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
struct atomic_flag_array {
    using value_type = std::atomic_flag;
    // This constructor will init all the flags in the array to false.
    explicit atomic_flag_array(const std::size_t &size) : m_ptr(nullptr), m_size(size)
    {
        if (unlikely(size > std::numeric_limits<std::size_t>::max() / sizeof(value_type))) {
            piranha_throw(std::bad_alloc, );
        }
        // NOTE: this throws if allocation fails, after this line everything is noexcept.
        m_ptr = static_cast<value_type *>(aligned_palloc(0u, size * sizeof(value_type)));
        for (std::size_t i = 0u; i < m_size; ++i) {
            // NOTE: atomic_flag should support aggregate init syntax:
            // http://en.cppreference.com/w/cpp/atomic/atomic
            // But it results in warnings, let's avoid initialisation
            // via ctor and just set the flag to false later.
            ::new (static_cast<void *>(m_ptr + i)) value_type;
            (m_ptr + i)->clear();
        }
    }
    ~atomic_flag_array()
    {
        // atomic_flag is guaranteed to have a trivial dtor,
        // so we can just free the storage:
        // http://en.cppreference.com/w/cpp/atomic/atomic
        aligned_pfree(0u, static_cast<void *>(m_ptr));
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
    value_type *m_ptr;
    const std::size_t m_size;
};
}
}

#endif
