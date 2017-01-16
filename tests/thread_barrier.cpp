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

#include <piranha/thread_barrier.hpp>

#define BOOST_TEST_MODULE thread_barrier_test
#include <boost/test/included/unit_test.hpp>

#include <functional>
#include <type_traits>

#include <piranha/init.hpp>
#include <piranha/thread_pool.hpp>

BOOST_AUTO_TEST_CASE(thread_barrier_test_01)
{
    piranha::init();
    const unsigned n_threads = 100;
    piranha::thread_barrier tb(n_threads);
    piranha::thread_pool::resize(n_threads);
    piranha::future_list<void> f_list;
    for (unsigned i = 0; i < n_threads; ++i) {
        BOOST_CHECK_NO_THROW(f_list.push_back(piranha::thread_pool::enqueue(i, std::bind(
                                                                                   [&tb](unsigned x, unsigned y) {
                                                                                       tb.wait();
                                                                                       (void)(x + y);
                                                                                   },
                                                                                   i, i + 1))));
    }
    BOOST_CHECK_NO_THROW(f_list.wait_all());
    BOOST_CHECK_NO_THROW(f_list.wait_all());
    BOOST_CHECK(!std::is_copy_assignable<piranha::thread_barrier>::value);
    BOOST_CHECK(!std::is_move_assignable<piranha::thread_barrier>::value);
}
