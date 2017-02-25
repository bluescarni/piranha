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

#include <boost/algorithm/string/predicate.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/init.hpp>
#include <piranha/type_traits.hpp>

using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long>;

using fp_types = std::tuple<float, double, long double>;

struct foo {
};

// Struct without copy ctor.
struct foo_nc {
    foo_nc(const foo_nc &) = delete;
};

struct conv1 {
};

struct conv2 {
};

struct conv3 {
};

struct conv4 {
};

namespace piranha
{

// Good specialisation.
template <>
struct safe_cast_impl<conv2, conv1> {
    conv2 operator()(const conv1 &) const;
};

// Bad specialisation.
template <>
struct safe_cast_impl<conv3, conv1> {
    int operator()(const conv1 &) const;
};

// Bad specialisation.
template <>
struct safe_cast_impl<conv4, conv1> {
    conv4 operator()(conv1 &) const;
};
}

using namespace piranha;

struct int_checker {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            BOOST_CHECK((has_safe_cast<T, U>::value));
            BOOST_CHECK((has_safe_cast<T &, const U>::value));
            BOOST_CHECK((has_safe_cast<const T &, U &&>::value));
            BOOST_CHECK((has_safe_cast<T &, U &&>::value));
            BOOST_CHECK((!has_safe_cast<void, U>::value));
            BOOST_CHECK_EQUAL(safe_cast<T>(U(12)), T(12));
            if (std::is_unsigned<T>::value && std::is_signed<U>::value) {
                BOOST_CHECK_EXCEPTION(safe_cast<T>(U(-1)), safe_cast_failure, [](const safe_cast_failure &e) {
                    return boost::contains(e.what(), "cannot be converted");
                });
                BOOST_CHECK_EXCEPTION(safe_cast<T &>(U(-10)), safe_cast_failure, [](const safe_cast_failure &e) {
                    return boost::contains(e.what(), "the integral value");
                });
                BOOST_CHECK_EXCEPTION(
                    safe_cast<const T>(U(-1)), std::invalid_argument,
                    [](const std::invalid_argument &e) { return boost::contains(e.what(), "the integral value"); });
                BOOST_CHECK_EXCEPTION(
                    safe_cast<T &&>(U(-10)), std::invalid_argument,
                    [](const std::invalid_argument &e) { return boost::contains(e.what(), "cannot be converted"); });
            }
            if (std::is_signed<T>::value == std::is_signed<U>::value
                && std::numeric_limits<T>::max() < std::numeric_limits<U>::max()) {
                BOOST_CHECK_EXCEPTION(
                    safe_cast<T>(std::numeric_limits<U>::max()), safe_cast_failure,
                    [](const safe_cast_failure &e) { return boost::contains(e.what(), "cannot be converted"); });
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        BOOST_CHECK((!has_safe_cast<T, void>::value));
        BOOST_CHECK((!has_safe_cast<T, std::string>::value));
        BOOST_CHECK((!has_safe_cast<std::string, T>::value));
        tuple_for_each(int_types{}, runner<T>{});
    }
};

struct fp_int_checker {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            BOOST_CHECK((has_safe_cast<T, U>::value));
            BOOST_CHECK((has_safe_cast<T &, const U>::value));
            BOOST_CHECK((has_safe_cast<T &&, const U &>::value));
            BOOST_CHECK((has_safe_cast<const T &&, U &>::value));
            BOOST_CHECK((!has_safe_cast<U, T>::value));
            BOOST_CHECK((!has_safe_cast<U &, T &&>::value));
            BOOST_CHECK((!has_safe_cast<const U &, T &&>::value));
            BOOST_CHECK((!has_safe_cast<const U, const T>::value));
            // This should work with any fp type.
            BOOST_CHECK_EQUAL((safe_cast<T>(U(23))), U(23));
            BOOST_CHECK_EQUAL((safe_cast<T const>(U(23))), U(23));
            BOOST_CHECK_EQUAL((safe_cast<T &>(U(23))), U(23));
            BOOST_CHECK_EQUAL((safe_cast<T &&>(U(23))), U(23));
            BOOST_CHECK_EXCEPTION(safe_cast<T>(U(1.5)), safe_cast_failure, [](const safe_cast_failure &e) {
                return boost::contains(e.what(), "fractional part");
            });
            if (std::is_unsigned<T>::value) {
                BOOST_CHECK_EXCEPTION(safe_cast<T>(U(-23)), safe_cast_failure, [](const safe_cast_failure &e) {
                    return boost::contains(e.what(), "cannot be converted");
                });
            }
            if (std::numeric_limits<U>::is_iec559) {
                BOOST_CHECK_EXCEPTION(
                    safe_cast<T>(std::numeric_limits<U>::quiet_NaN()), safe_cast_failure,
                    [](const safe_cast_failure &e) { return boost::contains(e.what(), "non-finite"); });
                BOOST_CHECK_EXCEPTION(
                    safe_cast<T>(std::numeric_limits<U>::infinity()), safe_cast_failure,
                    [](const safe_cast_failure &e) { return boost::contains(e.what(), "non-finite"); });
            }
#if defined(PIRANHA_COMPILER_IS_GCC)
// GCC complains about int to float conversions here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif
            if (std::numeric_limits<U>::max() > std::numeric_limits<T>::max()) {
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic pop
#endif
                BOOST_CHECK_EXCEPTION(
                    safe_cast<T>(std::numeric_limits<U>::max()), safe_cast_failure,
                    [](const safe_cast_failure &e) { return boost::contains(e.what(), "cannot be converted"); });
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(fp_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(safe_cast_test_00)
{
    init();
    // Same type.
    BOOST_CHECK((has_safe_cast<std::string, std::string>::value));
    BOOST_CHECK((has_safe_cast<std::string, const std::string>::value));
    BOOST_CHECK((has_safe_cast<std::string &, const std::string>::value));
    BOOST_CHECK((has_safe_cast<const std::string &, std::string &&>::value));
    BOOST_CHECK((has_safe_cast<foo, foo>::value));
    BOOST_CHECK_EQUAL(safe_cast<std::string>(std::string("hello")), "hello");
    BOOST_CHECK_EQUAL(safe_cast<const std::string>(std::string("hello")), "hello");
    BOOST_CHECK_EQUAL(safe_cast<const std::string &>(std::string("hello")), "hello");

    // Check with custom specialisations.
    BOOST_CHECK((has_safe_cast<conv2, conv1>::value));
    BOOST_CHECK((has_safe_cast<conv2 &, const conv1>::value));
    BOOST_CHECK((has_safe_cast<conv2 &, const conv1 &>::value));
    BOOST_CHECK((!has_safe_cast<conv3, conv1>::value));
    BOOST_CHECK((!has_safe_cast<conv3 &, const conv1>::value));
    BOOST_CHECK((!has_safe_cast<conv3, conv1 &&>::value));
    BOOST_CHECK((!has_safe_cast<conv4, conv1>::value));
    BOOST_CHECK((!has_safe_cast<conv4, conv1 &>::value));
    BOOST_CHECK((!has_safe_cast<const conv4, const conv1 &>::value));

    // Missing copy/move ctor.
    BOOST_CHECK((!has_safe_cast<foo_nc, foo_nc>::value));
    BOOST_CHECK((!has_safe_cast<foo_nc, const foo_nc>::value));
    BOOST_CHECK((!has_safe_cast<foo_nc &, const foo_nc &>::value));
    BOOST_CHECK((!has_safe_cast<const foo_nc &, const foo_nc &>::value));

    // void test.
    BOOST_CHECK((!has_safe_cast<void, void>::value));

    // Integral conversions.
    tuple_for_each(int_types{}, int_checker{});

    // FP conversions.
    tuple_for_each(int_types{}, fp_int_checker{});
}
