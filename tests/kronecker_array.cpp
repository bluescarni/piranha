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

#include <piranha/kronecker_array.hpp>

#define BOOST_TEST_MODULE kronecker_array_test
#include <boost/test/included/unit_test.hpp>

#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include <piranha/type_traits.hpp>

using namespace piranha;

using int_types = std::tuple<signed char, signed short, int, long, long long>;

// Limits.
struct limits_tester {
    template <typename T>
    void operator()(const T &) const
    {
        const auto &l = k_limits<T>();
        using l_type = uncvref_t<decltype(l)>;
        typedef typename l_type::value_type v_type;
        typedef typename l_type::size_type size_type;
        BOOST_CHECK(l.size() > 1u);
        BOOST_CHECK(l[0u] == v_type());
        BOOST_CHECK(std::get<0u>(l[1u])[0u] == -std::get<1u>(l[1u]));
        BOOST_CHECK(std::get<0u>(l[1u])[0u] == std::get<2u>(l[1u]));
        for (size_type i = 1u; i < l.size(); ++i) {
            for (size_type j = 0u; j < std::get<0u>(l[i]).size(); ++j) {
                BOOST_CHECK(std::get<0u>(l[i])[j] > 0);
            }
            BOOST_CHECK(std::get<1u>(l[i]) < 0);
            BOOST_CHECK(std::get<2u>(l[i]) > 0);
            BOOST_CHECK(std::get<3u>(l[i]) > 0);
            if (std::is_same<T, std::make_signed<std::size_t>::type>::value) {
                std::cout << '[';
                for (size_type j = 0u; j < std::get<0u>(l[i]).size(); ++j) {
                    std::cout << std::get<0u>(l[i])[j] << ',';
                }
                std::cout << "] " << std::get<1u>(l[i]) << ',' << std::get<2u>(l[i]) << ',' << std::get<3u>(l[i])
                          << '\n';
            }
        }
    }
};

BOOST_AUTO_TEST_CASE(kronecker_array_limits_test)
{
    tuple_for_each(int_types{}, limits_tester{});
}

// Coding/decoding.
struct coding_tester {
    template <typename T>
    void operator()(const T &) const
    {
        const auto &l = k_limits<T>();
        BOOST_CHECK(k_encode<T>(std::vector<std::int16_t>{}) == 0);
        BOOST_CHECK(k_encode<T>(std::vector<std::int16_t>{0}) == 0);
        BOOST_CHECK(k_encode<T>(std::vector<std::int16_t>{1}) == 1);
        BOOST_CHECK(k_encode<T>(std::vector<std::int16_t>{-1}) == -1);
        BOOST_CHECK(k_encode<T>(std::vector<std::int16_t>{-10}) == -10);
        BOOST_CHECK(k_encode<T>(std::vector<std::int16_t>{10}) == 10);
        // NOTE: static_cast for use when T is a char.
        const T emax1 = std::get<0u>(l[1u])[0u], emin1 = static_cast<T>(-emax1);
        BOOST_CHECK(k_encode<T>(std::vector<T>{emin1}) == emin1);
        BOOST_CHECK(k_encode<T>(std::vector<T>{emax1}) == emax1);
        std::mt19937 rng;
        // Test with max/min vectors in various sizes.
        for (std::uint_least8_t i = 1u; i < l.size(); ++i) {
            auto M = std::get<0u>(l[i]);
            auto m = M;
            for (auto it = m.begin(); it != m.end(); ++it) {
                *it = static_cast<T>(-(*it));
            }
            auto tmp(m);
            auto c = k_encode<T>(m);
            k_decode(c, tmp);
            BOOST_CHECK(m == tmp);
            tmp = M;
            c = k_encode<T>(M);
            k_decode(c, tmp);
            BOOST_CHECK(M == tmp);
            auto v1 = std::vector<T>(i, 0);
            auto v2 = v1;
            c = k_encode<T>(v1);
            k_decode(c, v1);
            BOOST_CHECK(v2 == v1);
            v1 = std::vector<T>(i, -1);
            v2 = v1;
            c = k_encode<T>(v1);
            k_decode(c, v1);
            BOOST_CHECK(v2 == v1);
            // Test with random values within the bounds.
            for (auto j = 0; j < 10000; ++j) {
                for (decltype(v1.size()) k = 0u; k < v1.size(); ++k) {
                    std::uniform_int_distribution<long long> dist(m[k], M[k]);
                    v1[k] = static_cast<T>(dist(rng));
                }
                v2 = v1;
                c = k_encode<T>(v1);
                k_decode(c, v1);
                BOOST_CHECK(v2 == v1);
            }
        }
        // Exceptions tests.
        BOOST_CHECK_THROW(k_encode<T>(std::vector<T>(l.size())), std::invalid_argument);
        BOOST_CHECK_THROW(k_encode<T>(std::vector<T>{T(0), std::numeric_limits<T>::min()}), std::invalid_argument);
        BOOST_CHECK_THROW(k_encode<T>(std::vector<T>{T(0), std::numeric_limits<T>::max()}), std::invalid_argument);
        std::vector<T> v1;
        v1.resize(static_cast<typename std::vector<T>::size_type>(l.size()));
        BOOST_CHECK_THROW(k_decode(T(0), v1), std::invalid_argument);
        v1.resize(0);
        BOOST_CHECK_THROW(k_decode(T(1), v1), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_array_coding_test)
{
    tuple_for_each(int_types{}, coding_tester{});
}

// k_decode_iterator.
struct k_decode_iterator_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using it_type = k_decode_iterator<T>;
        BOOST_CHECK((is_input_iterator<it_type>::value));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_array_k_decode_iterator_test)
{
    tuple_for_each(int_types{}, k_decode_iterator_tester{});
}
