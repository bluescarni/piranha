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

#include "../src/init.hpp"

#define BOOST_TEST_MODULE init_test
#include <boost/test/unit_test.hpp>

#include "../src/config.hpp"
#include "../src/settings.hpp"
#include "../src/thread_pool.hpp"

using namespace piranha;

struct dummy {
    ~dummy()
    {
        // NOTE: cannot use BOOST_CHECK here because this gets invoked outside the test case.
        piranha_assert(detail::shutdown());
    }
};

static dummy d;

BOOST_AUTO_TEST_CASE(init_main_test)
{
    settings::set_n_threads(3);
    // Multiple concurrent constructions.
    auto f0 = thread_pool::enqueue(0, []() { init(); });
    auto f1 = thread_pool::enqueue(1, []() { init(); });
    auto f2 = thread_pool::enqueue(2, []() { init(); });
    f0.wait();
    f1.wait();
    f2.wait();
    BOOST_CHECK(!detail::shutdown());
    BOOST_CHECK_EQUAL(detail::piranha_init_statics<>::s_failed.load(), 2u);
}
