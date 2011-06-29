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

#include "../src/settings.hpp"

#define BOOST_TEST_MODULE settings_test
#include <boost/test/unit_test.hpp>

#include <stdexcept>

#include "../src/runtime_info.hpp"

// Check getting and setting number of threads.
BOOST_AUTO_TEST_CASE(settings_thread_number)
{
	BOOST_CHECK_PREDICATE([](unsigned n){return n != 0;},(piranha::settings::get_n_threads()));
	for (unsigned i = 0; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
		piranha::settings::set_n_threads(i + 1);
		BOOST_CHECK_EQUAL(piranha::settings::get_n_threads(), i + 1);
	}
	BOOST_CHECK_THROW(piranha::settings::set_n_threads(0),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(settings_reset_thread_number)
{
	piranha::settings::set_n_threads(10u);
	piranha::settings::reset_n_threads();
	BOOST_CHECK_EQUAL(piranha::settings::get_n_threads(),piranha::runtime_info::get_hardware_concurrency());
}
