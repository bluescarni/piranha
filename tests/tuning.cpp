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

#include "../src/tuning.hpp"

#define BOOST_TEST_MODULE tuning_test
#include <boost/test/unit_test.hpp>

#include <stdexcept>
#include <thread>

#include "../src/environment.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(tuning_parallel_memory_set_test)
{
	environment env;
	BOOST_CHECK(tuning::get_parallel_memory_set());
	tuning::set_parallel_memory_set(false);
	BOOST_CHECK(!tuning::get_parallel_memory_set());
	std::thread t1([](){
		while (!tuning::get_parallel_memory_set()) {}
	});
	std::thread t2([](){
		tuning::set_parallel_memory_set(true);
	});
	t1.join();
	t2.join();
	BOOST_CHECK(tuning::get_parallel_memory_set());
	tuning::set_parallel_memory_set(false);
	BOOST_CHECK(!tuning::get_parallel_memory_set());
	tuning::reset_parallel_memory_set();
	BOOST_CHECK(tuning::get_parallel_memory_set());
}

BOOST_AUTO_TEST_CASE(tuning_block_size_test)
{
	BOOST_CHECK_EQUAL(tuning::get_multiplication_block_size(),256u);
	tuning::set_multiplication_block_size(512u);
	BOOST_CHECK_EQUAL(tuning::get_multiplication_block_size(),512u);
	std::thread t1([](){
		while (tuning::get_multiplication_block_size() != 1024u) {}
	});
	std::thread t2([](){
		tuning::set_multiplication_block_size(1024u);
	});
	t1.join();
	t2.join();
	BOOST_CHECK_THROW(tuning::set_multiplication_block_size(8000u),std::invalid_argument);
	BOOST_CHECK_EQUAL(tuning::get_multiplication_block_size(),1024u);
	tuning::reset_multiplication_block_size();
	BOOST_CHECK_EQUAL(tuning::get_multiplication_block_size(),256u);
}
