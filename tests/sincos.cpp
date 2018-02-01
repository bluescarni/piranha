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

#include <piranha/math/cos.hpp>
#include <piranha/math/sin.hpp>

#define BOOST_TEST_MODULE sincos_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>

using namespace piranha;

struct b_00 {
    b_00() = default;
    b_00(const b_00 &) = delete;
    b_00(b_00 &&) = delete;
};

struct b_01 {
    b_01() = default;
    b_01(const b_01 &) = default;
    b_01(b_01 &&) = default;
    ~b_01() = delete;
};

namespace piranha
{

namespace math
{

template <>
class sin_impl<b_00>
{
public:
    b_00 operator()(const b_00 &) const;
};

template <>
class sin_impl<b_01>
{
public:
    b_01 operator()(const b_01 &) const;
};

template <>
class cos_impl<b_00>
{
public:
    b_00 operator()(const b_00 &) const;
};

template <>
class cos_impl<b_01>
{
public:
    b_01 operator()(const b_01 &) const;
};
}
}

BOOST_AUTO_TEST_CASE(sin_test_00)
{
    BOOST_CHECK(!is_sine_type<std::string>::value);
    BOOST_CHECK(!is_sine_type<void>::value);
    BOOST_CHECK(!is_sine_type<void>::value);
    BOOST_CHECK(!is_sine_type<b_00>::value);
    BOOST_CHECK(!is_sine_type<b_01>::value);
    BOOST_CHECK(is_sine_type<float>::value);
    BOOST_CHECK(is_sine_type<double>::value);
    BOOST_CHECK(is_sine_type<long double>::value);
    BOOST_CHECK(is_sine_type<float &>::value);
    BOOST_CHECK(is_sine_type<const double>::value);
    BOOST_CHECK(is_sine_type<const long double &&>::value);
    BOOST_CHECK(is_sine_type<int>::value);
    BOOST_CHECK(is_sine_type<const unsigned>::value);
    BOOST_CHECK(is_sine_type<long>::value);
    BOOST_CHECK(is_sine_type<long long &>::value);
    BOOST_CHECK(is_sine_type<const char &>::value);
    BOOST_CHECK_EQUAL(math::sin(4.5f), std::sin(4.5f));
    BOOST_CHECK_EQUAL(math::sin(4.5), std::sin(4.5));
    BOOST_CHECK_EQUAL(math::sin(4.5l), std::sin(4.5l));
    BOOST_CHECK((std::is_same<decltype(math::sin(4.5f)), float>::value));
    BOOST_CHECK((std::is_same<decltype(math::sin(4.5)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::sin(4.5l)), long double>::value));
    BOOST_CHECK_EQUAL(math::sin(0), 0);
    BOOST_CHECK_EQUAL(math::sin(0u), 0u);
    BOOST_CHECK_EQUAL(math::sin(static_cast<signed char>(0)), static_cast<signed char>(0));
    BOOST_CHECK((std::is_same<decltype(math::sin(0)), int>::value));
    BOOST_CHECK((std::is_same<decltype(math::sin(0u)), unsigned>::value));
    BOOST_CHECK((std::is_same<decltype(math::sin(static_cast<signed char>(0))), signed char>::value));
    BOOST_CHECK_EXCEPTION(math::sin(42), std::domain_error, [](const std::domain_error &ex) {
        return boost::contains(ex.what(), "cannot compute the sine of the non-zero C++ integral 42");
    });
}

BOOST_AUTO_TEST_CASE(cos_test_00)
{
    BOOST_CHECK(!is_cosine_type<std::string>::value);
    BOOST_CHECK(!is_cosine_type<void>::value);
    BOOST_CHECK(!is_cosine_type<void>::value);
    BOOST_CHECK(!is_cosine_type<b_00>::value);
    BOOST_CHECK(!is_cosine_type<b_01>::value);
    BOOST_CHECK(is_cosine_type<float>::value);
    BOOST_CHECK(is_cosine_type<double>::value);
    BOOST_CHECK(is_cosine_type<long double>::value);
    BOOST_CHECK(is_cosine_type<float &>::value);
    BOOST_CHECK(is_cosine_type<const double>::value);
    BOOST_CHECK(is_cosine_type<const long double &&>::value);
    BOOST_CHECK(is_cosine_type<int>::value);
    BOOST_CHECK(is_cosine_type<const unsigned>::value);
    BOOST_CHECK(is_cosine_type<long>::value);
    BOOST_CHECK(is_cosine_type<long long &>::value);
    BOOST_CHECK(is_cosine_type<const char &>::value);
    BOOST_CHECK_EQUAL(math::cos(4.5f), std::cos(4.5f));
    BOOST_CHECK_EQUAL(math::cos(4.5), std::cos(4.5));
    BOOST_CHECK_EQUAL(math::cos(4.5l), std::cos(4.5l));
    BOOST_CHECK((std::is_same<decltype(math::cos(4.5f)), float>::value));
    BOOST_CHECK((std::is_same<decltype(math::cos(4.5)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::cos(4.5l)), long double>::value));
    BOOST_CHECK_EQUAL(math::cos(0), 1);
    BOOST_CHECK_EQUAL(math::cos(0u), 1u);
    BOOST_CHECK_EQUAL(math::cos(static_cast<signed char>(0)), static_cast<signed char>(1));
    BOOST_CHECK((std::is_same<decltype(math::cos(0)), int>::value));
    BOOST_CHECK((std::is_same<decltype(math::cos(0u)), unsigned>::value));
    BOOST_CHECK((std::is_same<decltype(math::cos(static_cast<signed char>(0))), signed char>::value));
    BOOST_CHECK_EXCEPTION(math::cos(42), std::domain_error, [](const std::domain_error &ex) {
        return boost::contains(ex.what(), "cannot compute the cosine of the non-zero C++ integral 42");
    });
}
