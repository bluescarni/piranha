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

#include <piranha/detail/ulshift.hpp>

#define BOOST_TEST_MODULE ulshift_test
#include <boost/test/included/unit_test.hpp>

#include <limits>
#include <type_traits>

#include <piranha/init.hpp>

using namespace piranha::detail;

BOOST_AUTO_TEST_CASE(ulshift_test_00)
{
    piranha::init();
    using utype = unsigned short;
    constexpr unsigned nbits = std::numeric_limits<utype>::digits;
    utype n0(1);
    n0 = static_cast<utype>(n0 << (nbits - 1u));
    BOOST_CHECK((std::is_same<decltype(ulshift(n0, 1u)), utype>::value));
    BOOST_CHECK_EQUAL(ulshift(n0, 1u), 0u);
    BOOST_CHECK_EQUAL(ulshift(n0, 2u), 0u);
    BOOST_CHECK_EQUAL(ulshift(n0, 3u), 0u);
    BOOST_CHECK_EQUAL(ulshift(utype(2), nbits - 1u), 0u);
}

BOOST_AUTO_TEST_CASE(ulshift_test_10)
{
    using utype = unsigned;
    constexpr unsigned nbits = std::numeric_limits<utype>::digits;
    utype n0(1);
    n0 = static_cast<utype>(n0 << (nbits - 1u));
    BOOST_CHECK((std::is_same<decltype(ulshift(n0, 1u)), utype>::value));
    BOOST_CHECK_EQUAL(ulshift(n0, 1u), 0u);
    BOOST_CHECK_EQUAL(ulshift(n0, 2u), 0u);
    BOOST_CHECK_EQUAL(ulshift(n0, 3u), 0u);
    BOOST_CHECK_EQUAL(ulshift(utype(2), nbits - 1u), 0u);
}
