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

#include "../src/thread.hpp"

#define BOOST_TEST_MODULE thread_test
#include <boost/test/unit_test.hpp>

#include <type_traits>

#include "../src/environment.hpp"
#include "../src/real.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(thread_main_test)
{
	environment env;
	real r1(0), r2(r1);
	auto f1 = [&r1]() {r1 += 1;};
	auto f2 = [&r2]() {r2 += 1;};
	thread t1(f1), t2(f2);
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
	auto g1 = [&r1]() {r1 += r1.pi();};
	auto g2 = [&r2]() {r2 += r2.pi();};
	thread t3(g1), t4(g2);
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

void foo();

struct functor_00
{
	void operator()();
};

struct functor_01
{
	int operator()();
};

struct functor_02
{
	functor_02();
	functor_02(const functor_02 &);
	functor_02(functor_02 &&);
	void operator()();
};

struct functor_03
{
	functor_03();
	functor_03(const functor_03 &);
	functor_03(functor_03 &&) = delete;
	void operator()();
};

BOOST_AUTO_TEST_CASE(thread_type_traits_test)
{
	auto f = [](){};
	BOOST_CHECK((std::is_constructible<thread,decltype(f)>::value));
	auto g = [](int){};
	BOOST_CHECK((!std::is_constructible<thread,decltype(g)>::value));
	BOOST_CHECK((!std::is_constructible<thread,decltype(foo)>::value));
	BOOST_CHECK((!std::is_constructible<thread,int>::value));
	BOOST_CHECK((std::is_constructible<thread,functor_00>::value));
	BOOST_CHECK((!std::is_constructible<thread,functor_01>::value));
	BOOST_CHECK((std::is_constructible<thread,functor_02>::value));
	BOOST_CHECK((!std::is_constructible<thread,functor_03>::value));
}
