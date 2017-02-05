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

#include <piranha/binomial.hpp>

#define BOOST_TEST_MODULE binomial_test
#include <boost/test/included/unit_test.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <piranha/init.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/type_traits.hpp>

static std::mt19937 rng;
static const int ntries = 1000;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

using namespace piranha;

BOOST_AUTO_TEST_CASE(binomial_fp_test)
{
    init();
    // Random testing.
    std::uniform_real_distribution<double> rdist(-100., 100.);
    for (int i = 0; i < ntries; ++i) {
        const double x = rdist(rng), y = rdist(rng);
        // NOTE: at the moment we have nothing to check this against,
        // maybe in the future we can check against real.
        BOOST_CHECK(std::isfinite(math_fp_binomial(x, y)));
    }
    std::uniform_int_distribution<int> idist(-100, 100);
    for (int i = 0; i < ntries; ++i) {
        // NOTE: maybe this could be checked against the mp_integer implementation.
        const int x = idist(rng), y = idist(rng);
        BOOST_CHECK(std::isfinite(math_fp_binomial(static_cast<double>(x), static_cast<double>(y))));
        BOOST_CHECK(std::isfinite(math_fp_binomial(static_cast<long double>(x), static_cast<long double>(y))));
    }
}

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
struct binomial_impl<b_00, b_00> {
    b_00 operator()(const b_00 &, const b_00 &) const;
};

template <>
struct binomial_impl<b_01, b_01> {
    b_01 operator()(const b_01 &, const b_01 &) const;
};
}
}

BOOST_AUTO_TEST_CASE(binomial_test_00)
{
    BOOST_CHECK((std::is_same<double, decltype(math::binomial(0., 0))>::value));
    BOOST_CHECK((std::is_same<double, decltype(math::binomial(0., 0u))>::value));
    BOOST_CHECK((std::is_same<double, decltype(math::binomial(0., 0l))>::value));
    BOOST_CHECK((std::is_same<float, decltype(math::binomial(0.f, 0))>::value));
    BOOST_CHECK((std::is_same<float, decltype(math::binomial(0.f, 0u))>::value));
    BOOST_CHECK((std::is_same<float, decltype(math::binomial(0.f, 0ll))>::value));
    BOOST_CHECK((std::is_same<long double, decltype(math::binomial(0.l, 0))>::value));
    BOOST_CHECK((std::is_same<long double, decltype(math::binomial(0.l, char(0)))>::value));
    BOOST_CHECK((std::is_same<long double, decltype(math::binomial(0.l, short(0)))>::value));
    BOOST_CHECK((has_binomial<double, int>::value));
    BOOST_CHECK((has_binomial<double, unsigned>::value));
    BOOST_CHECK((has_binomial<float, char>::value));
    BOOST_CHECK((has_binomial<float, float>::value));
    BOOST_CHECK((has_binomial<float, double>::value));
    if (std::numeric_limits<double>::has_quiet_NaN && std::numeric_limits<double>::has_infinity) {
        BOOST_CHECK_THROW(math::binomial(1., std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
        BOOST_CHECK_THROW(math::binomial(1., std::numeric_limits<double>::infinity()), std::invalid_argument);
        BOOST_CHECK_THROW(math::binomial(std::numeric_limits<double>::quiet_NaN(), 1.), std::invalid_argument);
        BOOST_CHECK_THROW(math::binomial(std::numeric_limits<double>::infinity(), 1.), std::invalid_argument);
        BOOST_CHECK_THROW(
            math::binomial(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()),
            std::invalid_argument);
        BOOST_CHECK_THROW(
            math::binomial(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()),
            std::invalid_argument);
    }
    BOOST_CHECK((!has_binomial<b_00, b_00>::value));
    BOOST_CHECK((!has_binomial<b_01, b_01>::value));
}

struct binomial_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mp_integer<T::value>;
        BOOST_CHECK((has_binomial<int_type, int_type>::value));
        BOOST_CHECK((has_binomial<int_type, int>::value));
        BOOST_CHECK((has_binomial<int_type, unsigned>::value));
        BOOST_CHECK((has_binomial<int_type, long>::value));
        BOOST_CHECK((has_binomial<int_type, char>::value));
        int_type n;
        BOOST_CHECK(math::binomial(n,0) == 1);
        BOOST_CHECK(math::binomial(n,1) == 0);
        n = 1;
        BOOST_CHECK(n.binomial(1) == 1);
        n = 5;
        BOOST_CHECK(n.binomial(3) == 10);
        n = -5;
        BOOST_CHECK(n.binomial(int_type(4)) == 70);
        BOOST_CHECK((has_binomial<int_type, int>::value));
        BOOST_CHECK((has_binomial<int, int_type>::value));
        BOOST_CHECK((std::is_same<int_type, decltype(math::binomial(int_type{}, 0))>::value));
        BOOST_CHECK((std::is_same<decltype(math::binomial(int_type{}, 0)), int_type>::value));
        BOOST_CHECK((has_binomial<int_type, double>::value));
        BOOST_CHECK((has_binomial<double, int_type>::value));
        BOOST_CHECK((std::is_same<double, decltype(math::binomial(int_type{}, 0.))>::value));
        BOOST_CHECK((std::is_same<decltype(math::binomial(int_type{}, 0.)), double>::value));
        BOOST_CHECK((has_binomial<int_type, int_type>::value));
        BOOST_CHECK((std::is_same<int_type, decltype(math::binomial(int_type{}, int_type{}))>::value));
        // Random tests.
        std::uniform_int_distribution<int> ud(-1000, 1000);
        std::uniform_int_distribution<int> promote_dist(0, 1);
        mpz_raii m;
        for (int i = 0; i < ntries; ++i) {
            auto tmp1 = ud(rng), tmp2 = ud(rng);
            n = tmp1;
            if (promote_dist(rng) && n.is_static()) {
                n.promote();
            }
            if (tmp2 < 0) {
                // NOTE: we cannot check this case with GMP, defer to some tests below.
                BOOST_CHECK_NO_THROW(n.binomial(tmp2));
                continue;
            }
            ::mpz_set_si(&m.m_mpz, static_cast<long>(tmp1));
            ::mpz_bin_ui(&m.m_mpz, &m.m_mpz, static_cast<unsigned long>(tmp2));
            BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.binomial(tmp2)), mpz_lexcast(m));
            // int -- integral.
            BOOST_CHECK_EQUAL(math::binomial(n, tmp2), n.binomial(tmp2));
            // integral -- int.
            BOOST_CHECK_EQUAL(math::binomial(tmp2, n), int_type(tmp2).binomial(n));
            // integral -- integral.
            BOOST_CHECK_EQUAL(math::binomial(tmp2, tmp1), integer(tmp2).binomial(tmp1));
            // int -- double.
            BOOST_CHECK_EQUAL(math::binomial(n, static_cast<double>(tmp2)),
                              math::binomial(double(n), static_cast<double>(tmp2)));
            // double -- int.
            BOOST_CHECK_EQUAL(math::binomial(static_cast<double>(tmp2), n),
                              math::binomial(static_cast<double>(tmp2), double(n)));
            BOOST_CHECK_EQUAL(n.binomial(tmp2), n.binomial(int_type(tmp2)));
            BOOST_CHECK_EQUAL(n.binomial(long(tmp2)), n.binomial(int_type(tmp2)));
            BOOST_CHECK_EQUAL(n.binomial((long long)(tmp2)), n.binomial(int_type(tmp2)));
            BOOST_CHECK_EQUAL(n.binomial((unsigned long)(tmp2)), n.binomial(int_type(tmp2)));
            BOOST_CHECK_EQUAL(n.binomial((unsigned long long)(tmp2)), n.binomial(int_type(tmp2)));
        }
        BOOST_CHECK_THROW(n.binomial(std::numeric_limits<unsigned long>::max() + int_type(1)), std::invalid_argument);
        // Negative k.
        BOOST_CHECK_EQUAL(int_type{-3}.binomial(-4), -3);
        BOOST_CHECK_EQUAL(int_type{-3}.binomial(-10), -36);
        BOOST_CHECK_EQUAL(int_type{-3}.binomial(-1), 0);
        BOOST_CHECK_EQUAL(int_type{3}.binomial(-1), 0);
        BOOST_CHECK_EQUAL(int_type{10}.binomial(-1), 0);
        BOOST_CHECK_EQUAL(int_type{-3}.binomial(-3), 1);
        BOOST_CHECK_EQUAL(int_type{-1}.binomial(-1), 1);
    }
};

BOOST_AUTO_TEST_CASE(binomial_test_01)
{
    tuple_for_each(size_types{}, binomial_tester{});
    // Check the ints.
    using int_type = integer;
    BOOST_CHECK((has_binomial<int, int>::value));
    BOOST_CHECK_EQUAL(math::binomial(4, 2), math::binomial(int_type(4), 2));
    BOOST_CHECK((has_binomial<char, unsigned>::value));
    BOOST_CHECK_EQUAL(math::binomial(char(4), 2u), math::binomial(int_type(4), 2));
    BOOST_CHECK((has_binomial<long long, int>::value));
    BOOST_CHECK_EQUAL(math::binomial(7ll, 4), math::binomial(int_type(7), 4));
    BOOST_CHECK((std::is_same<decltype(math::binomial(7ll, 4)), int_type>::value));
    BOOST_CHECK_EQUAL(math::binomial(-7ll, 4u), math::binomial(int_type(-7), 4));
    // Different static sizes.
    BOOST_CHECK((!has_binomial<mp_integer<1>, mp_integer<2>>::value));
    BOOST_CHECK((!has_binomial<mp_integer<2>, mp_integer<1>>::value));
}
