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

#include <piranha/exceptions.hpp>

#define BOOST_TEST_MODULE exceptions_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <new>
#include <string>
#include <type_traits>

#include <piranha/init.hpp>

using namespace piranha;

struct exc0 {
    exc0(int, double)
    {
    }
};

struct exc1 {
    exc1(int)
    {
    }
};

BOOST_AUTO_TEST_CASE(exception_test_00)
{
    init();
    // not_implemented_error.
    BOOST_CHECK((std::is_constructible<not_implemented_error, std::string>::value));
    BOOST_CHECK((std::is_constructible<not_implemented_error, char *>::value));
    BOOST_CHECK((std::is_constructible<not_implemented_error, const char *>::value));
    BOOST_CHECK((!std::is_constructible<not_implemented_error>::value));
    BOOST_CHECK_EXCEPTION(piranha_throw(not_implemented_error, "foobar"), not_implemented_error,
                          [](const not_implemented_error &e) { return boost::contains(e.what(), "foobar"); });
    BOOST_CHECK_EXCEPTION(piranha_throw(not_implemented_error, std::string("foobar")), not_implemented_error,
                          [](const not_implemented_error &e) { return boost::contains(e.what(), "foobar"); });
    BOOST_CHECK_EXCEPTION(piranha_throw(not_implemented_error, "foobar"), std::runtime_error,
                          [](const std::runtime_error &e) { return boost::contains(e.what(), "foobar"); });
    BOOST_CHECK_EXCEPTION(piranha_throw(not_implemented_error, std::string("foobar")), std::runtime_error,
                          [](const std::runtime_error &e) { return boost::contains(e.what(), "foobar"); });
    // zero_division_error.
    BOOST_CHECK((std::is_constructible<zero_division_error, std::string>::value));
    BOOST_CHECK((std::is_constructible<zero_division_error, char *>::value));
    BOOST_CHECK((std::is_constructible<zero_division_error, const char *>::value));
    BOOST_CHECK((!std::is_constructible<zero_division_error>::value));
    BOOST_CHECK_EXCEPTION(piranha_throw(zero_division_error, "foobar"), zero_division_error,
                          [](const zero_division_error &e) { return boost::contains(e.what(), "foobar"); });
    BOOST_CHECK_EXCEPTION(piranha_throw(zero_division_error, std::string("foobar")), zero_division_error,
                          [](const zero_division_error &e) { return boost::contains(e.what(), "foobar"); });
    BOOST_CHECK_EXCEPTION(piranha_throw(zero_division_error, "foobar"), std::domain_error,
                          [](const std::domain_error &e) { return boost::contains(e.what(), "foobar"); });
    BOOST_CHECK_EXCEPTION(piranha_throw(zero_division_error, std::string("foobar")), std::domain_error,
                          [](const std::domain_error &e) { return boost::contains(e.what(), "foobar"); });
    // A couple of tests with exceptions that do not accept string ctor.
    BOOST_CHECK_THROW(piranha_throw(std::bad_alloc, ), std::bad_alloc);
    BOOST_CHECK_THROW(piranha_throw(exc0, 1, 2.3), exc0);
    BOOST_CHECK_THROW(piranha_throw(exc1, 1), exc1);
}
