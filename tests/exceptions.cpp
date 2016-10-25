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

#include "../src/exceptions.hpp"

#define BOOST_TEST_MODULE exceptions_test
#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <string>
#include <type_traits>

#include "../src/init.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(exception_test_00)
{
    init();
    BOOST_CHECK((std::is_constructible<not_implemented_error,std::string>::value));
    BOOST_CHECK((std::is_constructible<not_implemented_error,char *>::value));
    BOOST_CHECK((std::is_constructible<not_implemented_error,const char *>::value));
    BOOST_CHECK_EXCEPTION(piranha_throw(not_implemented_error, "foobar"),not_implemented_error,[](const not_implemented_error &e) {
        return boost::contains(e.what(),"foobar");
    });
    BOOST_CHECK_EXCEPTION(piranha_throw(not_implemented_error, std::string("foobar")),not_implemented_error,[](const not_implemented_error &e) {
        return boost::contains(e.what(),"foobar");
    });
/*
    BOOST_CHECK_THROW(piranha_throw(custom_exception0, ), custom_exception0);
    BOOST_CHECK_THROW(piranha_throw(custom_exception1, ), custom_exception1);
    BOOST_CHECK_THROW(piranha_throw(custom_exception1, std::string("")), custom_exception1);
    BOOST_CHECK_THROW(piranha_throw(custom_exception1, ""), custom_exception1);
    BOOST_CHECK_THROW(piranha_throw(custom_exception2, ), custom_exception2);
    BOOST_CHECK_THROW(piranha_throw(custom_exception2, std::string("")), custom_exception2);
    BOOST_CHECK_THROW(piranha_throw(custom_exception2, std::string(""), 3), custom_exception2);
    BOOST_CHECK_THROW(piranha_throw(custom_exception3, ), custom_exception3);
    BOOST_CHECK_THROW(piranha_throw(custom_exception3, std::string("")), custom_exception3);
    BOOST_CHECK_THROW(piranha_throw(custom_exception3, std::string(""), 3), custom_exception3);
    BOOST_CHECK_THROW(piranha_throw(custom_exception3, 3, std::string("")), custom_exception3);
    const std::string empty;
    try {
        piranha_throw(custom_exception0, );
    } catch (const base_exception &e) {
        BOOST_CHECK_EQUAL(e.what(), empty);
    }
    try {
        piranha_throw(custom_exception1, );
    } catch (const base_exception &e) {
        BOOST_CHECK_EQUAL(e.what(), empty);
    }
    try {
        piranha_throw(custom_exception1, std::string(""));
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() != empty);
    }
    try {
        piranha_throw(custom_exception1, "");
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() != empty);
    }
    try {
        piranha_throw(custom_exception2, );
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() == empty);
    }
    try {
        piranha_throw(custom_exception2, "");
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() != empty);
    }
    try {
        piranha_throw(custom_exception2, "", 3);
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() != empty);
    }
    try {
        piranha_throw(custom_exception3, "", 3);
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() != empty);
    }
    try {
        piranha_throw(custom_exception3, 3, empty);
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() == empty);
    }
    try {
        piranha_throw(custom_exception4, "");
    } catch (const base_exception &e) {
        BOOST_CHECK(e.what() == empty);
    }*/
}
