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

#include <piranha/runtime_info.hpp>

#define BOOST_TEST_MODULE runtime_info_test
#include <boost/test/included/unit_test.hpp>

#include <iostream>

#include <piranha/init.hpp>
#include <piranha/memory.hpp>
#include <piranha/settings.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(runtime_info_print_test)
{
    init();
    std::cout << "Concurrency: " << runtime_info::get_hardware_concurrency() << '\n';
    std::cout << "Cache line size: " << runtime_info::get_cache_line_size() << '\n';
    std::cout << "Memory alignment primitives: "
              <<
#if defined(PIRANHA_HAVE_MEMORY_ALIGNMENT_PRIMITIVES)
        "available\n";
#else
        "not available\n";
#endif
}

BOOST_AUTO_TEST_CASE(runtime_info_settings_test)
{
    BOOST_CHECK(runtime_info::get_hardware_concurrency() == settings::get_n_threads()
                || runtime_info::get_hardware_concurrency() == 0u);
    BOOST_CHECK_EQUAL(runtime_info::get_cache_line_size(), settings::get_cache_line_size());
}
