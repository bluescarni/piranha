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

#include <iostream>

#include "../src/settings.hpp"
#include "../src/threading.hpp"

using namespace piranha;

// Simple check on the thread id.
BOOST_AUTO_TEST_CASE(runtime_info_thread_id_test)
{
	BOOST_CHECK_EQUAL(runtime_info::get_main_thread_id(),this_thread::get_id());
}

BOOST_AUTO_TEST_CASE(runtime_info_print_test)
{
	std::cout << "Concurrency: " << runtime_info::get_hardware_concurrency() << '\n';
	std::cout << "Cache line size: " << runtime_info::get_cache_line_size() << '\n';
	std::cout << "Memory alignment primitives: " << (runtime_info::have_memalign_primitives() ? "available" : "unavailable") << '\n';
	std::cout << "Threading primitives: " << ((runtime_info::threading_primitives() == 0) ? "C++11" : "Boost.Thread") << '\n';
}

BOOST_AUTO_TEST_CASE(runtime_info_settings_test)
{
	BOOST_CHECK_EQUAL(runtime_info::get_hardware_concurrency(),settings::get_n_threads());
	BOOST_CHECK_EQUAL(runtime_info::get_cache_line_size(),settings::get_cache_line_size());
}
