/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>

namespace piranha
{

namespace detail
{

// A simple RAII holder for an array of atomic flags. Bare minimum functionality.
struct atomic_flag_array {
    using value_type = std::atomic_flag;
    // This constructor will init all the flags in the array to false.
    explicit atomic_flag_array(const std::size_t &size) : m_size(size)
    {
        if (unlikely(size > std::numeric_limits<std::size_t>::max() / sizeof(value_type))) {
            piranha_throw(std::bad_alloc, );
        }
        // Dynamically create an array of unsigned char with enough storage.
        // This will throw bad_alloc in case the memory cannot be allocated.
        // NOTE: this is required to return memory sufficiently aligned for any type
        // which does not have extended alignment requirements:
        // https://stackoverflow.com/questions/10587879/does-new-char-actually-guarantee-aligned-memory-for-a-class-type
        m_ptr.reset(::new unsigned char[size * sizeof(value_type)]);
        // Now we use the unsigned char buffer to provide storage for the atomic flags. See:
        // http://eel.is/c++draft/intro.object
        // From now on, everything is noexcept.
        const auto end_ptr = m_ptr.get() + size * sizeof(value_type);
        for (auto ptr = m_ptr.get(); ptr != end_ptr; ptr += sizeof(value_type)) {
            // NOTE: atomic_flag should support aggregate init syntax:
            // http://en.cppreference.com/w/cpp/atomic/atomic
            // But it results in warnings, let's avoid initialisation
            // via ctor and just set the flag to false later.
            ::new (static_cast<void *>(ptr)) value_type;
            reinterpret_cast<value_type *>(ptr)->clear();
        }
    }
    // atomic_flag is guaranteed to have a trivial dtor,
    // so we can just let unique_ptr free the storage. No need
    // for custom dtor.
    // http://en.cppreference.com/w/cpp/atomic/atomic
    // Delete explicitly all other ctors/assignment operators.
    atomic_flag_array() = delete;
    atomic_flag_array(const atomic_flag_array &) = delete;
    atomic_flag_array(atomic_flag_array &&) = delete;
    atomic_flag_array &operator=(const atomic_flag_array &) = delete;
    atomic_flag_array &operator=(atomic_flag_array &&) = delete;
    value_type &operator[](const std::size_t &i)
    {
        return *reinterpret_cast<value_type *>(m_ptr.get() + sizeof(value_type) * i);
    }
    const value_type &operator[](const std::size_t &i) const
    {
        return *reinterpret_cast<const value_type *>(m_ptr.get() + sizeof(value_type) * i);
    }
    // Data members.
    std::unique_ptr<unsigned char[]> m_ptr;
    const std::size_t m_size;
};
}
}

#endif
