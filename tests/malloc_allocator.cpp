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
#include <stdexcept>

#include "../src/environment.hpp"
#include "../src/integer.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(malloc_allocator_unaligned_test)
{
	environment env;
	malloc_allocator<char> a;
	auto ptr = a.allocate(0);
	BOOST_CHECK(ptr == nullptr);
	BOOST_CHECK_NO_THROW(a.deallocate(ptr,0));
	ptr = a.allocate(1);
	BOOST_CHECK(ptr != nullptr);
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
	if (malloc_allocator<char>::have_memalign_primitives && !(sizeof(void *) & (sizeof(void *) - 1u))) {
		malloc_allocator<char> a;
		malloc_allocator<char> b(sizeof(void *));
		b = a;
		BOOST_CHECK(b == a);
		malloc_allocator<char> c(sizeof(void *));
		b = std::move(a);
		BOOST_CHECK(b == a);
		// Constructor from different instance.
		malloc_allocator<integer> d(alignof(integer));
		malloc_allocator<char> e(d);
		BOOST_CHECK(e.get_alignment() == d.get_alignment());
	}
}

BOOST_AUTO_TEST_CASE(malloc_allocator_aligned_test)
{
	// Do the tests only if sizeof(void *) is a power of 2. All alignments are
	// required to be powers of 2, so if alignof(char) < sizeof(void *), sizeof(void *)
	// is a valid alignment for char (supposing that the platform provides that extended alignment).
	if (malloc_allocator<char>::have_memalign_primitives && !(sizeof(void *) & (sizeof(void *) - 1u))) {
		const std::size_t good_align = alignof(char) < sizeof(void *) ? sizeof(void *) : alignof(char), bad_align = 7u;
		malloc_allocator<char> good(good_align);
		BOOST_CHECK_THROW(new malloc_allocator<char>(bad_align),std::invalid_argument);
		auto ptr = good.allocate(0);
		BOOST_CHECK(ptr == nullptr);
		BOOST_CHECK_NO_THROW(good.deallocate(ptr,0));
		ptr = good.allocate(1);
		auto ptr2 = good.allocate(1);
		BOOST_CHECK(ptr != nullptr);
		BOOST_CHECK(ptr2 != nullptr);
		BOOST_CHECK_NO_THROW(good.deallocate(ptr,0));
		BOOST_CHECK_NO_THROW(good.deallocate(ptr2,0));
	}
}

BOOST_AUTO_TEST_CASE(malloc_allocator_equality_test)
{
	BOOST_CHECK(malloc_allocator<char>{} == malloc_allocator<char>{});
	if (malloc_allocator<char>::have_memalign_primitives) {
		BOOST_CHECK(malloc_allocator<char>{sizeof(void *)} == malloc_allocator<char>{sizeof(void *)});
		BOOST_CHECK(malloc_allocator<char>{sizeof(void *)} != malloc_allocator<char>{});
	}
}
