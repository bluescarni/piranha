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

#include "../src/thread_management.hpp"

#define BOOST_TEST_MODULE thread_management_test
#include <boost/test/included/unit_test.hpp>

#include <mutex>

#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/runtime_info.hpp"
#include "../src/settings.hpp"
#include "../src/thread_barrier.hpp"
#include "../src/thread_pool.hpp"

std::mutex mutex;

static inline void test_function()
{
    for (unsigned i = 0u; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
        piranha::bind_to_proc(i);
        const auto retval = piranha::bound_proc();
        // Lock because Boost unit test is not thread-safe.
        std::lock_guard<std::mutex> lock(mutex);
        BOOST_CHECK_EQUAL(retval.first, true);
        BOOST_CHECK_EQUAL(retval.second, i);
    }
}

BOOST_AUTO_TEST_CASE(thread_management_new_threads_bind)
{
    piranha::init();
    for (unsigned i = 0u; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
        auto f = piranha::thread_pool::enqueue(i, []() { test_function(); });
        f.wait();
        try {
            f.get();
        } catch (const piranha::not_implemented_error &) {
            // If we are getting a NIE it's good, it means the platform does not
            // support binding.
        }
    }
}

// Check thread-safe binding using thread_pool.
BOOST_AUTO_TEST_CASE(thread_management_task_group_bind)
{
    piranha::future_list<void> f_list;
    for (unsigned i = 0u; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
        f_list.push_back(piranha::thread_pool::enqueue(0, []() { test_function(); }));
    }
    f_list.wait_all();
    try {
        f_list.get_all();
    } catch (const piranha::not_implemented_error &) {
    }
}
