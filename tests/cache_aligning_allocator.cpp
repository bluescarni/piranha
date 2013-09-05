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

#include "../src/cache_aligning_allocator.hpp"

#define BOOST_TEST_MODULE cache_aligning_allocator_test
#include <boost/test/unit_test.hpp>

#include <string>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/runtime_info.hpp"
#include "../src/settings.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(cache_aligning_allocator_constructor_test)
{
	environment env;
	cache_aligning_allocator<char> caa1;
	BOOST_CHECK_EQUAL(caa1.alignment(),runtime_info::get_cache_line_size());
	cache_aligning_allocator<integer> caa2;
	BOOST_CHECK_EQUAL(caa2.alignment(),runtime_info::get_cache_line_size());
	cache_aligning_allocator<std::string> caa3;
	BOOST_CHECK_EQUAL(caa3.alignment(),runtime_info::get_cache_line_size());
	// Constructor from different instance.
	cache_aligning_allocator<int> caa4(caa1);
	BOOST_CHECK_EQUAL(caa4.alignment(),caa1.alignment());
	// Constructor from different instance.
	cache_aligning_allocator<int> caa5(std::move(caa1));
	BOOST_CHECK_EQUAL(caa4.alignment(),caa5.alignment());
	settings::set_cache_line_size(settings::get_cache_line_size() * 2u);
	cache_aligning_allocator<char> caa6;
	BOOST_CHECK(caa6.alignment() == settings::get_cache_line_size());
	cache_aligning_allocator<int> caa7(std::move(caa6));
	BOOST_CHECK(caa7.alignment() == settings::get_cache_line_size());
	settings::set_cache_line_size(3);
	cache_aligning_allocator<int> caa8;
	BOOST_CHECK(caa8.alignment() == 0u);
}
