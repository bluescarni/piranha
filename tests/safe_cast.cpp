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

#include <piranha/safe_cast.hpp>

#define BOOST_TEST_MODULE safe_cast_test
#include <boost/test/included/unit_test.hpp>

#include <iterator>
#include <string>
#include <type_traits>

#include <boost/algorithm/string/predicate.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(safe_cast_test_00)
{
    BOOST_CHECK((is_safely_castable<int &&, int>::value));
    BOOST_CHECK((is_safely_castable<const float, long>::value));
    BOOST_CHECK((is_safely_castable<const double, long>::value));
    BOOST_CHECK((!is_safely_castable<const double, void>::value));
    BOOST_CHECK((!is_safely_castable<void, void>::value));

    BOOST_CHECK_EQUAL(safe_cast<unsigned>(5), 5u);
    BOOST_CHECK((std::is_same<decltype(safe_cast<unsigned>(5)), unsigned>::value));
    BOOST_CHECK_EXCEPTION(safe_cast<unsigned>(-5), safe_cast_failure, [](const safe_cast_failure &scf) {
        return boost::contains(scf.what(), "the safe conversion of a value of type");
    });
    BOOST_CHECK_EQUAL(safe_cast<int>(123.), 123);
    BOOST_CHECK((std::is_same<decltype(safe_cast<int>(123.)), int>::value));
    BOOST_CHECK_EXCEPTION(safe_cast<int>(123.456), safe_cast_failure, [](const safe_cast_failure &scf) {
        return boost::contains(scf.what(), "the safe conversion of a value of type");
    });
}
