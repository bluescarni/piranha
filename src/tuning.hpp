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

#ifndef PIRANHA_TUNING_HPP
#define PIRANHA_TUNING_HPP

#include <atomic>
#include <stdexcept>

#include "config.hpp"
#include "exceptions.hpp"
#include "runtime_info.hpp"

namespace piranha
{

namespace detail
{

template <typename = int>
struct base_tuning {
    static std::atomic<bool> s_parallel_memory_set;
    static std::atomic<unsigned long> s_mult_block_size;
    static std::atomic<unsigned long> s_estimate_threshold;
};

template <typename T>
std::atomic<bool> base_tuning<T>::s_parallel_memory_set(true);

template <typename T>
std::atomic<unsigned long> base_tuning<T>::s_mult_block_size(256u);

template <typename T>
std::atomic<unsigned long> base_tuning<T>::s_estimate_threshold(200u);
}

/// Performance tuning.
/**
 * This class provides static methods to manipulate global variables useful for performance tuning.
 * All the methods in this class are thread-safe.
 */
class tuning : private detail::base_tuning<>
{
public:
    /// Get the \p parallel_memory_set flag.
    /**
     * Piranha can use multiple threads when initialising large memory areas (e.g., during the initialisation
     * of the result of the multiplication of two large polynomials). This can improve performance on systems
     * with fast or multiple memory buses, but it could lead to degraded performance on slower/single-socket machines.
     *
     * The default value of this flag is \p true (i.e., Piranha will use multiple threads while initialising
     * large memory areas).
     *
     * @return current value of the \p parallel_memory_set flag.
     */
    static bool get_parallel_memory_set()
    {
        return s_parallel_memory_set.load();
    }
    /// Set the \p parallel_memory_set flag.
    /**
     * @see piranha::tuning::get_parallel_memory_set() for an explanation of the meaning of this flag.
     *
     * @param[in] flag desired value for the \p parallel_memory_set flag.
     */
    static void set_parallel_memory_set(bool flag)
    {
        s_parallel_memory_set.store(flag);
    }
    /// Reset the \p parallel_memory_set flag.
    /**
     * This method will reset the \p parallel_memory_set flag to its default value.
     *
     * @see piranha::tuning::get_parallel_memory_set() for an explanation of the meaning of this flag.
     */
    static void reset_parallel_memory_set()
    {
        s_parallel_memory_set.store(true);
    }
    /// Get the multiplication block size.
    /**
     * The multiplication algorithms for certain series types (e.g., polynomials) divide the input operands in
     * blocks before processing them. This flag regulates the maximum size of these blocks.
     *
     * Larger block have less overhead, but can degrade the performance of memory access. Smaller blocks can promote
     * faster memory access but can also incur in larger overhead.
     *
     * The default value of this flag is 256.
     *
     * @return the block size used in some series multiplication routines.
     */
    static unsigned long get_multiplication_block_size()
    {
        return s_mult_block_size.load();
    }
    /// Set the multiplication block size.
    /**
     * @see piranha::tuning::get_multiplication_block_size() for an explanation of the meaning of this value.
     *
     * @param[in] size desired value for the block size.
     *
     * @throws std::invalid_argument if \p size is outside an implementation-defined range.
     */
    static void set_multiplication_block_size(unsigned long size)
    {
        if (unlikely(size < 16u || size > 4096u)) {
            piranha_throw(std::invalid_argument, "invalid block size");
        }
        s_mult_block_size.store(size);
    }
    /// Reset the multiplication block size.
    /**
     * This method will reset the multiplication block size to its default value.
     *
     * @see piranha::tuning::get_multiplication_block_size() for an explanation of the meaning of this value.
     */
    static void reset_multiplication_block_size()
    {
        s_mult_block_size.store(256u);
    }
    /// Get the series estimation threshold.
    /**
     * In series multiplication it can be advantageous to employ a heuristic to estimate the final size
     * of the result before actually performing the multiplication. The cost of estimation is proportionally
     * larger for small operands, and it can result in noticeable overhead for small multiplications.
     * This flag establishes a threshold below which the estimation of the size of the product of a series
     * multiplication will not be performed.
     *
     * The precise way in which this value is used depends on the multiplication algorithm.
     * The default value of this flag is 200.
     *
     * @return the series estimation threshold.
     */
    static unsigned long get_estimate_threshold()
    {
        return s_estimate_threshold.load();
    }
    /// Set the series estimation threshold.
    /**
     * @see piranha::tuning::s_estimate_threshold() for an explanation of the meaning of this value.
     *
     * @param[in] size desired value for the series estimation threshold.
     */
    static void set_estimate_threshold(unsigned long size)
    {
        s_estimate_threshold.store(size);
    }
    /// Reset the series estimation threshold.
    /**
     * This method will reset the series estimation threshold to its default value.
     *
     * @see piranha::tuning::get_estimate_threshold() for an explanation of the meaning of this value.
     */
    static void reset_estimate_threshold()
    {
        s_estimate_threshold.store(200u);
    }
};
}

#endif
