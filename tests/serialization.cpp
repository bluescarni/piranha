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

#include "../src/serialization.hpp"

#define BOOST_TEST_MODULE serialization_test
#include <boost/test/unit_test.hpp>

#include <sstream>

#include "../src/config.hpp"
#include "../src/init.hpp"

using namespace piranha;

using msgpack::packer;
using msgpack::sbuffer;

BOOST_AUTO_TEST_CASE(serialization_boost_test_00)
{
    // Placeholder for now.
    init();
}

#if defined(PIRANHA_ENABLE_MSGPACK)

#include <algorithm>
#include <atomic>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/config.hpp"
#include "../src/is_key.hpp"
#include "../src/symbol_set.hpp"

using integral_types = boost::mpl::vector<char, signed char, short, int, long, long long, unsigned char, unsigned short,
                                          unsigned, unsigned long, unsigned long long>;

using fp_types = boost::mpl::vector<float, double, long double>;

template <typename T>
using sw = detail::msgpack_stream_wrapper<T>;

// A struct with no msgpack support.
struct no_msgpack {
};

// A key with msgpack support.
struct key01 {
    key01() = default;
    key01(const key01 &) = default;
    key01(key01 &&) noexcept;
    key01 &operator=(const key01 &) = default;
    key01 &operator=(key01 &&) noexcept;
    key01(const symbol_set &);
    bool operator==(const key01 &) const;
    bool operator!=(const key01 &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    key01 merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, key01>> t_subs(const std::string &, const T &, const U &,
                                                      const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    key01 trim(const symbol_set &, const symbol_set &) const;
    template <typename Stream>
    int msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format, const symbol_set &) const;
    int msgpack_convert(const msgpack::object &, msgpack_format, const symbol_set &);
};

// A key without msgpack support.
struct key02 {
    key02() = default;
    key02(const key02 &) = default;
    key02(key02 &&) noexcept;
    key02 &operator=(const key02 &) = default;
    key02 &operator=(key02 &&) noexcept;
    key02(const symbol_set &);
    bool operator==(const key02 &) const;
    bool operator!=(const key02 &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    key02 merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, key02>> t_subs(const std::string &, const T &, const U &,
                                                      const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    key02 trim(const symbol_set &, const symbol_set &) const;
    template <typename Stream>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format, const symbol_set &);
    template <typename Stream>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format) const;
    int msgpack_convert(msgpack::object &, msgpack_format, const symbol_set &);
    int msgpack_convert(const msgpack::object &, msgpack_format);
};

namespace std
{

template <>
struct hash<key01> {
    std::size_t operator()(const key01 &) const;
};

template <>
struct hash<key02> {
    std::size_t operator()(const key02 &) const;
};
}

static const int ntrials = 1000;

// Helper function to roundtrip the conversion to/from msgpack for type T.
template <typename T>
static inline T msgpack_roundtrip(const T &x, msgpack_format f)
{
    sbuffer sbuf;
    packer<sbuffer> p(sbuf);
    msgpack_pack(p, x, f);
    std::size_t offset = 0u;
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size(), offset);
    piranha_assert(offset == sbuf.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

// As above, but using stringstream as backend.
template <typename T>
static inline T msgpack_roundtrip_sstream(const T &x, msgpack_format f)
{
    sw<std::stringstream> oss;
    packer<sw<std::stringstream>> p(oss);
    msgpack_pack(p, x, f);
    std::vector<char> vec;
    std::copy(std::istreambuf_iterator<char>(oss), std::istreambuf_iterator<char>(), std::back_inserter(vec));
    std::size_t offset = 0u;
    auto oh = msgpack::unpack(vec.data(), vec.size(), offset);
    piranha_assert(offset == vec.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

BOOST_AUTO_TEST_CASE(serialization_msgpack_tt_test)
{
    BOOST_CHECK(is_msgpack_stream<std::ostringstream>::value);
    BOOST_CHECK(!is_msgpack_stream<std::ostringstream &>::value);
    BOOST_CHECK(!is_msgpack_stream<const std::ostringstream &>::value);
    BOOST_CHECK(!is_msgpack_stream<const std::ostringstream>::value);
    BOOST_CHECK(is_msgpack_stream<sbuffer>::value);
    BOOST_CHECK(!is_msgpack_stream<float>::value);
    BOOST_CHECK(!is_msgpack_stream<const double>::value);
    BOOST_CHECK(is_msgpack_stream<sw<std::ostringstream>>::value);
    BOOST_CHECK(!is_msgpack_stream<sw<std::ostringstream> &>::value);
    BOOST_CHECK((has_msgpack_pack<sbuffer, int>::value));
    BOOST_CHECK((!has_msgpack_pack<sbuffer, no_msgpack>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, int>::value));
    BOOST_CHECK((has_msgpack_pack<sw<std::ostringstream>, int>::value));
    BOOST_CHECK((!has_msgpack_pack<sbuffer &, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream &, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream &&, int>::value));
    BOOST_CHECK((!has_msgpack_pack<std::ostringstream &&, int>::value));
    BOOST_CHECK((has_msgpack_convert<int>::value));
    BOOST_CHECK((has_msgpack_convert<double>::value));
    BOOST_CHECK((has_msgpack_convert<int &>::value));
    BOOST_CHECK((has_msgpack_convert<double &>::value));
    BOOST_CHECK((!has_msgpack_convert<no_msgpack>::value));
    BOOST_CHECK((!has_msgpack_convert<const int>::value));
    BOOST_CHECK((!has_msgpack_convert<const double>::value));
    BOOST_CHECK((has_msgpack_convert<int &&>::value));
    BOOST_CHECK((has_msgpack_convert<double &&>::value));
    BOOST_CHECK((!has_msgpack_convert<const int &&>::value));
    BOOST_CHECK((!has_msgpack_convert<const double &&>::value));
    BOOST_CHECK(is_key<key01>::value);
    BOOST_CHECK((key_has_msgpack_pack<sbuffer, key01>::value));
    BOOST_CHECK((!key_has_msgpack_pack<sbuffer &, key01>::value));
    BOOST_CHECK((!key_has_msgpack_pack<const sbuffer, key01>::value));
    BOOST_CHECK(is_key<key02>::value);
    BOOST_CHECK((!key_has_msgpack_pack<sbuffer, key02>::value));
    BOOST_CHECK((!key_has_msgpack_convert<key02>::value));
}

struct int_tester {
    template <typename T>
    void operator()(const T &) const
    {
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto f : {0, 1}) {
                for (auto i = 0; i < ntrials; ++i) {
                    const auto tmp = dist(eng);
                    auto cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                    cmp = msgpack_roundtrip_sstream(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
    }
};

BOOST_AUTO_TEST_CASE(serialization_test_msgpack_int)
{
    boost::mpl::for_each<integral_types>(int_tester());
}

struct fp_tester {
    template <typename T>
    void operator()(const T &) const
    {
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_real_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto f : {0, 1}) {
                for (auto i = 0; i < ntrials; ++i) {
                    const auto tmp = dist(eng);
                    auto cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                    cmp = msgpack_roundtrip_sstream(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
        // Additional checking for non-finite values.
        if (std::numeric_limits<T>::has_quiet_NaN && std::numeric_limits<T>::has_infinity) {
            for (auto f : {0, 1}) {
                auto tmp = std::numeric_limits<T>::quiet_NaN();
                tmp = std::copysign(tmp, T(1.));
                auto cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isnan(cmp));
                BOOST_CHECK(!std::signbit(cmp));
                tmp = std::copysign(tmp, T(-1.));
                cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isnan(cmp));
                BOOST_CHECK(std::signbit(cmp));
                tmp = std::numeric_limits<T>::infinity();
                cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isinf(cmp));
                BOOST_CHECK(!std::signbit(cmp));
                tmp = std::numeric_limits<T>::infinity();
                tmp = std::copysign(tmp, T(-1.));
                cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isinf(cmp));
                BOOST_CHECK(std::signbit(cmp));
            }
        }
        if (std::is_same<T, long double>::value) {
            // Check that a malformed string in the portable serialization of long double
            // raises the appropriate exception.
            sbuffer sbuf;
            packer<sbuffer> p(sbuf);
            p.pack("hello world");
            std::size_t offset = 0;
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size(), offset);
            long double tmp;
            BOOST_CHECK_THROW(msgpack_convert(tmp, oh.get(), msgpack_format::portable), std::invalid_argument);
        }
    }
};

BOOST_AUTO_TEST_CASE(serialization_test_msgpack_float)
{
    boost::mpl::for_each<fp_types>(fp_tester());
}

#endif
