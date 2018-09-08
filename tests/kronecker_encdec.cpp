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

#include <piranha/utils/kronecker_encdec.hpp>

#define BOOST_TEST_MODULE kronecker_encdec_test
#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>

#include <piranha/detail/demangle.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using int_types = std::tuple<signed char, signed short, int, long, long long>;

// Limits.
struct limits_tester {
    template <typename T>
    void operator()(const T &) const
    {
        const auto &l = piranha::k_limits<T>();
        BOOST_CHECK(l.size() > 1u);
        BOOST_CHECK(l[0u] == uncvref_t<decltype(l[0u])>{});
        BOOST_CHECK(std::get<0u>(l[1u])[0u] == -std::get<1u>(l[1u]));
        BOOST_CHECK(std::get<0u>(l[1u])[0u] == std::get<2u>(l[1u]));
        for (decltype(l.size()) i = 1u; i < l.size(); ++i) {
            for (decltype(std::get<0u>(l[i]).size()) j = 0u; j < std::get<0u>(l[i]).size(); ++j) {
                BOOST_CHECK(std::get<0u>(l[i])[j] > 0);
            }
            BOOST_CHECK(std::get<1u>(l[i]) < 0);
            BOOST_CHECK(std::get<2u>(l[i]) > 0);
            BOOST_CHECK(std::get<3u>(l[i]) > 0);
            // NOTE: print the limits for the signed counterpart of std::size_t,
            // which is likely to be the "natural" integral type on the platform.
            if (std::is_same<T, std::make_signed<std::size_t>::type>::value) {
                std::cout << '[';
                for (decltype(std::get<0u>(l[i]).size()) j = 0u; j < std::get<0u>(l[i]).size(); ++j) {
                    std::cout << std::get<0u>(l[i])[j] << ',';
                }
                std::cout << "] " << std::get<1u>(l[i]) << ',' << std::get<2u>(l[i]) << ',' << std::get<3u>(l[i])
                          << '\n';
            }
        }
    }
};

BOOST_AUTO_TEST_CASE(kronecker_limits_test)
{
    tuple_for_each(int_types{}, limits_tester{});
}

// Error handling in the streaming encoder/decoder.
struct stream_errors_tester {
    template <typename T>
    void operator()(const T &) const
    {
        const auto &l = piranha::k_limits<T>();
        BOOST_CHECK_EXCEPTION(k_encoder<T>{l.size()}, std::overflow_error, [&l](const std::overflow_error &e) {
            return boost::contains(e.what(), "cannot Kronecker-encode a sequence of size " + std::to_string(l.size())
                                                 + " to the signed integral type '" + piranha::demangle<T>()
                                                 + "': the maximum allowed size for this signed integral type is "
                                                 + std::to_string(l.size() - 1u));
        });
        k_encoder<T> k0{0};
        BOOST_CHECK_EXCEPTION(k0 << T(1), std::out_of_range, [](const std::out_of_range &e) {
            return boost::contains(
                e.what(), "cannot push any more values to this Kronecker encoder: the number of "
                          "values already pushed to the encoder is equal to the size used for construction (0)");
        });
        BOOST_CHECK_EXCEPTION(k0 << T(1), std::out_of_range, [](const std::out_of_range &e) {
            return boost::contains(
                e.what(), "cannot push any more values to this Kronecker encoder: the number of "
                          "values already pushed to the encoder is equal to the size used for construction (0)");
        });
        k_encoder<T> k1{1};
        BOOST_CHECK_EXCEPTION(k1 << T(1) << T(2), std::out_of_range, [](const std::out_of_range &e) {
            return boost::contains(
                e.what(), "cannot push any more values to this Kronecker encoder: the number of "
                          "values already pushed to the encoder is equal to the size used for construction (1)");
        });
        k_encoder<T> k2{2};
        if (std::get<0>(l[2])[0] < std::numeric_limits<T>::max()) {
            BOOST_CHECK_EXCEPTION(
                k2 << std::numeric_limits<T>::max(), std::overflow_error, [&l](const std::overflow_error &e) {
                    return boost::contains(e.what(),
                                           "one of the elements of a sequence to be Kronecker-encoded is out of "
                                           "bounds: the value of the element is "
                                               + std::to_string(std::numeric_limits<T>::max())
                                               + ", while the bounds are [" + std::to_string(-std::get<0>(l[2])[0])
                                               + ", " + std::to_string(std::get<0>(l[2])[0]) + "]");
                });
            BOOST_CHECK_EXCEPTION(
                k2 << std::numeric_limits<T>::max(), std::overflow_error, [&l](const std::overflow_error &e) {
                    return boost::contains(e.what(),
                                           "one of the elements of a sequence to be Kronecker-encoded is out of "
                                           "bounds: the value of the element is "
                                               + std::to_string(std::numeric_limits<T>::max())
                                               + ", while the bounds are [" + std::to_string(-std::get<0>(l[2])[0])
                                               + ", " + std::to_string(std::get<0>(l[2])[0]) + "]");
                });
        }
        k_encoder<T> k3{3};
        k3 << T(1);
        BOOST_CHECK_EXCEPTION(k3.get(), std::out_of_range, [](const std::out_of_range &e) {
            return boost::contains(
                e.what(), "cannot fetch the Kronecker-encoded value from this Kronecker encoder: the number of "
                          "values pushed to the encoder (1) is less than the size used for construction (3)");
        });
        if (std::numeric_limits<T>::max() > std::get<1>(l[1])) {
            BOOST_CHECK_EXCEPTION(
                (k_decoder<T>{std::numeric_limits<T>::max(), 1}), std::overflow_error,
                [&l](const std::overflow_error &e) {
                    return boost::contains(
                        e.what(),
                        "cannot Kronecker-decode the signed integer " + std::to_string(std::numeric_limits<T>::max())
                            + " of type '" + piranha::demangle<T>()
                            + "' into a range of size 1: the value of the integer is outside the allowed bounds ["
                            + std::to_string(std::get<1>(l[1])) + ", " + std::to_string(std::get<2>(l[1])) + "]");
                });
        }
        k_decoder<T> d0{1, 1};
        T out;
        d0 >> out;
        BOOST_CHECK_EXCEPTION(d0 >> out, std::out_of_range, [&l](const std::out_of_range &e) {
            return boost::contains(e.what(), "cannot decode any more values from this Kronecker decoder: the number of "
                                             "values already decoded is equal to the size used for construction (1)");
        });
        BOOST_CHECK_EXCEPTION(d0 >> out, std::out_of_range, [&l](const std::out_of_range &e) {
            return boost::contains(e.what(), "cannot decode any more values from this Kronecker decoder: the number of "
                                             "values already decoded is equal to the size used for construction (1)");
        });
    }
};

BOOST_AUTO_TEST_CASE(kronecker_stream_errors)
{
    tuple_for_each(int_types{}, stream_errors_tester{});
}

// Examples in the docs.
BOOST_AUTO_TEST_CASE(kronecker_doctests)
{
    {
        k_encoder<int> k(3);
        k << 1 << 2 << 3;
        auto code = k.get();
        (void)code;
    }
    {
        k_decoder<int> k(123, 3);
        int a, b, c;
        k >> a >> b >> c;
    }
    {
        int v[] = {7, 8, 9};
        auto code = k_encode<int>(v, 3);
        (void)code;
    }
    {
        int v[] = {7, 8, 9};
        auto code = k_encode<int>(v, v + 3);
        (void)code;
    }
    {
        std::vector<long> v{1, 2, 3};
        auto code = k_encode<long>(v);
        (void)code;
    }
    {
        int vec[3];
        k_decode(42, vec, 3);
    }
    {
        int vec[3];
        k_decode(42, vec, vec + 3);
    }
    {
        std::vector<int> vec(3);
        k_decode(42, vec);
        int arr[3];
        k_decode(42, arr);
        BOOST_CHECK(std::equal(arr, arr + 3, vec.begin()));
    }
}

template <typename T, typename... Args>
using k_encode_t = decltype(piranha::k_encode<T>(std::declval<Args>()...));

template <typename... Args>
using k_decode_t = decltype(piranha::k_decode(std::declval<Args>()...));

// Concepts and type traits.
BOOST_AUTO_TEST_CASE(kronecker_concepts_type_traits)
{
    // Uncv-ed C++ integral.
    BOOST_CHECK((is_uncv_cpp_signed_integral<int>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<int &>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<const int>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<volatile int>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<unsigned>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<void>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<std::string>::value));
    BOOST_CHECK((!is_uncv_cpp_signed_integral<double>::value));
    // Encodable iterator.
    BOOST_CHECK((is_k_encodable_iterator<std::vector<int>::const_iterator, long>::value));
    BOOST_CHECK((is_k_encodable_iterator<std::vector<int>::iterator, long>::value));
    BOOST_CHECK((is_k_encodable_iterator<int *, int>::value));
    BOOST_CHECK((is_k_encodable_iterator<const double *, int>::value));
    BOOST_CHECK((!is_k_encodable_iterator<std::vector<std::string>::const_iterator, long>::value));
    BOOST_CHECK((!is_k_encodable_iterator<int *, double>::value));
    BOOST_CHECK((!is_k_encodable_iterator<int *, void>::value));
    BOOST_CHECK((is_k_encodable_iterator<std::istream_iterator<int>, int>::value));
    BOOST_CHECK((!is_k_encodable_iterator<std::istream_iterator<std::string>, int>::value));
    BOOST_CHECK((!is_k_encodable_iterator<std::istream_iterator<int>, void>::value));
    // Encodable forward iterator.
    BOOST_CHECK((is_k_encodable_forward_iterator<std::vector<int>::const_iterator, long>::value));
    BOOST_CHECK((is_k_encodable_forward_iterator<std::vector<int>::iterator, long>::value));
    BOOST_CHECK((is_k_encodable_forward_iterator<int *, int>::value));
    BOOST_CHECK((is_k_encodable_forward_iterator<const double *, int>::value));
    BOOST_CHECK((!is_k_encodable_forward_iterator<std::vector<std::string>::const_iterator, long>::value));
    BOOST_CHECK((!is_k_encodable_forward_iterator<int *, double>::value));
    BOOST_CHECK((!is_k_encodable_forward_iterator<int *, void>::value));
    BOOST_CHECK((!is_k_encodable_forward_iterator<std::istream_iterator<int>, int>::value));
    BOOST_CHECK((!is_k_encodable_forward_iterator<std::istream_iterator<std::string>, int>::value));
    BOOST_CHECK((!is_k_encodable_forward_iterator<std::istream_iterator<int>, void>::value));
    // Encodable forward range.
    BOOST_CHECK((is_k_encodable_forward_range<std::vector<int>, long>::value));
    BOOST_CHECK((!is_k_encodable_forward_range<void, long>::value));
    BOOST_CHECK((!is_k_encodable_forward_range<std::vector<int>, void>::value));
    BOOST_CHECK((is_k_encodable_forward_range<int(&)[3], long>::value));
    BOOST_CHECK((!is_k_encodable_forward_range<std::vector<std::string>, long>::value));
    // Decodable forward iterator.
    BOOST_CHECK((is_k_decodable_forward_iterator<std::vector<int>::iterator, long>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<std::vector<int>::const_iterator, long>::value));
    BOOST_CHECK((is_k_decodable_forward_iterator<int *, int>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<const int *, int>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<double *, int>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<std::vector<std::string>::iterator, long>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<int *, double>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<int *, void>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<std::ostream_iterator<int>, int>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<std::ostream_iterator<std::string>, int>::value));
    BOOST_CHECK((!is_k_decodable_forward_iterator<std::istream_iterator<int>, void>::value));
    // Decodable forward range.
    BOOST_CHECK((is_k_decodable_forward_range<std::vector<int> &, long>::value));
    BOOST_CHECK((!is_k_decodable_forward_range<const std::vector<int> &, long>::value));
    BOOST_CHECK((!is_k_decodable_forward_range<void, long>::value));
    BOOST_CHECK((!is_k_decodable_forward_range<std::vector<int> &, void>::value));
    BOOST_CHECK((is_k_decodable_forward_range<int(&)[3], long>::value));
    BOOST_CHECK((!is_k_decodable_forward_range<const int(&)[3], long>::value));
    BOOST_CHECK((!is_k_decodable_forward_range<std::vector<std::string> &, long>::value));
    // Misc tests.
    BOOST_CHECK((is_detected<k_encode_t, int, int *, std::size_t>::value));
    BOOST_CHECK((is_detected<k_encode_t, int, int *, int *>::value));
    BOOST_CHECK((is_detected<k_encode_t, int, const std::vector<int> &>::value));
    BOOST_CHECK((!is_detected<k_encode_t, double, int *, std::size_t>::value));
    BOOST_CHECK((!is_detected<k_encode_t, int, std::string *, std::size_t>::value));
    BOOST_CHECK((!is_detected<k_encode_t, int, std::string *, std::string *>::value));
    BOOST_CHECK((!is_detected<k_encode_t, int, const std::vector<std::string> &>::value));
    BOOST_CHECK((is_detected<k_decode_t, int, int *, std::size_t>::value));
    BOOST_CHECK((is_detected<k_decode_t, int, int *, int *>::value));
    BOOST_CHECK((is_detected<k_decode_t, int, std::vector<int> &>::value));
    BOOST_CHECK((!is_detected<k_decode_t, int, const int *, std::size_t>::value));
    BOOST_CHECK((!is_detected<k_decode_t, int, const int *, const int *>::value));
    BOOST_CHECK((!is_detected<k_decode_t, int, const std::vector<int> &>::value));
    BOOST_CHECK((!is_detected<k_decode_t, double, int *, std::size_t>::value));
    BOOST_CHECK((!is_detected<k_decode_t, int, std::string *, std::string *>::value));
    BOOST_CHECK((!is_detected<k_decode_t, int, std::vector<std::string> &>::value));
}

// Coding/decoding.
struct coding_tester {
    template <typename T>
    void operator()(const T &) const
    {
        const auto &l = piranha::k_limits<T>();
        BOOST_CHECK(k_encode<T>(std::vector<std::int_least16_t>{}) == 0);
        BOOST_CHECK(k_encode<T>(std::vector<std::int_least16_t>{0}) == 0);
        BOOST_CHECK(k_encode<T>(std::vector<std::int_least16_t>{1}) == 1);
        BOOST_CHECK(k_encode<T>(std::vector<std::int_least16_t>{-1}) == -1);
        BOOST_CHECK(k_encode<T>(std::vector<std::int_least16_t>{-10}) == -10);
        BOOST_CHECK(k_encode<T>(std::vector<std::int_least16_t>{10}) == 10);
        // NOTE: static_cast for use when T is a char.
        const T emax1 = std::get<0u>(l[1u])[0u], emin1 = static_cast<T>(-emax1);
        BOOST_CHECK(k_encode<T>(std::vector<T>{emin1}) == emin1);
        BOOST_CHECK(k_encode<T>(std::vector<T>{emax1}) == emax1);
        std::mt19937 rng;
        // Test with max/min vectors in various sizes.
        for (decltype(l.size()) i = 1u; i < l.size(); ++i) {
            auto M = std::get<0u>(l[i]);
            auto m = M;
            for (auto it = m.begin(); it != m.end(); ++it) {
                *it = static_cast<T>(-(*it));
            }
            auto tmp(m);
            auto c = k_encode<T>(m);
            // Try also the size-based enc/dec overloads.
            BOOST_CHECK(c == k_encode<T>(m.begin(), m.size()));
            k_decode(c, tmp);
            BOOST_CHECK(m == tmp);
            k_decode(c, tmp.begin(), tmp.size());
            BOOST_CHECK(m == tmp);
            tmp = M;
            c = k_encode<T>(M);
            BOOST_CHECK(c == k_encode<T>(M.begin(), M.size()));
            k_decode(c, tmp);
            BOOST_CHECK(M == tmp);
            k_decode(c, tmp.begin(), tmp.size());
            BOOST_CHECK(M == tmp);
            auto v1 = std::vector<T>(i, 0);
            auto v2 = v1;
            c = k_encode<T>(v1);
            BOOST_CHECK(c == k_encode<T>(v1.begin(), v1.size()));
            k_decode(c, v1);
            BOOST_CHECK(v2 == v1);
            k_decode(c, v1.begin(), v1.size());
            BOOST_CHECK(v2 == v1);
            v1 = std::vector<T>(i, -1);
            v2 = v1;
            c = k_encode<T>(v1);
            BOOST_CHECK(c == k_encode<T>(v1.begin(), v1.size()));
            k_decode(c, v1);
            BOOST_CHECK(v2 == v1);
            k_decode(c, v1.begin(), v1.size());
            BOOST_CHECK(v2 == v1);
            // Test with random values within the bounds.
            for (auto j = 0; j < 10000; ++j) {
                for (decltype(v1.size()) k = 0u; k < v1.size(); ++k) {
                    std::uniform_int_distribution<long long> dist(m[k], M[k]);
                    v1[k] = static_cast<T>(dist(rng));
                }
                v2 = v1;
                c = k_encode<T>(v1);
                BOOST_CHECK(c == k_encode<T>(v1.begin(), v1.size()));
                k_decode(c, v1);
                BOOST_CHECK(v2 == v1);
                k_decode(c, v1.begin(), v1.size());
                BOOST_CHECK(v2 == v1);
            }
        }
        // Exceptions tests.
        BOOST_CHECK_THROW(k_encode<T>(std::vector<T>(l.size())), std::overflow_error);
        BOOST_CHECK_THROW(k_encode<T>(std::vector<T>{T(0), std::numeric_limits<T>::min()}), std::overflow_error);
        BOOST_CHECK_THROW(k_encode<T>(std::vector<T>{T(0), std::numeric_limits<T>::max()}), std::overflow_error);
        std::vector<T> v1;
        v1.resize(static_cast<typename std::vector<T>::size_type>(l.size()));
        BOOST_CHECK_THROW(k_decode(T(0), v1), std::overflow_error);
        v1.resize(0);
        BOOST_CHECK_THROW(k_decode(T(1), v1), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_coding_test)
{
    tuple_for_each(int_types{}, coding_tester{});
}
