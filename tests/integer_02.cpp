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

#include <piranha/integer.hpp>

#define BOOST_TEST_MODULE integer_02_test
#include <boost/test/included/unit_test.hpp>

#include <atomic>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

#include <piranha/config.hpp>

#if defined(PIRANHA_WITH_BOOST_S11N)
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#endif

#include <mp++/integer.hpp>

#include <piranha/exceptions.hpp>
#include <piranha/s11n.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

static std::random_device rd;

// Small raii class for creating a tmp file.
// NOTE: this will not actually create the file, it will just create
// a tmp file name - so one is supposed to use the m_path member to create a file
// in the usual way. The destructor will attempt to delete the file at m_path, nothing
// will happen if the file does not exist.
struct tmp_file {
    tmp_file() : m_path(PIRANHA_BINARY_TESTS_DIR "/" + std::to_string(rd())) {}
    ~tmp_file() noexcept(false)
    {
        std::ifstream f(m_path.c_str());
        if (f.good() && std::remove(m_path.c_str())) {
            throw std::runtime_error("Error removing the temporary file '" + m_path + "'");
        }
    }
    std::string m_path;
};

static const std::vector<data_format> dfs = {data_format::boost_binary, data_format::boost_portable,
                                             data_format::msgpack_binary, data_format::msgpack_portable};

static const std::vector<compression> cfs
    = {compression::none, compression::bzip2, compression::zlib, compression::gzip};

template <typename T>
static inline T save_roundtrip(const T &x, data_format f, compression c)
{
    tmp_file file;
    save_file(x, file.m_path, f, c);
    T retval;
    load_file(retval, file.m_path, f, c);
    return retval;
}

constexpr int ntries = 1000;

constexpr int ntries_file = 100;

#if defined(PIRANHA_WITH_BOOST_S11N)

template <typename OArchive, typename IArchive, typename T>
static inline T boost_roundtrip(const T &x, bool promote = false)
{
    std::stringstream ss;
    {
        OArchive oa(ss);
        boost_save(oa, x);
    }
    T retval;
    if (promote) {
        retval.promote();
    }
    {
        IArchive ia(ss);
        boost_load(ia, retval);
    }
    return retval;
}

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const int_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, const int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::xml_oarchive &, int_type &>::value));
        BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const int_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, int_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, int_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::text_iarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, int_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::xml_iarchive, int_type>::value));
        BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, int_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const int_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive &, int_type &>::value));
        // A few checks with zero.
        BOOST_CHECK_EQUAL(
            (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(int_type{})),
            int_type{});
        int_type tmp;
        tmp.promote();
        BOOST_CHECK_EQUAL((boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp)),
                          int_type{});
        tmp = int_type{};
        BOOST_CHECK_EQUAL(
            (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp, true)), int_type{});
        tmp.promote();
        BOOST_CHECK_EQUAL(
            (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp, true)), int_type{});
        // Random multi-threaded testing.
        std::atomic<bool> status(true);
        auto checker = [&status](unsigned n) {
            std::mt19937 rng(static_cast<std::mt19937::result_type>(n));
            std::uniform_int_distribution<long long> dist(std::numeric_limits<long long>::min(),
                                                          std::numeric_limits<long long>::max());
            std::uniform_int_distribution<int> pdist(0, 1);
            for (int i = 0; i < ntries; ++i) {
                int_type cmp(dist(rng));
                if (pdist(rng) && cmp.is_static()) {
                    cmp.promote();
                }
                // Make it occupy a few mpz limbs, sometimes.
                if (pdist(rng)) {
                    cmp *= cmp;
                    cmp *= cmp;
                }
                // Randomly flip sign (useful if the code above was run, as that forces the value to be positive
                // because it squares the original value twice).
                if (pdist(rng)) {
                    cmp.neg();
                }
                auto tmp2 = boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                    cmp, pdist(rng));
                if (tmp2 != cmp) {
                    status.store(false);
                }
                tmp2 = boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(cmp, pdist(rng));
                if (tmp2 != cmp) {
                    status.store(false);
                }
            }
            // Try with small values as well.
            dist = std::uniform_int_distribution<long long>(-10, 10);
            for (int i = 0; i < ntries; ++i) {
                int_type cmp(dist(rng));
                if (pdist(rng) && cmp.is_static()) {
                    cmp.promote();
                }
                auto tmp2 = boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                    cmp, pdist(rng));
                if (tmp2 != cmp) {
                    status.store(false);
                }
                tmp2 = boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(cmp, pdist(rng));
                if (tmp2 != cmp) {
                    status.store(false);
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

BOOST_AUTO_TEST_CASE(integer_boost_s11n_test)
{
    tuple_for_each(size_types{}, boost_s11n_tester{});
}

#endif

struct save_load_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        std::atomic<bool> status(true);
        auto checker = [&status](unsigned n) {
            std::mt19937 rng(static_cast<std::mt19937::result_type>(n));
            std::uniform_int_distribution<long long> dist(std::numeric_limits<long long>::min(),
                                                          std::numeric_limits<long long>::max());
            std::uniform_int_distribution<int> pdist(0, 1);
            for (int i = 0; i < ntries_file; ++i) {
                for (auto f : dfs) {
                    for (auto c : cfs) {
                        int_type tmp(dist(rng));
                        if (pdist(rng) && tmp.is_static()) {
                            tmp.promote();
                        }
                        // Make it occupy a few mpz limbs, sometimes.
                        if (pdist(rng)) {
                            tmp *= tmp;
                            tmp *= tmp;
                        }
                        // Randomly flip sign (useful if the code above was run, as that forces the value to be positive
                        // because it squares the original value twice).
                        if (pdist(rng)) {
                            tmp.neg();
                        }
#if defined(PIRANHA_WITH_BOOST_S11N) && defined(PIRANHA_WITH_MSGPACK) && defined(PIRANHA_WITH_ZLIB)                    \
    && defined(PIRANHA_WITH_BZIP2)
                        // NOTE: we are not expecting any failure if we have all optional deps.
                        auto cmp = save_roundtrip(tmp, f, c);
                        if (cmp != tmp) {
                            status.store(false);
                        }
#else
                        // If msgpack or zlib are not available, we will have not_implemented_error
                        // failures.
                        try {
                            auto cmp = save_roundtrip(tmp, f, c);
                            if (cmp != tmp) {
                                status.store(false);
                            }
                        } catch (const not_implemented_error &) {
                            continue;
                        }
#endif
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

BOOST_AUTO_TEST_CASE(integer_save_load_test)
{
    tuple_for_each(size_types{}, save_load_tester{});
}

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline T msgpack_roundtrip(const T &x, msgpack_format f, bool promote = false)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, x, f);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    T retval;
    if (promote) {
        retval.promote();
    }
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

struct msgpack_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK((has_msgpack_pack<std::stringstream, int_type>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, int_type>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, int_type &>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const int_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<std::stringstream &, int_type>::value));
        BOOST_CHECK((!has_msgpack_pack<const std::stringstream, int_type>::value));
        BOOST_CHECK((!has_msgpack_pack<const std::stringstream &, int_type>::value));
        BOOST_CHECK((!has_msgpack_pack<int, int_type>::value));
        BOOST_CHECK((has_msgpack_convert<int_type>::value));
        BOOST_CHECK((has_msgpack_convert<int_type &>::value));
        BOOST_CHECK((!has_msgpack_convert<const int_type &>::value));
        // A few checks with zero.
        for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
            BOOST_CHECK_EQUAL((msgpack_roundtrip(int_type{}, f)), int_type{});
            int_type tmp;
            tmp.promote();
            BOOST_CHECK_EQUAL((msgpack_roundtrip(tmp, f)), int_type{});
            tmp = int_type{};
            BOOST_CHECK_EQUAL((msgpack_roundtrip(tmp, f)), int_type{});
            tmp.promote();
            BOOST_CHECK_EQUAL((msgpack_roundtrip(tmp, f)), int_type{});
        }
        // Random multi-threaded testing.
        std::atomic<bool> status(true);
        auto checker = [&status](unsigned n) {
            std::mt19937 rng(static_cast<std::mt19937::result_type>(n));
            std::uniform_int_distribution<long long> dist(std::numeric_limits<long long>::min(),
                                                          std::numeric_limits<long long>::max());
            std::uniform_int_distribution<int> pdist(0, 1);
            for (int i = 0; i < ntries; ++i) {
                for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                    int_type cmp(dist(rng));
                    if (pdist(rng) && cmp.is_static()) {
                        cmp.promote();
                    }
                    // Make it occupy a few mpz limbs, sometimes.
                    if (pdist(rng)) {
                        cmp *= cmp;
                        cmp *= cmp;
                    }
                    // Randomly flip sign (useful if the code above was run, as that forces the value to be positive
                    // because it squares the original value twice).
                    if (pdist(rng)) {
                        cmp.neg();
                    }
                    auto tmp = msgpack_roundtrip(cmp, f, pdist(rng));
                    if (tmp != cmp) {
                        status.store(false);
                    }
                }
            }
            // Small values.
            dist = std::uniform_int_distribution<long long>(-10, 10);
            for (int i = 0; i < ntries; ++i) {
                for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                    int_type cmp(dist(rng));
                    if (pdist(rng) && cmp.is_static()) {
                        cmp.promote();
                    }
                    auto tmp = msgpack_roundtrip(cmp, f, pdist(rng));
                    if (tmp != cmp) {
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

BOOST_AUTO_TEST_CASE(integer_msgpack_s11n_test)
{
    tuple_for_each(size_types{}, msgpack_s11n_tester{});
}

#endif
