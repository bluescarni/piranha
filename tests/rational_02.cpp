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

#include <piranha/mp_rational.hpp>

#define BOOST_TEST_MODULE mp_rational_02_test
#include <boost/test/included/unit_test.hpp>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <random>
#include <sstream>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/init.hpp>
#include <piranha/s11n.hpp>

using namespace piranha;

using boost::archive::binary_oarchive;
using boost::archive::binary_iarchive;
using boost::archive::text_oarchive;
using boost::archive::text_iarchive;
using boost::archive::xml_oarchive;
using boost::archive::xml_iarchive;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

template <typename OArchive, typename IArchive, typename T>
static inline void boost_roundtrip(const T &x)
{
    std::stringstream ss;
    {
        OArchive oa(ss);
        boost_save(oa, x);
    }
    T retval;
    {
        IArchive ia(ss);
        boost_load(ia, retval);
    }
    BOOST_CHECK_EQUAL(retval, x);
}

static const int ntrials = 1000;

static std::mt19937 rng;

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mp_rational<T::value>;
        BOOST_CHECK((has_boost_save<binary_oarchive, q_type>::value));
        BOOST_CHECK((has_boost_save<binary_oarchive &, q_type>::value));
        BOOST_CHECK((has_boost_save<binary_oarchive &, q_type &>::value));
        BOOST_CHECK((has_boost_save<binary_oarchive &, const q_type &>::value));
        BOOST_CHECK((!has_boost_save<void, const q_type &>::value));
        BOOST_CHECK((!has_boost_save<const binary_oarchive &, const q_type &>::value));
        BOOST_CHECK((has_boost_save<xml_oarchive, q_type>::value));
        BOOST_CHECK((!has_boost_save<binary_iarchive, q_type>::value));
        BOOST_CHECK((has_boost_load<binary_iarchive, q_type>::value));
        BOOST_CHECK((has_boost_load<binary_iarchive &, q_type>::value));
        BOOST_CHECK((has_boost_load<binary_iarchive &, q_type &>::value));
        BOOST_CHECK((!has_boost_load<void, q_type &>::value));
        BOOST_CHECK((!has_boost_load<binary_iarchive &, const q_type &>::value));
        BOOST_CHECK((!has_boost_load<const binary_iarchive &, const q_type &>::value));
        BOOST_CHECK((has_boost_load<xml_iarchive, q_type>::value));
        BOOST_CHECK((!has_boost_load<binary_oarchive, q_type>::value));
        // A few simple checks.
        boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{});
        boost_roundtrip<text_oarchive, text_iarchive>(q_type{});
        boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{-1});
        boost_roundtrip<text_oarchive, text_iarchive>(q_type{23});
        boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{-1, 5});
        boost_roundtrip<text_oarchive, text_iarchive>(q_type{23, 67});
        // Random testing.
        std::uniform_int_distribution<int> dist(-1000, 1000);
        for (auto i = 0; i < ntrials; ++i) {
            auto num = dist(rng);
            auto den = dist(rng);
            if (den == 0) {
                continue;
            }
            boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{num, den});
            boost_roundtrip<text_oarchive, text_iarchive>(q_type{num, den});
        }
    }
};

BOOST_AUTO_TEST_CASE(mp_rational_boost_s11n_test)
{
    init();
    tuple_for_each(size_types{}, boost_s11n_tester());
}

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline T msgpack_roundtrip(const T &x, msgpack_format f)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, x, f);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

struct msgpack_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mp_rational<T::value>;
        using int_type = typename q_type::int_type;
        BOOST_CHECK((has_msgpack_pack<std::stringstream, q_type>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, q_type>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, q_type &>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, q_type &&>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const q_type>::value));
        BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, const q_type>::value));
        BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer, const q_type>::value));
        BOOST_CHECK((!has_msgpack_pack<int, const q_type>::value));
        BOOST_CHECK((!has_msgpack_pack<void, const q_type>::value));
        BOOST_CHECK((has_msgpack_convert<q_type>::value));
        BOOST_CHECK((has_msgpack_convert<q_type &>::value));
        BOOST_CHECK((!has_msgpack_convert<const q_type>::value));
        BOOST_CHECK((!has_msgpack_convert<const q_type &>::value));
        // A few simple checks.
        msgpack_roundtrip(q_type{}, msgpack_format::portable);
        msgpack_roundtrip(q_type{}, msgpack_format::binary);
        msgpack_roundtrip(q_type{-1}, msgpack_format::binary);
        msgpack_roundtrip(q_type{23}, msgpack_format::portable);
        msgpack_roundtrip(q_type{-1, 5}, msgpack_format::binary);
        msgpack_roundtrip(q_type{23, 67}, msgpack_format::portable);
        // Random testing.
        std::uniform_int_distribution<int> dist(-1000, 1000);
        for (auto i = 0; i < ntrials; ++i) {
            auto num = dist(rng);
            auto den = dist(rng);
            if (den == 0) {
                continue;
            }
            msgpack_roundtrip(q_type{num, den}, msgpack_format::binary);
            msgpack_roundtrip(q_type{num, den}, msgpack_format::portable);
        }
        {
            // Check bogus rationals with portable format.
            msgpack::sbuffer sbuf;
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, int_type{2}, msgpack_format::portable);
            msgpack_pack(p, int_type{-2}, msgpack_format::portable);
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            q_type q;
            msgpack_convert(q, oh.get(), msgpack_format::portable);
            BOOST_CHECK_EQUAL(q, -1);
        }
        {
            // Check exception.
            msgpack::sbuffer sbuf;
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, int_type{2}, msgpack_format::portable);
            msgpack_pack(p, int_type{0}, msgpack_format::portable);
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            q_type q{42};
            BOOST_CHECK_THROW(msgpack_convert(q, oh.get(), msgpack_format::portable), zero_division_error);
            BOOST_CHECK_EQUAL(q, 42);
        }
        {
            // Explicitly canonicalise the value when using binary format.
            msgpack::sbuffer sbuf;
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, int_type{-4}, msgpack_format::binary);
            msgpack_pack(p, int_type{-2}, msgpack_format::binary);
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            q_type q;
            msgpack_convert(q, oh.get(), msgpack_format::binary);
            q.canonicalise();
            BOOST_CHECK_EQUAL(q, 2);
        }
    }
};

BOOST_AUTO_TEST_CASE(mp_rational_msgpack_s11n_test)
{
    tuple_for_each(size_types{}, msgpack_s11n_tester());
}

#endif
