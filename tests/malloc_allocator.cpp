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

#include "../src/malloc_allocator.hpp"

#define BOOST_TEST_MODULE malloc_allocator_test
#include <boost/test/unit_test.hpp>

#include <boost/integer_traits.hpp>
#include <cstddef>
#include <new>

#include "../src/config.hpp"
#include "../src/integer.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(malloc_allocator_unaligned_test)
{
	malloc_allocator<char> a;
	auto ptr = a.allocate(0);
	BOOST_CHECK(ptr == piranha_nullptr);
	BOOST_CHECK_NO_THROW(a.deallocate(ptr,0));
	ptr = a.allocate(1);
	BOOST_CHECK(ptr != piranha_nullptr);
	BOOST_CHECK_NO_THROW(a.deallocate(ptr,0));
	malloc_allocator<char[2]> b;
	BOOST_CHECK_THROW(b.allocate(boost::integer_traits<malloc_allocator<char[2]>::size_type>::const_max),std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(malloc_allocator_construction_test)
{
	malloc_allocator<integer> a;
	auto ptr = a.allocate(1);
	integer tmp(1);
	a.construct(ptr,tmp);
	BOOST_CHECK_EQUAL(*ptr,tmp);
	a.destroy(ptr);
	a.deallocate(ptr,0);
	ptr = a.allocate(1);
	a.construct(ptr,"1");
	BOOST_CHECK_EQUAL(*ptr,tmp);
	a.destroy(ptr);
	a.deallocate(ptr,0);
}

BOOST_AUTO_TEST_CASE(malloc_allocator_aligned_test)
{
	// We assume here that size of void * is a power of 2.
	static const std::size_t alignment = sizeof(void *) * sizeof(void *);
	malloc_allocator<char,alignment> a;
	auto ptr = a.allocate(0);
	BOOST_CHECK(ptr == piranha_nullptr);
	BOOST_CHECK_NO_THROW(a.deallocate(ptr,0));
	ptr = a.allocate(1);
	BOOST_CHECK(ptr != piranha_nullptr);
	BOOST_CHECK(!((std::uintptr_t)ptr % alignment));
	BOOST_CHECK_NO_THROW(a.deallocate(ptr,0));
}
