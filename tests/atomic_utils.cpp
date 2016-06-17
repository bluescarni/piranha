/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "../src/detail/atomic_utils.hpp"

#define BOOST_TEST_MODULE atomic_utils_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <vector>

#include "../src/init.hpp"
#include "../src/thread_barrier.hpp"

using namespace piranha;

using a_array = detail::atomic_flag_array;
using alg = detail::atomic_lock_guard;

BOOST_AUTO_TEST_CASE(atomic_utils_atomic_flag_array_test)
{
	init();
	// Test with just an empty array.
	a_array a0(0u);
	// Non-empty.
	std::size_t size = 100u;
	a_array a1(size);
	// Verify everything is set to false.
	for (std::size_t i = 0u; i < size; ++i) {
		BOOST_CHECK(!a1[i].test_and_set());
		BOOST_CHECK(a1[i].test_and_set());
	}
	// Concurrent.
	size = 1000000u;
	a_array a2(size);
	thread_barrier tb(2u);
	auto func = [&a2,&tb,size]() {
		tb.wait();
		for (std::size_t i = 0u; i < size; ++i) {
			a2[i].test_and_set();
		}
	};
	std::thread t0(func);
	std::thread t1(func);
	t0.join();
	t1.join();
	for (std::size_t i = 0u; i < size; ++i) {
		BOOST_CHECK(a2[i].test_and_set());
		// Check also the const getter of the array.
		BOOST_CHECK(std::addressof(a2[i]) == std::addressof(static_cast<const a_array &>(a2)[i]));
	}
	// Some type traits checks.
	BOOST_CHECK(!std::is_constructible<a_array>::value);
	BOOST_CHECK(!std::is_copy_constructible<a_array>::value);
	BOOST_CHECK(!std::is_move_constructible<a_array>::value);
	BOOST_CHECK(!std::is_copy_assignable<a_array>::value);
	BOOST_CHECK(!std::is_move_assignable<a_array>::value);
}

BOOST_AUTO_TEST_CASE(atomic_utils_atomic_lock_guard_test)
{
	// Some type traits checks.
	BOOST_CHECK(!std::is_constructible<alg>::value);
	BOOST_CHECK(!std::is_copy_constructible<alg>::value);
	BOOST_CHECK(!std::is_move_constructible<alg>::value);
	BOOST_CHECK(!std::is_copy_assignable<alg>::value);
	BOOST_CHECK(!std::is_move_assignable<alg>::value);
	// Concurrent writes protected by a spinlock.
	std::size_t size = 10000u;
	using size_type = std::vector<double>::size_type;
	std::vector<double> v(size,0.);
	a_array a0(size);
	thread_barrier tb(2u);
	auto func = [&a0,&tb,&v,size]() {
		tb.wait();
		for (std::size_t i = 0u; i < size; ++i) {
			alg l(a0[i]);
			v[static_cast<size_type>(i)] = 1.;
		}
	};
	std::thread t0(func);
	std::thread t1(func);
	t0.join();
	t1.join();
	BOOST_CHECK(std::all_of(v.begin(),v.end(),[](double x){return x == 1.;}));
}
