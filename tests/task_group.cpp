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

#include "../src/task_group.hpp"

#define BOOST_TEST_MODULE task_group_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <forward_list>
#include <functional>
#include <stdexcept>
#include <vector>

#include "../src/config.hpp"
#include "../src/environment.hpp"

using namespace piranha;

// Test construction and multiple waits.
BOOST_AUTO_TEST_CASE(task_group_run_test_01)
{
	environment env;
	task_group tg;
	for (int i = 0; i < 1000; ++i) {
		BOOST_CHECK_NO_THROW(tg.add_task([](){}));
	}
	BOOST_CHECK_NO_THROW(tg.wait_all());
	BOOST_CHECK_NO_THROW(tg.wait_all());
	BOOST_CHECK_NO_THROW(tg.get_all());
	BOOST_CHECK_NO_THROW(tg.get_all());
}

// Test destructor with embedded wait.
BOOST_AUTO_TEST_CASE(task_group_run_test_02)
{
	task_group tg;
	for (int i = 0; i < 1000; ++i) {
		BOOST_CHECK_NO_THROW(tg.add_task(std::bind([](int x, int y) {(void)(x + y);},i,i + 1)));
	}
}

BOOST_AUTO_TEST_CASE(task_group_run_test_03)
{
	task_group tg;
	for (int i = 0; i < 1000; ++i) {
		BOOST_CHECK_NO_THROW(tg.add_task([](){throw std::runtime_error("");}));
	}
	for (int i = 0; i < 1000; ++i) {
		BOOST_CHECK_THROW(tg.get_all(),std::runtime_error);
	}
	BOOST_CHECK_NO_THROW(tg.get_all());
	BOOST_CHECK_NO_THROW(tg.wait_all());
}

BOOST_AUTO_TEST_CASE(task_group_run_test_04)
{
	task_group tg;
	std::vector<unsigned> values(100u);
	for (auto i = 0u; i < 100u; ++i) {
		auto s = boost::lexical_cast<std::string>(1u);
		BOOST_CHECK_NO_THROW(tg.add_task([&values,s,i](){values[i] += boost::lexical_cast<unsigned>(s);}));
	}
	tg.wait_all();
	BOOST_CHECK((std::all_of(values.begin(),values.end(),[](unsigned n){return n == 1u;})));
}

BOOST_AUTO_TEST_CASE(task_group_run_test_05)
{
	task_group tg;
	std::forward_list<int> values;
	for (auto i = 0u; i < 1000u; ++i) {
		values.push_front(0);
		int *ptr = &values.front();
		tg.add_task([ptr](){*ptr += 1;});
	}
	tg.wait_all();
	BOOST_CHECK((std::all_of(values.begin(),values.end(),[](unsigned n){return n == 1u;})));
}

BOOST_AUTO_TEST_CASE(task_group_run_test_06)
{
	task_group tg;
	std::forward_list<int> values;
	for (auto i = 0u; i < 1000u; ++i) {
		values.push_front(0);
		int *ptr = &values.front();
		tg.add_task([ptr](){*ptr += 1;});
	}
	tg.wait_all();
	BOOST_CHECK((std::all_of(values.begin(),values.end(),[](unsigned n){return n == 1u;})));
}
