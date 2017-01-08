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

#ifndef PIRANHA_DYNAMIC_ALIGNING_ALLOCATOR_HPP
#define PIRANHA_DYNAMIC_ALIGNING_ALLOCATOR_HPP

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

#include "config.hpp"
#include "memory.hpp"

namespace piranha
{

/// Memory allocator with runtime alignment support.
/**
 * This allocator can be used to allocate memory aligned to a specific boundary, specified at runtime.
 * The alignment value is provided upon construction and it is stored as a member of the allocator object.
 * The allocator is standard-compliant and can hence be used in standard containers.
 */
template <typename T>
class dynamic_aligning_allocator
{
public:
    /// Value type of the allocator.
    /**
     * Alias for \p T.
     */
    using value_type = T;
    /// Size type.
    using size_type = std::size_t;
    /// Move assignment propagation.
    /**
     * This allocator has to be move assigned when the container using it is move assigned.
     */
    using propagate_on_container_move_assignment = std::true_type;
    /// Default constructor.
    /**
     * Will set the internal alignment value to zero.
     */
    dynamic_aligning_allocator() : m_alignment(0)
    {
    }
    /// Defaulted copy constructor.
    dynamic_aligning_allocator(const dynamic_aligning_allocator &) = default;
    /// Defaulted move constructor.
    dynamic_aligning_allocator(dynamic_aligning_allocator &&) = default;
    /// Constructor from alignment value.
    /**
     * @param[in] alignment alignment value that will be used for allocation.
     */
    explicit dynamic_aligning_allocator(const std::size_t &alignment) : m_alignment(alignment)
    {
    }
    /// Converting copy constructor.
    /**
     * After construction, the alignment will be the same as \p other.
     *
     * @param[in] other construction argument.
     */
    template <typename U>
    explicit dynamic_aligning_allocator(const dynamic_aligning_allocator<U> &other) : m_alignment(other.alignment())
    {
    }
    /// Converting move constructor.
    /**
     * After construction, the alignment will be the same as \p other.
     *
     * @param[in] other construction argument.
     */
    template <typename U>
    explicit dynamic_aligning_allocator(dynamic_aligning_allocator<U> &&other) : m_alignment(other.alignment())
    {
    }
    /// Defaulted destructor.
    ~dynamic_aligning_allocator() = default;
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    dynamic_aligning_allocator &operator=(const dynamic_aligning_allocator &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    dynamic_aligning_allocator &operator=(dynamic_aligning_allocator &&other) = default;
    /// Maximum allocatable size.
    /**
     * @return the maximum number of objects of type \p value_type that can be allocated by a single call to
     * allocate().
     */
    constexpr size_type max_size() const
    {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }
    /// Allocation function.
    /**
     * The allocation function is a thin wrapper around piranha::aligned_palloc().
     *
     * @param[in] size number of instances of type \p value_type for which the space will be allocated.
     *
     * @return a pointer to the allocated storage.
     *
     * @throws std::bad_alloc if \p size is larger than max_size().
     * @throws unspecified any exception thrown by piranha::aligned_palloc().
     */
    value_type *allocate(const size_type &size) const
    {
        if (unlikely(size > max_size())) {
            piranha_throw(std::bad_alloc, );
        }
        return static_cast<value_type *>(
            aligned_palloc(m_alignment, static_cast<size_type>(size * sizeof(value_type))));
    }
    /// Deallocation function.
    /**
     * The allocation function is a thin wrapper around piranha::aligned_pfree().
     *
     * @param[in] ptr a pointer to a memory block allocated via allocate().
     *
     * @throws unspecified any exception thrown by piranha::aligned_pfree().
     */
    void deallocate(value_type *ptr, const size_type &) const
    {
        aligned_pfree(m_alignment, ptr);
    }
    /// Equality operator.
    /**
     * @param[in] other comparison argument.
     *
     * @return \p true if the alignments of \p this and \p other coincide, \p false otherwise.
     */
    bool operator==(const dynamic_aligning_allocator &other) const
    {
        return m_alignment == other.m_alignment;
    }
    /// Inequality operator.
    /**
     * @param[in] other comparison argument.
     *
     * @return the opposite of operator==().
     */
    bool operator!=(const dynamic_aligning_allocator &other) const
    {
        return !(this->operator==(other));
    }
    /// Alignment getter.
    /**
     * @return the alignment value used for construction.
     */
    std::size_t alignment() const
    {
        return m_alignment;
    }

private:
    std::size_t m_alignment;
};
}

#endif
