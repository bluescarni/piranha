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

#include "../src/runtime_info.hpp"

#define BOOST_TEST_MODULE runtime_info_test
#include <boost/test/unit_test.hpp>

#include "../src/threading.hpp"

// Simple check on the thread id.
BOOST_AUTO_TEST_CASE(runtime_info_thread_id_test)
{
	BOOST_CHECK_EQUAL(piranha::runtime_info::get_main_thread_id(),piranha::this_thread::get_id());
}

// Simple check on the thread concurrency.
BOOST_AUTO_TEST_CASE(runtime_info_hardware_concurrency_test)
{
	BOOST_CHECK_NO_THROW(piranha::runtime_info::hardware_concurrency());
}

// Simple check on cache line size.
BOOST_AUTO_TEST_CASE(runtime_info_get_cache_line_size)
{
	BOOST_CHECK_NO_THROW(piranha::runtime_info::get_cache_line_size());
}
