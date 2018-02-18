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

#include <piranha/math/is_one.hpp>

#define BOOST_TEST_MODULE is_one_test
#include <boost/test/included/unit_test.hpp>

#include <complex>
#include <string>

struct trivial {
};

struct trivial_a {
};

struct trivial_b {
};

struct trivial_c {
};

struct trivial_d {
};

namespace piranha
{

template <>
class is_one_impl<trivial_a, void>
{
public:
    char operator()(const trivial_a &);
};

template <>
class is_one_impl<trivial_b, void>
{
public:
    char operator()(const trivial_a &);
};

template <>
class is_one_impl<trivial_c, void>
{
public:
    std::string operator()(const trivial_c &);
};

template <>
class is_one_impl<trivial_d, void>
{
public:
    trivial_d operator()(const trivial_d &) const;
};
}

using namespace piranha;

BOOST_AUTO_TEST_CASE(is_one_test_00)
{
    BOOST_CHECK(!is_is_one_type<void>::value);
    BOOST_CHECK(!is_is_one_type<std::string>::value);
    BOOST_CHECK(is_is_one_type<int>::value);
    BOOST_CHECK(is_is_one_type<const long>::value);
    BOOST_CHECK(is_is_one_type<const unsigned long &>::value);
    BOOST_CHECK(is_is_one_type<float &&>::value);
    BOOST_CHECK(is_is_one_type<std::complex<float>>::value);
    BOOST_CHECK(is_is_one_type<std::complex<double> &>::value);
    BOOST_CHECK(is_is_one_type<const std::complex<long double> &>::value);
    BOOST_CHECK(!piranha::is_one(0));
    BOOST_CHECK(piranha::is_one(1u));
    BOOST_CHECK(!piranha::is_one(0.));
    BOOST_CHECK(!piranha::is_one(1.23l));
    BOOST_CHECK(!piranha::is_one(std::complex<float>{0, 0}));
    BOOST_CHECK(piranha::is_one(std::complex<float>{1, 0}));
    BOOST_CHECK(!piranha::is_one(std::complex<float>{1, -1}));
    BOOST_CHECK(!piranha::is_one(std::complex<float>{1.2f, 0}));
    BOOST_CHECK(!piranha::is_one(std::complex<double>{-1, 0}));
    BOOST_CHECK(!piranha::is_one(std::complex<double>{0, -1}));
    BOOST_CHECK(!piranha::is_one(std::complex<double>{1, 1}));
    BOOST_CHECK((!is_is_one_type<trivial>::value));
    BOOST_CHECK((!is_is_one_type<trivial &>::value));
    BOOST_CHECK((!is_is_one_type<trivial &&>::value));
    BOOST_CHECK((!is_is_one_type<const trivial &&>::value));
    BOOST_CHECK(is_is_one_type<trivial_a>::value);
    BOOST_CHECK(is_is_one_type<trivial_a &>::value);
    BOOST_CHECK(!is_is_one_type<trivial_b>::value);
    BOOST_CHECK(!is_is_one_type<trivial_c>::value);
    BOOST_CHECK(!is_is_one_type<trivial_d>::value);
}
