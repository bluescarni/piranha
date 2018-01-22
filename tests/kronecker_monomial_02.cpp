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

#include <piranha/kronecker_monomial.hpp>

#define BOOST_TEST_MODULE kronecker_monomial_02_test
#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <initializer_list>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/s11n.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using int_types = std::tuple<signed char, int, long, long long>;

static const int ntries = 1000;

static std::mutex mut;

template <typename OArchive, typename IArchive, typename T>
static inline void boost_roundtrip(const T &x, const symbol_fset &args, bool mt = false)
{
    using w_type = boost_s11n_key_wrapper<T>;
    {
        std::stringstream ss;
        {
            OArchive oa(ss);
            boost_save(oa, w_type{x, args});
        }
        T retval;
        {
            IArchive ia(ss);
            w_type w{retval, args};
            boost_load(ia, w);
        }
        if (mt) {
            std::lock_guard<std::mutex> lock(mut);
            BOOST_CHECK(x == retval);
        } else {
            BOOST_CHECK(x == retval);
        }
    }
    // Try the boost api too.
    {
        std::stringstream ss;
        {
            OArchive oa(ss);
            w_type w{x, args};
            oa << w;
        }
        T retval;
        {
            IArchive ia(ss);
            w_type w{retval, args};
            ia >> w;
        }
        if (mt) {
            std::lock_guard<std::mutex> lock(mut);
            BOOST_CHECK(x == retval);
        } else {
            BOOST_CHECK(x == retval);
        }
    }
}

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using k_type = kronecker_monomial<T>;
        using w_type = boost_s11n_key_wrapper<k_type>;
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, w_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, w_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, w_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, w_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::xml_oarchive, w_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::xml_iarchive, w_type>::value));
        BOOST_CHECK((!has_boost_save<boost::archive::text_iarchive, w_type>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::text_oarchive, w_type>::value));
        BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive const, w_type>::value));
        BOOST_CHECK((!has_boost_save<void, w_type>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive const, w_type>::value));
        BOOST_CHECK((!has_boost_load<void, w_type>::value));
        const std::vector<std::string> names = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "l"};
        auto t_func = [&names](unsigned n) {
            std::uniform_int_distribution<unsigned> sdist(0, 10);
            std::uniform_int_distribution<int> edist(-10, 10);
            std::mt19937 rng(n);
            std::vector<T> expos;
            for (int i = 0; i < ntries; ++i) {
                auto s = sdist(rng);
                expos.resize(s);
                std::generate(expos.begin(), expos.end(), [&rng, &edist]() { return edist(rng); });
                k_type k;
                try {
                    k = k_type(expos);
                } catch (...) {
                    continue;
                }
                boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(
                    k, symbol_fset(names.begin(), names.begin() + s), true);
                boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                    k, symbol_fset(names.begin(), names.begin() + s), true);
            }
        };
        std::thread t0(t_func, 0);
        std::thread t1(t_func, 1);
        std::thread t2(t_func, 2);
        std::thread t3(t_func, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        // Test inconsistent size.
        {
            std::stringstream ss;
            {
                boost::archive::text_oarchive oa(ss);
                boost_save(oa, w_type{k_type{}, symbol_fset{}});
            }
            k_type retval{T(1), T(2)};
            {
                boost::archive::text_iarchive ia(ss);
                symbol_fset new_ss{"x"};
                w_type w{retval, new_ss};
                BOOST_CHECK_EXCEPTION(boost_load(ia, w), std::invalid_argument, [](const std::invalid_argument &iae) {
                    return boost::contains(
                        iae.what(),
                        "invalid size detected in the deserialization of a Kronercker "
                        "monomial: the deserialized size (0) differs from the size of the reference symbol set (1)");
                });
            }
            BOOST_CHECK((retval == k_type{T(1), T(2)}));
        }
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_boost_s11n_test)
{
    tuple_for_each(int_types{}, boost_s11n_tester());
}

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline void msgpack_roundtrip(const T &x, const symbol_fset &args, msgpack_format f, bool mt = false)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    x.msgpack_pack(p, f, args);
    T retval;
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    retval.msgpack_convert(oh.get(), f, args);
    if (mt) {
        std::lock_guard<std::mutex> lock(mut);
        BOOST_CHECK(x == retval);
    } else {
        BOOST_CHECK(x == retval);
    }
}

struct msgpack_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        BOOST_CHECK((key_has_msgpack_pack<msgpack::sbuffer, k_type>::value));
        BOOST_CHECK((!key_has_msgpack_pack<msgpack::sbuffer &, k_type>::value));
        BOOST_CHECK((!key_has_msgpack_pack<int, k_type>::value));
        BOOST_CHECK((!key_has_msgpack_pack<void, k_type>::value));
        BOOST_CHECK((key_has_msgpack_convert<k_type>::value));
        BOOST_CHECK((!key_has_msgpack_convert<k_type const &>::value));
        const std::vector<std::string> names = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "l"};
        auto t_func = [&names](unsigned n) {
            std::uniform_int_distribution<unsigned> sdist(0, 10);
            std::uniform_int_distribution<int> edist(-10, 10);
            std::mt19937 rng(n);
            std::vector<T> expos;
            for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                for (int i = 0; i < ntries; ++i) {
                    auto s = sdist(rng);
                    expos.resize(s);
                    std::generate(expos.begin(), expos.end(), [&rng, &edist]() { return edist(rng); });
                    k_type k;
                    try {
                        k = k_type(expos);
                    } catch (...) {
                        continue;
                    }
                    msgpack_roundtrip(k, symbol_fset(names.begin(), names.begin() + s), f, true);
                }
            }
        };
        std::thread t0(t_func, 0);
        std::thread t1(t_func, 1);
        std::thread t2(t_func, 2);
        std::thread t3(t_func, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        // Test inconsistent size.
        {
            msgpack::sbuffer sbuf;
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(1);
            msgpack_pack(p, T(1), msgpack_format::portable);
            k_type retval{T(2)};
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            BOOST_CHECK_EXCEPTION(
                retval.msgpack_convert(oh.get(), msgpack_format::portable, symbol_fset{}), std::invalid_argument,
                [](const std::invalid_argument &ia) {
                    return boost::contains(
                        ia.what(),
                        "invalid size detected in the deserialization of a Kronercker "
                        "monomial: the deserialized size (1) differs from the size of the reference symbol set (0)");
                });
            BOOST_CHECK((retval == k_type{T(2)}));
        }
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_msgpack_s11n_test)
{
    tuple_for_each(int_types{}, msgpack_s11n_tester());
}

#endif
