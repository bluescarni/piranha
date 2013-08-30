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

#include "../src/aligned_memory.hpp"

#define BOOST_TEST_MODULE aligned_memory_test
#include <boost/test/unit_test.hpp>

#include <new>

#include "../src/config.hpp"
#include "../src/environment.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(aligned_memory_aligned_malloc_test)
{
	environment env;
	// Test the common codepath.
	auto ptr = aligned_palloc(0,0);
	BOOST_CHECK(ptr == nullptr);
	BOOST_CHECK_NO_THROW(aligned_pfree(0,ptr));
	ptr = aligned_palloc(123,0);
	BOOST_CHECK(ptr == nullptr);
	BOOST_CHECK_NO_THROW(aligned_pfree(123,ptr));
	ptr = aligned_palloc(0,1);
	BOOST_CHECK(ptr != nullptr);
	BOOST_CHECK_NO_THROW(aligned_pfree(0,ptr));
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
	// posix_memalign requires power of two and multiple of sizeof(void *) for alignment value.
	BOOST_CHECK_THROW(aligned_palloc(3,1),std::bad_alloc);
	BOOST_CHECK_THROW(aligned_palloc(7,1),std::bad_alloc);
	if (sizeof(void *) % alignof(int) == 0) {
		ptr = aligned_palloc(sizeof(void *),sizeof(int));
		BOOST_CHECK_NO_THROW(aligned_pfree(sizeof(void *),ptr));
	}
#endif
}
