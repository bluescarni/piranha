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

#include <piranha/pow.hpp>

#define BOOST_TEST_MODULE pow_test
#include <boost/test/included/unit_test.hpp>

#include <cmath>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>

#include <piranha/init.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

BOOST_AUTO_TEST_CASE(pow_fp_test)
{
    init();
    BOOST_CHECK(math::pow(2., 2.) == std::pow(2., 2.));
    BOOST_CHECK(math::pow(2.f, 2.) == std::pow(2.f, 2.));
    BOOST_CHECK(math::pow(2., 2.f) == std::pow(2., 2.f));
    BOOST_CHECK((std::is_same<decltype(math::pow(2., 2.)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2.f, 2.f)), float>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2., 2.f)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2.f, 2.)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2.f, 2)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2.f, 2.L)), long double>::value));
    BOOST_CHECK(math::pow(2., 2) == std::pow(2., 2));
    BOOST_CHECK(math::pow(2.f, 2) == std::pow(2.f, 2));
    BOOST_CHECK((std::is_same<decltype(math::pow(2., 2)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2.f, 2)), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::pow(2.f, char(2))), double>::value));
    BOOST_CHECK((is_exponentiable<double, double>::value));
    BOOST_CHECK((!is_exponentiable<void, double>::value));
    BOOST_CHECK((!is_exponentiable<double, void>::value));
    BOOST_CHECK((!is_exponentiable<void, void>::value));
    BOOST_CHECK((is_exponentiable<double, unsigned short>::value));
    BOOST_CHECK((is_exponentiable<double &, double>::value));
    BOOST_CHECK((is_exponentiable<const double, double>::value));
    BOOST_CHECK((is_exponentiable<double &, double &>::value));
    BOOST_CHECK((is_exponentiable<double &, double const &>::value));
    BOOST_CHECK((is_exponentiable<double, double &>::value));
    BOOST_CHECK((is_exponentiable<float, double>::value));
    BOOST_CHECK((is_exponentiable<double, float>::value));
    BOOST_CHECK((is_exponentiable<double, int>::value));
    BOOST_CHECK((is_exponentiable<float, char>::value));
}

using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long>;

struct int_pow_tester {
    template <typename U>
    struct runner {
        template <typename T>
        void operator()(const T &) const
        {
            using piranha::math::pow;
            using int_type = mp_integer<U::value>;
            BOOST_CHECK((is_exponentiable<int_type, T>::value));
            BOOST_CHECK((is_exponentiable<int_type, float>::value));
            BOOST_CHECK((is_exponentiable<float, int_type>::value));
            BOOST_CHECK((is_exponentiable<double, int_type>::value));
            BOOST_CHECK((is_exponentiable<long double, int_type>::value));
            int_type n;
            BOOST_CHECK((std::is_same<int_type, decltype(pow(n, T(0)))>::value));
            BOOST_CHECK_EQUAL(pow(n, T(0)), 1);
            if (std::is_signed<T>::value) {
                BOOST_CHECK_THROW(pow(n, T(-1)), mppp::zero_division_error);
            }
            n = 1;
            BOOST_CHECK_EQUAL(pow(n, T(0)), 1);
            if (std::is_signed<T>::value) {
                BOOST_CHECK_EQUAL(pow(n, T(-1)), 1);
            }
            n = -1;
            BOOST_CHECK_EQUAL(pow(n, T(0)), 1);
            if (std::is_signed<T>::value) {
                BOOST_CHECK_EQUAL(pow(n, T(-1)), -1);
            }
            n = 2;
            BOOST_CHECK_EQUAL(pow(n, T(0)), 1);
            BOOST_CHECK_EQUAL(pow(n, T(1)), 2);
            BOOST_CHECK_EQUAL(pow(n, T(2)), 4);
            BOOST_CHECK_EQUAL(pow(n, T(4)), 16);
            BOOST_CHECK_EQUAL(pow(n, T(5)), 32);
            if (std::is_signed<T>::value) {
                BOOST_CHECK_EQUAL(pow(n, T(-1)), 0);
            }
            n = -3;
            BOOST_CHECK_EQUAL(pow(n, T(0)), 1);
            BOOST_CHECK_EQUAL(pow(n, T(1)), -3);
            BOOST_CHECK_EQUAL(pow(n, T(2)), 9);
            BOOST_CHECK_EQUAL(pow(n, T(4)), 81);
            BOOST_CHECK_EQUAL(pow(n, T(5)), -243);
            BOOST_CHECK_EQUAL(pow(n, T(13)), -1594323);
            if (std::is_signed<T>::value) {
                BOOST_CHECK_EQUAL(pow(n, T(-1)), 0);
            }
            // Test here the various math::pow() overloads as well.
            // Integer--integer.
            BOOST_CHECK((is_exponentiable<int_type, int_type>::value));
            BOOST_CHECK((std::is_same<int_type, decltype(pow(int_type(1), int_type(1)))>::value));
            BOOST_CHECK_EQUAL(pow(int_type(2), int_type(3)), 8);
            // Integer -- integral.
            BOOST_CHECK((is_exponentiable<int_type, int>::value));
            BOOST_CHECK((is_exponentiable<int_type, char>::value));
            BOOST_CHECK((is_exponentiable<int_type, unsigned long>::value));
            BOOST_CHECK((std::is_same<int_type, decltype(pow(int_type(1), 1))>::value));
            BOOST_CHECK((std::is_same<int_type, decltype(pow(int_type(1), 1ul))>::value));
            BOOST_CHECK((std::is_same<int_type, decltype(pow(int_type(1), (signed char)1))>::value));
            BOOST_CHECK_EQUAL(pow(int_type(2), 3), 8);
            // Integer -- floating-point.
            BOOST_CHECK((is_exponentiable<int_type, double>::value));
            BOOST_CHECK((std::is_same<double, decltype(pow(int_type(1), 1.))>::value));
            BOOST_CHECK_EQUAL(pow(int_type(2), 3.), pow(2., 3.));
            BOOST_CHECK_EQUAL(pow(int_type(2), 1. / 3.), pow(2., 1. / 3.));
            // Integral -- integer.
            BOOST_CHECK((is_exponentiable<int, int_type>::value));
            BOOST_CHECK((is_exponentiable<short, int_type>::value));
            BOOST_CHECK((std::is_same<int_type, decltype(pow(1, int_type(1)))>::value));
            BOOST_CHECK((std::is_same<int_type, decltype(pow(short(1), int_type(1)))>::value));
            BOOST_CHECK_EQUAL(pow(2, int_type(3)), 8.);
            // Floating-point -- integer.
            BOOST_CHECK((is_exponentiable<float, int_type>::value));
            BOOST_CHECK((is_exponentiable<double, int_type>::value));
            BOOST_CHECK((std::is_same<float, decltype(pow(1.f, int_type(1)))>::value));
            BOOST_CHECK((std::is_same<double, decltype(pow(1., int_type(1)))>::value));
            BOOST_CHECK_EQUAL(pow(2.f, int_type(3)), pow(2.f, 3.f));
            BOOST_CHECK_EQUAL(pow(2., int_type(3)), pow(2., 3.));
            BOOST_CHECK_EQUAL(pow(2.f / 5.f, int_type(3)), pow(2.f / 5.f, 3.f));
            BOOST_CHECK_EQUAL(pow(2. / 7., int_type(3)), pow(2. / 7., 3.));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(int_types{}, runner<T>{});
    }
};

struct mp_integer_pow_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mp_integer<T::value>;
        using piranha::math::pow;
        BOOST_CHECK((is_exponentiable<int_type, int_type>::value));
        BOOST_CHECK((!is_exponentiable<int_type, void>::value));
        BOOST_CHECK((!is_exponentiable<void, int_type>::value));
        BOOST_CHECK((is_exponentiable<const int_type, int_type &>::value));
        BOOST_CHECK((is_exponentiable<float, int_type>::value));
        BOOST_CHECK((is_exponentiable<float &&, const int_type &>::value));
        BOOST_CHECK((is_exponentiable<double, int_type>::value));
        BOOST_CHECK((is_exponentiable<double, int_type &>::value));
        BOOST_CHECK((is_exponentiable<long double, int_type>::value));
        BOOST_CHECK((is_exponentiable<const long double, int_type &&>::value));
        int_type n;
        BOOST_CHECK((std::is_same<int_type, decltype(pow(n, n))>::value));
        BOOST_CHECK_EQUAL(pow(n, int_type(0)), 1);
        BOOST_CHECK_THROW(pow(n, int_type(-1)), mppp::zero_division_error);
        n = 1;
        BOOST_CHECK_EQUAL(pow(n, int_type(0)), 1);
        BOOST_CHECK_EQUAL(pow(n, int_type(-1)), 1);
        n = -1;
        BOOST_CHECK_EQUAL(pow(n, int_type(0)), 1);
        BOOST_CHECK_EQUAL(pow(n, int_type(-1)), -1);
        n = 2;
        BOOST_CHECK_EQUAL(pow(n, int_type(0)), 1);
        BOOST_CHECK_EQUAL(pow(n, int_type(1)), 2);
        BOOST_CHECK_EQUAL(pow(n, int_type(2)), 4);
        BOOST_CHECK_EQUAL(pow(n, int_type(4)), 16);
        BOOST_CHECK_EQUAL(pow(n, int_type(5)), 32);
        BOOST_CHECK_EQUAL(pow(n, int_type(-1)), 0);
        n = -3;
        BOOST_CHECK_EQUAL(pow(n, int_type(0)), 1);
        BOOST_CHECK_EQUAL(pow(n, int_type(1)), -3);
        BOOST_CHECK_EQUAL(pow(n, int_type(2)), 9);
        BOOST_CHECK_EQUAL(pow(n, int_type(4)), 81);
        BOOST_CHECK_EQUAL(pow(n, int_type(5)), -243);
        BOOST_CHECK_EQUAL(pow(n, int_type(13)), -1594323);
        BOOST_CHECK_EQUAL(pow(n, int_type(-1)), 0);
    }
};

BOOST_AUTO_TEST_CASE(pow_mp_integer_test)
{
    tuple_for_each(size_types{}, int_pow_tester{});
    tuple_for_each(size_types{}, mp_integer_pow_tester{});
    // Integral--integral pow.
    BOOST_CHECK_EQUAL(math::pow(4, 2), 16);
    BOOST_CHECK_EQUAL(math::pow(-3ll, (unsigned short)3), -27);
    BOOST_CHECK((std::is_same<integer, decltype(math::pow(-3ll, (unsigned short)3))>::value));
    BOOST_CHECK((is_exponentiable<int, int>::value));
    BOOST_CHECK((is_exponentiable<int, char>::value));
    BOOST_CHECK((is_exponentiable<unsigned, long long>::value));
    BOOST_CHECK((!is_exponentiable<mp_integer<1>, mp_integer<2>>::value));
    BOOST_CHECK((!is_exponentiable<mp_integer<2>, mp_integer<1>>::value));
    BOOST_CHECK((!is_exponentiable<integer, std::string>::value));
}
