/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#include <piranha/lamptify.hpp>

#define BOOST_TEST_MODULE lamptify_test
#include <boost/test/included/unit_test.hpp>

#include <piranha/init.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(lamptify_test_00)
{
    init();
    auto l = math::lamptify<double>(4.5, {});
    std::cout << l({}) << '\n';
    std::cout << has_lamptify<double, double>::value << '\n';
    std::cout << has_lamptify<void, double>::value << '\n';
    std::cout << has_lamptify<double, void>::value << '\n';
    std::cout << has_lamptify<void, void>::value << '\n';
}
