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

#include "../src/monomial.hpp"

#define BOOST_TEST_MODULE monomial_02_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/is_key.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/s11n.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

using expo_types = std::tuple<signed char, int, integer, rational>;
using size_types = std::tuple<std::integral_constant<std::size_t, 0u>, std::integral_constant<std::size_t, 1u>,
                              std::integral_constant<std::size_t, 5u>, std::integral_constant<std::size_t, 10u>>;

static const int ntrials = 100;

static std::mt19937 rng;

// This is missing the s11n methods.
struct fake_int_01 {
    fake_int_01();
    explicit fake_int_01(int);
    fake_int_01(const fake_int_01 &);
    fake_int_01(fake_int_01 &&) noexcept;
    fake_int_01 &operator=(const fake_int_01 &);
    fake_int_01 &operator=(fake_int_01 &&) noexcept;
    ~fake_int_01();
    bool operator==(const fake_int_01 &) const;
    bool operator!=(const fake_int_01 &) const;
    bool operator<(const fake_int_01 &) const;
    fake_int_01 operator+(const fake_int_01 &) const;
    fake_int_01 &operator+=(const fake_int_01 &);
    fake_int_01 operator-(const fake_int_01 &) const;
    fake_int_01 &operator-=(const fake_int_01 &);
    friend std::ostream &operator<<(std::ostream &, const fake_int_01 &);
};

namespace std
{

template <>
struct hash<fake_int_01> {
    typedef size_t result_type;
    typedef fake_int_01 argument_type;
    result_type operator()(const argument_type &) const;
};
}

namespace piranha
{

namespace math
{

template <>
struct negate_impl<fake_int_01> {
    void operator()(fake_int_01 &) const;
};
}
}

template <typename OArchive, typename IArchive, typename Monomial>
static inline Monomial boost_round_trip_monomial(const Monomial &m, const symbol_set &s)
{
    using w_type = boost_s11n_key_wrapper<Monomial>;
    std::stringstream ss;
    {
        OArchive oa(ss);
        boost_save(oa, w_type{m, s});
    }
    Monomial n;
    {
        IArchive ia(ss);
        w_type w{n, s};
        boost_load(ia, w);
    }
    return n;
}

struct boost_s11n_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using monomial_type = monomial<T, U>;
            using w_type = boost_s11n_key_wrapper<monomial_type>;
            // Test the type traits.
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, w_type>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, w_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, const w_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, const w_type>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, w_type>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, w_type &>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, w_type &&>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const w_type &>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const w_type>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, w_type>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, w_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, w_type &&>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, w_type>::value));
            BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, w_type>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, w_type>::value));
            BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive, w_type>::value));
            BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive, w_type>::value));
            BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, w_type>::value));
            BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, w_type>::value));
            BOOST_CHECK((!has_boost_save<int, w_type>::value));
            BOOST_CHECK((!has_boost_load<int, w_type>::value));
            // Check exceptions.
            symbol_set s{symbol{"a"}};
            monomial_type m;
            std::stringstream ss;
            {
                boost::archive::text_oarchive oa(ss);
                BOOST_CHECK_EXCEPTION(
                    boost_save(oa, w_type{m, s}), std::invalid_argument, [](const std::invalid_argument &ia) {
                        return boost::contains(
                            ia.what(),
                            "incompatible symbol set in monomial serialization: the reference "
                            "symbol set has a size of 1, while the monomial being serialized has a size of 0");
                    });
            }
            ss.str("");
            ss.clear();
            m = monomial_type{T(1)};
            {
                boost::archive::text_oarchive oa(ss);
                boost_save(oa, w_type{m, s});
            }
            {
                boost::archive::text_iarchive ia(ss);
                symbol_set s2;
                w_type w{m, s2};
                BOOST_CHECK_EXCEPTION(boost_load(ia, w), std::invalid_argument, [](const std::invalid_argument &iae) {
                    return boost::contains(
                        iae.what(),
                        "incompatible symbol set in monomial serialization: the reference "
                        "symbol set has a size of 0, while the monomial being deserialized has a size of 1");
                });
            }
            // A few simple tests.
            m = monomial_type{};
            auto n = boost_round_trip_monomial<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                m, symbol_set{});
            BOOST_CHECK(n == m);
            n = boost_round_trip_monomial<boost::archive::text_oarchive, boost::archive::text_iarchive>(m,
                                                                                                        symbol_set{});
            BOOST_CHECK(n == m);
            std::vector<T> vexpo = {T(1), T(2), T(3)};
            m = monomial_type(vexpo.begin(), vexpo.end());
            n = boost_round_trip_monomial<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                m, symbol_set{symbol{"a"}, symbol{"b"}, symbol{"c"}});
            BOOST_CHECK(n == m);
            n = boost_round_trip_monomial<boost::archive::text_oarchive, boost::archive::text_iarchive>(
                m, symbol_set{symbol{"a"}, symbol{"b"}, symbol{"c"}});
            BOOST_CHECK(n == m);
            // Random testing.
            random_test<U>();
        }
        template <typename U, typename V = T, typename std::enable_if<std::is_same<V, integer>::value, int>::type = 0>
        void random_test() const
        {
            using monomial_type = monomial<T, U>;
            using size_type = typename monomial_type::size_type;
            std::uniform_int_distribution<size_type> sdist(0u, 10u);
            std::uniform_int_distribution<int> edist(-10, 10);
            const std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
            for (auto i = 0; i < ntrials; ++i) {
                const auto size = sdist(rng);
                std::vector<T> tmp;
                for (size_type j = 0; j < size; ++j) {
                    tmp.emplace_back(edist(rng));
                }
                monomial_type m(tmp.begin(), tmp.end());
                symbol_set ss(vs.begin(), vs.begin() + size);
                BOOST_CHECK(
                    (m == boost_round_trip_monomial<boost::archive::text_oarchive, boost::archive::text_iarchive>(m,
                                                                                                                  ss)));
                BOOST_CHECK(
                    (m == boost_round_trip_monomial<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                              m, ss)));
            }
        }
        template <typename U, typename V = T, typename std::enable_if<std::is_same<V, rational>::value, int>::type = 0>
        void random_test() const
        {
            using monomial_type = monomial<T, U>;
            using size_type = typename monomial_type::size_type;
            std::uniform_int_distribution<size_type> sdist(0u, 10u);
            std::uniform_int_distribution<int> edist(-10, 10);
            const std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
            for (auto i = 0; i < ntrials; ++i) {
                const auto size = sdist(rng);
                std::vector<T> tmp;
                for (size_type j = 0; j < size; ++j) {
                    int num = edist(rng), den = edist(rng);
                    if (!den) {
                        den = 1;
                    }
                    tmp.emplace_back(num, den);
                }
                monomial_type m(tmp.begin(), tmp.end());
                symbol_set ss(vs.begin(), vs.begin() + size);
                BOOST_CHECK(
                    (m == boost_round_trip_monomial<boost::archive::text_oarchive, boost::archive::text_iarchive>(m,
                                                                                                                  ss)));
                BOOST_CHECK(
                    (m == boost_round_trip_monomial<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                              m, ss)));
            }
        }
        template <typename U, typename V = T, typename std::enable_if<std::is_integral<V>::value, int>::type = 0>
        void random_test() const
        {
            using monomial_type = monomial<T, U>;
            using size_type = typename monomial_type::size_type;
            std::uniform_int_distribution<size_type> sdist(0u, 10u);
            std::uniform_int_distribution<T> edist(-10, 10);
            const std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
            for (auto i = 0; i < ntrials; ++i) {
                const auto size = sdist(rng);
                std::vector<T> tmp;
                for (size_type j = 0; j < size; ++j) {
                    tmp.push_back(edist(rng));
                }
                monomial_type m(tmp.begin(), tmp.end());
                symbol_set ss(vs.begin(), vs.begin() + size);
                BOOST_CHECK(
                    (m == boost_round_trip_monomial<boost::archive::text_oarchive, boost::archive::text_iarchive>(m,
                                                                                                                  ss)));
                BOOST_CHECK(
                    (m == boost_round_trip_monomial<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                              m, ss)));
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>());
    }
};

BOOST_AUTO_TEST_CASE(monomial_boost_s11n_test)
{
    init();
    tuple_for_each(expo_types{}, boost_s11n_tester());
    BOOST_CHECK((is_key<monomial<fake_int_01>>::value));
    BOOST_CHECK(
        (!has_boost_save<boost::archive::binary_oarchive, boost_s11n_key_wrapper<monomial<fake_int_01>>>::value));
    BOOST_CHECK(
        (!has_boost_load<boost::archive::binary_iarchive, boost_s11n_key_wrapper<monomial<fake_int_01>>>::value));
}

#if defined(PIRANHA_WITH_MSGPACK)

#include <algorithm>
#include <atomic>
#include <iterator>
#include <thread>

using msgpack::sbuffer;
using msgpack::packer;

template <typename T>
using sw = msgpack_stream_wrapper<T>;

template <typename Monomial>
static inline Monomial msgpack_round_trip_monomial(const Monomial &m, const symbol_set &s, msgpack_format f)
{
    sbuffer sbuf;
    packer<sbuffer> p(sbuf);
    m.msgpack_pack(p, f, s);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    Monomial n;
    n.msgpack_convert(oh.get(), f, s);
    return n;
}

template <typename Monomial>
static inline Monomial msgpack_round_trip_monomial_ss(const Monomial &m, const symbol_set &s, msgpack_format f)
{
    sw<std::stringstream> ss;
    packer<sw<std::stringstream>> p(ss);
    m.msgpack_pack(p, f, s);
    std::vector<char> vchar;
    std::copy(std::istreambuf_iterator<char>(ss), std::istreambuf_iterator<char>(), std::back_inserter(vchar));
    auto oh = msgpack::unpack(vchar.data(), static_cast<std::size_t>(vchar.size()));
    Monomial n;
    n.msgpack_convert(oh.get(), f, s);
    return n;
}

struct msgpack_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using monomial_type = monomial<T, U>;
            BOOST_CHECK((key_has_msgpack_pack<msgpack::sbuffer, monomial_type>::value));
            BOOST_CHECK((key_has_msgpack_pack<std::ostringstream, monomial_type>::value));
            BOOST_CHECK((key_has_msgpack_pack<std::ostringstream, monomial_type &>::value));
            BOOST_CHECK((key_has_msgpack_pack<std::ostringstream, const monomial_type &>::value));
            BOOST_CHECK((key_has_msgpack_pack<std::ostringstream, const monomial_type>::value));
            BOOST_CHECK((key_has_msgpack_pack<sw<std::ostringstream>, monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<msgpack::sbuffer &, monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<int, monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<const std::ostringstream, monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<const std::ostringstream &&, monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<std::ostringstream &&, monomial_type>::value));
            BOOST_CHECK((key_has_msgpack_convert<monomial_type>::value));
            BOOST_CHECK((key_has_msgpack_convert<monomial_type &>::value));
            BOOST_CHECK((key_has_msgpack_convert<monomial_type &&>::value));
            BOOST_CHECK((!key_has_msgpack_convert<const monomial_type &>::value));
            BOOST_CHECK((!key_has_msgpack_convert<const monomial_type>::value));
            // Some simple checks.
            for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                BOOST_CHECK((msgpack_round_trip_monomial(monomial_type{}, symbol_set{}, f) == monomial_type{}));
                BOOST_CHECK((msgpack_round_trip_monomial_ss(monomial_type{}, symbol_set{}, f) == monomial_type{}));
                monomial_type m{T(1), T(2)};
                symbol_set s{symbol{"a"}, symbol{"b"}};
                BOOST_CHECK((msgpack_round_trip_monomial(m, s, f) == m));
                BOOST_CHECK((msgpack_round_trip_monomial_ss(m, s, f) == m));
                // Test exceptions.
                sbuffer sbuf;
                packer<sbuffer> p(sbuf);
                BOOST_CHECK_EXCEPTION(
                    m.msgpack_pack(p, f, symbol_set{}), std::invalid_argument, [](const std::invalid_argument &ia) {
                        return boost::contains(
                            ia.what(),
                            "incompatible symbol set in monomial serialization: the reference "
                            "symbol set has a size of 0, while the monomial being serialized has a size of 2");
                    });
                m.msgpack_pack(p, f, s);
                auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
                BOOST_CHECK_EXCEPTION(
                    m.msgpack_convert(oh.get(), f, symbol_set{}), std::invalid_argument,
                    [](const std::invalid_argument &ia) {
                        return boost::contains(
                            ia.what(),
                            "incompatible symbol set in monomial serialization: the reference "
                            "symbol set has a size of 0, while the monomial being deserialized has a size of 2");
                    });
            }
            // Random checks.
            random_test<U>();
        }
        template <typename U, typename V = T, typename std::enable_if<std::is_same<V, integer>::value, int>::type = 0>
        void random_test() const
        {
            using monomial_type = monomial<T, U>;
            using size_type = typename monomial_type::size_type;
            std::atomic<bool> flag(true);
            auto checker = [&flag](unsigned n) {
                std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
                std::uniform_int_distribution<size_type> sdist(0u, 10u);
                std::uniform_int_distribution<int> edist(-10, 10);
                const std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
                for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                    for (auto i = 0; i < ntrials; ++i) {
                        const auto size = sdist(eng);
                        std::vector<T> tmp;
                        for (size_type j = 0; j < size; ++j) {
                            tmp.emplace_back(edist(eng));
                        }
                        monomial_type m(tmp.begin(), tmp.end());
                        symbol_set ss(vs.begin(), vs.begin() + size);
                        if (m != msgpack_round_trip_monomial(m, ss, f)) {
                            flag.store(false);
                        }
                        if (m != msgpack_round_trip_monomial_ss(m, ss, f)) {
                            flag.store(false);
                        }
                    }
                }
            };
            std::thread t0(checker, 0);
            std::thread t1(checker, 1);
            std::thread t2(checker, 2);
            std::thread t3(checker, 3);
            t0.join();
            t1.join();
            t2.join();
            t3.join();
            BOOST_CHECK(flag.load());
        }
        template <typename U, typename V = T, typename std::enable_if<std::is_same<V, rational>::value, int>::type = 0>
        void random_test() const
        {
            using monomial_type = monomial<T, U>;
            using size_type = typename monomial_type::size_type;
            std::atomic<bool> flag(true);
            auto checker = [&flag](unsigned n) {
                std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
                std::uniform_int_distribution<size_type> sdist(0u, 10u);
                std::uniform_int_distribution<int> edist(-10, 10);
                const std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
                for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                    for (auto i = 0; i < ntrials; ++i) {
                        const auto size = sdist(eng);
                        std::vector<T> tmp;
                        for (size_type j = 0; j < size; ++j) {
                            int num = edist(eng), den = edist(eng);
                            if (!den) {
                                den = 1;
                            }
                            tmp.emplace_back(num, den);
                        }
                        monomial_type m(tmp.begin(), tmp.end());
                        symbol_set ss(vs.begin(), vs.begin() + size);
                        if (m != msgpack_round_trip_monomial(m, ss, f)) {
                            flag.store(false);
                        }
                        if (m != msgpack_round_trip_monomial_ss(m, ss, f)) {
                            flag.store(false);
                        }
                    }
                }
            };
            std::thread t0(checker, 0);
            std::thread t1(checker, 1);
            std::thread t2(checker, 2);
            std::thread t3(checker, 3);
            t0.join();
            t1.join();
            t2.join();
            t3.join();
            BOOST_CHECK(flag.load());
        }
        template <typename U, typename V = T, typename std::enable_if<std::is_integral<V>::value, int>::type = 0>
        void random_test() const
        {
            using monomial_type = monomial<T, U>;
            using size_type = typename monomial_type::size_type;
            std::atomic<bool> flag(true);
            auto checker = [&flag](unsigned n) {
                std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
                std::uniform_int_distribution<size_type> sdist(0u, 10u);
                std::uniform_int_distribution<T> edist(-10, 10);
                const std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
                for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                    for (auto i = 0; i < ntrials; ++i) {
                        const auto size = sdist(eng);
                        std::vector<T> tmp;
                        for (size_type j = 0; j < size; ++j) {
                            tmp.push_back(edist(eng));
                        }
                        monomial_type m(tmp.begin(), tmp.end());
                        symbol_set ss(vs.begin(), vs.begin() + size);
                        if (m != msgpack_round_trip_monomial(m, ss, f)) {
                            flag.store(false);
                        }
                        if (m != msgpack_round_trip_monomial_ss(m, ss, f)) {
                            flag.store(false);
                        }
                    }
                }
            };
            std::thread t0(checker, 0);
            std::thread t1(checker, 1);
            std::thread t2(checker, 2);
            std::thread t3(checker, 3);
            t0.join();
            t1.join();
            t2.join();
            t3.join();
            BOOST_CHECK(flag.load());
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>());
    }
};

BOOST_AUTO_TEST_CASE(monomial_msgpack_test)
{
    tuple_for_each(expo_types{}, msgpack_tester());
    BOOST_CHECK((!key_has_msgpack_pack<msgpack::sbuffer, monomial<fake_int_01>>::value));
    BOOST_CHECK((!key_has_msgpack_pack<std::ostringstream, monomial<fake_int_01>>::value));
    BOOST_CHECK((!key_has_msgpack_pack<sw<std::ostringstream>, monomial<fake_int_01>>::value));
    BOOST_CHECK((!key_has_msgpack_convert<monomial<fake_int_01>>::value));
}

#endif
