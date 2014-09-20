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
struct base_tuning
{
	static std::atomic<bool>	s_parallel_memory_set;
	static std::atomic<unsigned>	s_mult_block_size;
};

template <typename T>
std::atomic<bool> base_tuning<T>::s_parallel_memory_set(true);

template <typename T>
std::atomic<unsigned> base_tuning<T>::s_mult_block_size(256u);

}

/// Performance tuning.
/**
 * This class provides static methods to manipulate global variables useful for performance tuning.
 * All the methods in this class are thread-safe.
 */
class tuning: private detail::base_tuning<>
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
		static unsigned get_multiplication_block_size()
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
		static void set_multiplication_block_size(unsigned size)
		{
			if (unlikely(size < 16u || size > 4096u)) {
				piranha_throw(std::invalid_argument,"invalid block size");
			}
			s_mult_block_size.store(size);
		}

};

}

#endif
