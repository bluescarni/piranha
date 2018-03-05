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

#include <piranha/safe_convert.hpp>

#define BOOST_TEST_MODULE safe_convert_test
#include <boost/test/included/unit_test.hpp>

#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <tuple>
#include <type_traits>

#include <mp++/integer.hpp>

#include <piranha/integer.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long>;

// The usual wrapper to generate uniform int distributions.
template <typename T>
static inline std::uniform_int_distribution<long long> get_dist(const std::true_type &)
{
    return std::uniform_int_distribution<long long>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
}

template <typename T>
static inline std::uniform_int_distribution<unsigned long long> get_dist(const std::false_type &)
{
    return std::uniform_int_distribution<unsigned long long>(0, std::numeric_limits<T>::max());
}

static std::mt19937 rng;
static const int ntrials = 1000;

struct foo {
};

struct bar {
    int n = 0;
};

namespace piranha
{

template <>
class safe_convert_impl<foo, foo>
{
public:
    bool operator()(foo &, const foo &) const;
    bool operator()(foo &, foo &) const = delete;
};
}

struct int_checker {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            BOOST_CHECK((!is_safely_convertible<U, T>::value));
            BOOST_CHECK((is_safely_convertible<U, T &>::value));
            BOOST_CHECK((is_safely_convertible<const U, T &>::value));
            BOOST_CHECK((is_safely_convertible<U &, T &>::value));
            BOOST_CHECK((!is_safely_convertible<U, void>::value));
            T out;
            auto dist = get_dist<U>(std::is_signed<U>{});
            for (auto i = 0; i < ntrials; ++i) {
                const auto tmp = static_cast<U>(dist(rng));
                const bool flag = safe_convert(out, tmp);
                // We check that the conversion status is consistent
                // with the conversion routine from mp++.
                BOOST_CHECK(flag == mppp::get(out, integer{tmp}));
                if (flag) {
                    // If the conversion was successful, let's make
                    // sure the value was actually written out.
                    BOOST_CHECK_EQUAL(out, static_cast<T>(tmp));
                }
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        BOOST_CHECK((!is_safely_convertible<void, T &>::value));
        BOOST_CHECK((!is_safely_convertible<std::string, T &>::value));
        BOOST_CHECK((!is_safely_convertible<T, std::string &>::value));
        tuple_for_each(int_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(safe_convert_test_00)
{
    BOOST_CHECK((!is_safely_convertible<void, void>::value));
    BOOST_CHECK((!is_safely_convertible<int, void>::value));
    BOOST_CHECK((!is_safely_convertible<void, int>::value));
    BOOST_CHECK((is_safely_convertible<const int, int &>::value));
    BOOST_CHECK((is_safely_convertible<const int &, int &>::value));
    BOOST_CHECK((is_safely_convertible<int &, int &>::value));
    BOOST_CHECK((is_safely_convertible<int &&, int &>::value));
    BOOST_CHECK((!is_safely_convertible<int &&, int>::value));
    BOOST_CHECK((!is_safely_convertible<int &&, const int>::value));
    BOOST_CHECK((!is_safely_convertible<int &&, const int &>::value));
    BOOST_CHECK((is_safely_convertible<const float, int &>::value));
    BOOST_CHECK((is_safely_convertible<const double &, unsigned &>::value));
    BOOST_CHECK((is_safely_convertible<long double &, long long &>::value));
    BOOST_CHECK((is_safely_convertible<float &&, char &>::value));
    BOOST_CHECK((!is_safely_convertible<float &&, int>::value));
    BOOST_CHECK((!is_safely_convertible<const double &&, int>::value));
    BOOST_CHECK((!is_safely_convertible<long double &&, const int &>::value));
    BOOST_CHECK((!is_safely_convertible<double, float &>::value));
    BOOST_CHECK((!is_safely_convertible<double &, const long double &>::value));
    BOOST_CHECK((is_safely_convertible<foo, foo &>::value));
    BOOST_CHECK((!is_safely_convertible<foo &, foo &>::value));

    // Integral conversions.
    tuple_for_each(int_types{}, int_checker{});

    // Fp to int conversions.
    unsigned un;
    BOOST_CHECK(!safe_convert(un, -1.));
    BOOST_CHECK(safe_convert(un, 5.));
    BOOST_CHECK_EQUAL(un, 5u);
    int n;
    BOOST_CHECK(!safe_convert(n, 1.5f));
    BOOST_CHECK(safe_convert(n, 3.l));
    BOOST_CHECK_EQUAL(n, 3);
    // A couple of tests for the common case in which we have ieee FP and a 32-bit
    // integer type available.
    if (std::numeric_limits<double>::is_iec559) {
        BOOST_CHECK(!safe_convert(n, std::numeric_limits<double>::quiet_NaN()));
        BOOST_CHECK(!safe_convert(n, std::numeric_limits<double>::infinity()));
        BOOST_CHECK(!safe_convert(n, -std::numeric_limits<double>::infinity()));
        if (std::numeric_limits<std::uint_least32_t>::digits == 32 && std::numeric_limits<double>::digits > 32
            && std::numeric_limits<double>::radix == 2) {
            // 4294967296 == 2**32.
            std::uint_least32_t un32;
            BOOST_CHECK(!safe_convert(un32, 4294967296.));
            BOOST_CHECK(safe_convert(un32, 4294967295.));
            BOOST_CHECK_EQUAL(un32, 4294967295ull);
        }
    }

    // Check the default implementation.
    BOOST_CHECK((is_safely_convertible<bar, bar &>::value));
    BOOST_CHECK((is_safely_convertible<bar, bar &&>::value));
    BOOST_CHECK((is_safely_convertible<const bar, bar &>::value));
    BOOST_CHECK((is_safely_convertible<const bar &, bar &>::value));
    BOOST_CHECK((!is_safely_convertible<const bar &, const bar &>::value));
    bar b;
    b.n = 12;
    safe_convert(b, bar{});
    BOOST_CHECK_EQUAL(b.n, 0);
}
