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

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "config.hpp"
#include "gmp_memory.hpp"

namespace piranha
{
namespace detail
{

void *gmp_allocate_function(std::size_t alloc_size)
{
	if (unlikely(alloc_size == boost::integer_traits<std::size_t>::const_max)) {
		std::cout << "GMP memory allocation failed.\n";
		std::exit(1);
	}
	void *retval = std::malloc(alloc_size + 1u);
	if (unlikely(!retval)) {
		std::cout << "GMP memory allocation failed.\n";
		std::exit(1);
	}
	// Set the tag byte.
	static_cast<char *>(retval)[alloc_size] = 0;
	return retval;
}

void gmp_free_function(void *ptr, std::size_t size)
{
	auto char_ptr = static_cast<char *>(ptr);
	if (char_ptr[size]) {
		char_ptr[size] = 0;
	} else {
		std::free(ptr);
	}
}

void *gmp_reallocate_function(void *ptr, std::size_t old_size, std::size_t new_size)
{
	if (unlikely(old_size == new_size)) {
		return ptr;
	}
	// This will already have the tag byte set properly.
	void *new_ptr = gmp_allocate_function(new_size);
	// Copy old content.
	std::memcpy(new_ptr,ptr,std::min<std::size_t>(old_size,new_size));
	// Clear old pointer, taking into consideration tag byte.
	gmp_free_function(ptr,old_size);
	return new_ptr;
}

}
}
