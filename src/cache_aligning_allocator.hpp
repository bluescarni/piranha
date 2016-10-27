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

#ifndef PIRANHA_CACHE_ALIGNING_ALLOCATOR_HPP
#define PIRANHA_CACHE_ALIGNING_ALLOCATOR_HPP

#include <cstddef>
#include <utility>

#include "dynamic_aligning_allocator.hpp"
#include "memory.hpp"
#include "safe_cast.hpp"
#include "settings.hpp"

namespace piranha
{

/// Allocator that tries to align memory to the cache line size.
/**
 * This allocator will try to allocate memory aligned to the cache line size (as reported by piranha::settings).
 *
 * Exception safety and move semantics are equivalent to piranha::dynamic_aligning_allocator.
 */
template <typename T>
class cache_aligning_allocator : public dynamic_aligning_allocator<T>
{
    using base = dynamic_aligning_allocator<T>;
    static std::size_t determine_alignment()
    {
#if !defined(PIRANHA_HAVE_MEMORY_ALIGNMENT_PRIMITIVES)
        return 0u;
#endif
        try {
            const std::size_t alignment = safe_cast<std::size_t>(settings::get_cache_line_size());
            if (!alignment_check<T>(alignment)) {
                return 0u;
            }
            return alignment;
        } catch (...) {
            return 0u;
        }
    }

public:
    // NOTE: these members (down to the default ctor) are not needed according
    // to the C++11 standard, but GCC's stdlib does not support allocator aware
    // containers yet.
    /// Allocator rebind
    template <typename U>
    struct rebind {
        /// Allocator re-parametrised over \p U.
        using other = cache_aligning_allocator<U>;
    };
    /// Pointer type.
    using pointer = T *;
    /// Const pointer type.
    using const_pointer = T const *;
    /// Reference type.
    using reference = T &;
    /// Const reference type.
    using const_reference = T const &;
    /// Destructor method.
    /**
     * @param[in] p address of the object of type \p T to be destroyed.
     */
    void destroy(pointer p)
    {
        p->~T();
    }
    /// Variadic construction method.
    /**
     * @param[in] p address where the object will be constructed.
     * @param[in] args arguments that will be forwarded for construction.
     */
    // NOTE: here, according to the standard, the allocator must be
    // able to construct objects of arbitrary type:
    // http://en.cppreference.com/w/cpp/concept/Allocator
    template <typename U, typename... Args>
    void construct(U *p, Args &&... args)
    {
        ::new (static_cast<void *>(p)) U(std::forward<Args>(args)...);
    }
    /// Default constructor.
    /**
     * Will invoke the base constructor with an alignment value determined as follows:
     * - if no memory alignment primitives are available on the host platform, the value will be zero;
     * - if the cache line size reported by piranha::settings::get_cache_line_size() passes the checks
     *   performed by piranha::alignment_check() of \p T, it will be used as construction value;
     * - otherwise, zero will be used.
     */
    cache_aligning_allocator() : base(determine_alignment())
    {
    }
    /// Defaulted copy constructor.
    cache_aligning_allocator(const cache_aligning_allocator &) = default;
    /// Defaulted move constructor.
    cache_aligning_allocator(cache_aligning_allocator &&) = default;
    /// Copy-constructor from different instance.
    /**
     * Will forward the call to the corresponding constructor in piranha::dynamic_aligning_allocator.
     *
     * @param[in] other construction argument.
     */
    template <typename U>
    explicit cache_aligning_allocator(const cache_aligning_allocator<U> &other) : base(other)
    {
    }
    /// Move-constructor from different instance.
    /**
     * Will forward the call to the corresponding constructor in piranha::dynamic_aligning_allocator.
     *
     * @param[in] other construction argument.
     */
    template <typename U>
    explicit cache_aligning_allocator(cache_aligning_allocator<U> &&other) : base(std::move(other))
    {
    }
    /// Defaulted destructor.
    ~cache_aligning_allocator() = default;
    /// Defaulted copy assignment operator.
    cache_aligning_allocator &operator=(const cache_aligning_allocator &) = default;
    /// Defaulted move assignment operator.
    cache_aligning_allocator &operator=(cache_aligning_allocator &&) = default;
};
}

#endif
