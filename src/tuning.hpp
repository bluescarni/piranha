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

#include "runtime_info.hpp"

namespace piranha
{

namespace detail
{

template <typename = int>
struct base_tuning
{
	static std::atomic<bool> s_parallel_memory_set;
};

template <typename T>
std::atomic<bool> base_tuning<T>::s_parallel_memory_set(true);

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
		static bool get_parallel_memory_set() noexcept
		{
			return s_parallel_memory_set.load();
		}
		/// Set the \p parallel_memory_set flag.
		/**
		 * @see piranha::tuning::get_parallel_memory_set() for an explanation of the meaning of this flag.
		 *
		 * @param[in] flag desired value for the \p parallel_memory_set flag.
		 */
		static void set_parallel_memory_set(bool flag) noexcept
		{
			s_parallel_memory_set.store(flag);
		}

};

}

#endif
