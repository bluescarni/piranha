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

#ifndef PIRANHA_CACHE_ALIGNING_ALLOCATOR_HPP
#define PIRANHA_CACHE_ALIGNING_ALLOCATOR_HPP

#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>

#include "malloc_allocator.hpp"
#include "settings.hpp"

namespace piranha
{

/// Allocator that tries to align memory to the cache line size.
/**
 * This allocator will try to allocate memory aligned to the cache line size (as reported by piranha::settings).
 * If the cache line size cannot be detected, or if it is incompatible with the alignment requirements on the platform,
 * or if no memory aligning primitives are available, memory will be allocated via <tt>std::malloc()</tt>.
 * 
 * Exception safety and move semantics are equivalent to piranha::malloc_allocator.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T>
class cache_aligning_allocator: public malloc_allocator<T>
{
		typedef malloc_allocator<T> base;
		static std::size_t determine_alignment()
		{
			try {
				const std::size_t alignment = boost::numeric_cast<std::size_t>(settings::get_cache_line_size());
				base::check_alignment(alignment);
				return alignment;
			} catch (...) {
				return 0u;
			}
		}
	public:
		// Typedefs from base are good enough.
		/// Allocator rebinding.
		template <typename U>
		struct rebind
		{
			/// Rebound allocator type.
			typedef cache_aligning_allocator<U> other;
		};
		/// Default constructor.
		/**
		 * Will invoke the base constructor with an alignment argument that is either 0,
		 * or the cache line size on the host platform - if such value is suitable as alignment value
		 * in piranha::malloc_allocator. In other words, construction is always guaranteed to be
		 * successful.
		 */
		cache_aligning_allocator():base(determine_alignment()) {}
};

}

#endif
