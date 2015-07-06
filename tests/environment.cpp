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

#include "../src/environment.hpp"

#define BOOST_TEST_MODULE environment_test
#include <boost/test/unit_test.hpp>

#include <iostream>

#include "../src/settings.hpp"
#include "../src/thread_pool.hpp"

using namespace piranha;

struct dummy
{
	~dummy()
	{
		std::cout << "Shutdown flag is: " << environment::shutdown() << '\n';
	}
};

static dummy d;

BOOST_AUTO_TEST_CASE(environment_main_test)
{
	settings::set_n_threads(3);
	// Multiple concurrent constructions.
	auto f0 = thread_pool::enqueue(0,[]() {environment env;});
	auto f1 = thread_pool::enqueue(1,[]() {environment env;});
	auto f2 = thread_pool::enqueue(2,[]() {environment env;});
	f0.wait();
	f1.wait();
	f2.wait();
	BOOST_CHECK(!environment::shutdown());
}
