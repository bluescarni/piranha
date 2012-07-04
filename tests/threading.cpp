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

#include "../src/threading.hpp"

#define BOOST_TEST_MODULE threading_test
#include <boost/test/unit_test.hpp>

#include "../src/environment.hpp"
#include "../src/real.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(threading_thread_test)
{
	environment env;
	{
		// Default-construction and destruction.
		thread t1, t2, t3;
	}
	auto f = [](real *r) {*r += 1;};
	real r1, r2;
	thread t1(f,&r1), t2(f,&r2);
	BOOST_CHECK(t1.joinable());
	BOOST_CHECK(t2.joinable());
	// Multiple join and detach calls.
	t1.join();
	t2.join();
	t1.join();
	t2.join();
	t1.detach();
	t2.detach();
	t1.detach();
	t2.detach();
	BOOST_CHECK(!t1.joinable());
	BOOST_CHECK(!t2.joinable());
	BOOST_CHECK_EQUAL(r1,1);
	BOOST_CHECK_EQUAL(r2,1);
	// Test the mpfr cache freeing.
	auto g = [](real *r) {*r += r->pi();};
	thread t3(g,&r1), t4(g,&r2);
	BOOST_CHECK(t3.joinable());
	BOOST_CHECK(t4.joinable());
	t3.join();
	t4.join();
	t3.join();
	t4.join();
	t3.detach();
	t4.detach();
	t3.detach();
	t4.detach();
	BOOST_CHECK(!t3.joinable());
	BOOST_CHECK(!t4.joinable());
	BOOST_CHECK_EQUAL(r1,1 + real{}.pi());
	BOOST_CHECK_EQUAL(r2,1 + real{}.pi());
}
