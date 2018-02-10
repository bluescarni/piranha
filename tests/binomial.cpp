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

#include <piranha/math/binomial.hpp>

#define BOOST_TEST_MODULE binomial_test
#include <boost/test/included/unit_test.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>

#include <piranha/integer.hpp>
#include <piranha/type_traits.hpp>

static std::mt19937 rng;
static const int ntries = 1000;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

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

template <>
class binomial_impl<b_00, b_00>
{
public:
    b_00 operator()(const b_00 &, const b_00 &) const;
};

template <>
class binomial_impl<b_01, b_01>
{
public:
    b_01 operator()(const b_01 &, const b_01 &) const;
};
}

BOOST_AUTO_TEST_CASE(binomial_test_00)
{
    BOOST_CHECK((!are_binomial_types<double, double>::value));
    BOOST_CHECK((!are_binomial_types<void, double>::value));
    BOOST_CHECK((!are_binomial_types<double, void>::value));
    BOOST_CHECK((!are_binomial_types<void, void>::value));
    BOOST_CHECK((!are_binomial_types<b_00, b_00>::value));
    BOOST_CHECK((!are_binomial_types<b_01, b_01>::value));
}

struct binomial_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK((are_binomial_types<int_type, int_type>::value));
        BOOST_CHECK((are_binomial_types<int_type, int_type &>::value));
        BOOST_CHECK((are_binomial_types<const int_type, int_type &>::value));
        BOOST_CHECK((are_binomial_types<int_type, int>::value));
        BOOST_CHECK((are_binomial_types<int_type, unsigned>::value));
        BOOST_CHECK((are_binomial_types<int_type, long>::value));
        BOOST_CHECK((are_binomial_types<int_type, char>::value));
        BOOST_CHECK((!are_binomial_types<int_type, void>::value));
        BOOST_CHECK((!are_binomial_types<void, int_type>::value));
        int_type n;
        BOOST_CHECK(piranha::binomial(n, 0) == 1);
        BOOST_CHECK(piranha::binomial(n, 1) == 0);
        n = 1;
        BOOST_CHECK(piranha::binomial(n, 1) == 1);
        n = 5;
        BOOST_CHECK(piranha::binomial(n, 3) == 10);
        n = -5;
        BOOST_CHECK(piranha::binomial(n, int_type(4)) == 70);
        BOOST_CHECK((are_binomial_types<int_type, int>::value));
        BOOST_CHECK((are_binomial_types<int, int_type>::value));
        BOOST_CHECK((std::is_same<int_type, decltype(piranha::binomial(int_type{}, 0))>::value));
        BOOST_CHECK((std::is_same<decltype(piranha::binomial(int_type{}, 0)), int_type>::value));
        BOOST_CHECK((!are_binomial_types<int_type, double>::value));
        BOOST_CHECK((!are_binomial_types<double, int_type>::value));
        BOOST_CHECK((are_binomial_types<int_type, int_type>::value));
        BOOST_CHECK((std::is_same<int_type, decltype(piranha::binomial(int_type{}, int_type{}))>::value));
        // Random tests.
        std::uniform_int_distribution<int> ud(-1000, 1000);
        std::uniform_int_distribution<int> promote_dist(0, 1);
        for (int i = 0; i < ntries; ++i) {
            auto tmp1 = ud(rng), tmp2 = ud(rng);
            n = tmp1;
            if (promote_dist(rng) && n.is_static()) {
                n.promote();
            }
            BOOST_CHECK_NO_THROW(piranha::binomial(n, tmp2));
        }
    }
};

BOOST_AUTO_TEST_CASE(binomial_test_01)
{
    tuple_for_each(size_types{}, binomial_tester{});
    // Check the ints.
    using int_type = integer;
    BOOST_CHECK((are_binomial_types<int, int>::value));
    BOOST_CHECK_EQUAL(piranha::binomial(4, 2), piranha::binomial(int_type(4), 2));
    BOOST_CHECK((are_binomial_types<char, unsigned>::value));
    BOOST_CHECK_EQUAL(piranha::binomial(char(4), 2u), piranha::binomial(int_type(4), 2));
    BOOST_CHECK((are_binomial_types<long long, int>::value));
    BOOST_CHECK_EQUAL(piranha::binomial(7ll, 4), piranha::binomial(int_type(7), 4));
    BOOST_CHECK((std::is_same<decltype(piranha::binomial(7ll, 4)), int_type>::value));
    BOOST_CHECK_EQUAL(piranha::binomial(-7ll, 4u), piranha::binomial(int_type(-7), 4));
    // Different static sizes.
    BOOST_CHECK((!are_binomial_types<mppp::integer<1>, mppp::integer<2>>::value));
    BOOST_CHECK((!are_binomial_types<mppp::integer<2>, mppp::integer<1>>::value));
#if defined(MPPP_HAVE_GCC_INT128)
    BOOST_CHECK((are_binomial_types<int_type, __int128_t>::value));
    BOOST_CHECK((are_binomial_types<int_type, __uint128_t>::value));
    BOOST_CHECK((are_binomial_types<__int128_t, int_type>::value));
    BOOST_CHECK((are_binomial_types<__uint128_t, int_type>::value));
    BOOST_CHECK((std::is_same<decltype(piranha::binomial(int_type(), __int128_t())), int_type>::value));
    BOOST_CHECK((std::is_same<decltype(piranha::binomial(__int128_t(), int_type())), int_type>::value));
    BOOST_CHECK_EQUAL(piranha::binomial(__int128_t(4), int_type(2)), piranha::binomial(int_type(4), 2));
    BOOST_CHECK_EQUAL(piranha::binomial(int_type(4), __uint128_t(2)), piranha::binomial(int_type(4), 2));
#endif
}
