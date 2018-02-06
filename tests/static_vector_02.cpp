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

#include <piranha/static_vector.hpp>

#define BOOST_TEST_MODULE static_vector_02_test
#include <boost/test/included/unit_test.hpp>

#include <cstddef>
#include <initializer_list>
#include <random>
#include <sstream>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/integer.hpp>
#include <piranha/rational.hpp>
#include <piranha/s11n.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using value_types = std::tuple<int, integer, rational>;
using size_types = std::tuple<std::integral_constant<std::size_t, 1u>, std::integral_constant<std::size_t, 5u>,
                              std::integral_constant<std::size_t, 10u>>;

static std::mt19937 rng;

#if defined(PIRANHA_WITH_BOOST_S11N) || defined(PIRANHA_WITH_MSGPACK)
static const int ntrials = 1000;
#endif

struct no_s11n {
};

BOOST_AUTO_TEST_CASE(static_vector_empty_test) {}

#if defined(PIRANHA_WITH_BOOST_S11N)

template <typename OArchive, typename IArchive, typename V>
static inline void boost_round_trip(const V &v)
{
    {
        std::stringstream ss;
        {
            OArchive oa(ss);
            boost_save(oa, v);
        }
        V retval;
        {
            IArchive ia(ss);
            boost_load(ia, retval);
        }
        BOOST_CHECK(retval == v);
    }
    // Check also boost api.
    {
        std::stringstream ss;
        {
            OArchive oa(ss);
            oa << v;
        }
        V retval;
        {
            IArchive ia(ss);
            ia >> retval;
        }
        BOOST_CHECK(retval == v);
    }
}

struct boost_s11n_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using vector_type = static_vector<T, U::value>;
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, vector_type>::value));
            BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, vector_type>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, vector_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, vector_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const vector_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, const vector_type &>::value));
            BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, vector_type &>::value));
            BOOST_CHECK((!has_boost_save<void, vector_type &>::value));
            BOOST_CHECK((!has_boost_save<const boost::archive::text_oarchive &, vector_type &>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, vector_type>::value));
            BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, vector_type>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, vector_type &>::value));
            BOOST_CHECK((has_boost_load<boost::archive::text_iarchive &, vector_type &>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const vector_type &>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive &, const vector_type &>::value));
            BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, vector_type &>::value));
            BOOST_CHECK((!has_boost_load<const boost::archive::text_iarchive &, vector_type &>::value));
            BOOST_CHECK((!has_boost_load<void, vector_type &>::value));
            std::uniform_int_distribution<unsigned> sdist(0u, U::value);
            std::uniform_int_distribution<int> edist(-10, 10);
            for (int i = 0; i < ntrials; ++i) {
                vector_type v;
                auto s = sdist(rng);
                for (decltype(s) j = 0; j < s; ++j) {
                    v.push_back(T(edist(rng)));
                    boost_round_trip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(v);
                    boost_round_trip<boost::archive::text_oarchive, boost::archive::text_iarchive>(v);
                }
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>());
    }
};

BOOST_AUTO_TEST_CASE(static_vector_boost_s11n_test)
{
    tuple_for_each(value_types{}, boost_s11n_tester());
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive, static_vector<no_s11n, 10u>>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::text_oarchive, static_vector<no_s11n, 10u>>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, static_vector<no_s11n, 10u>>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive, static_vector<no_s11n, 10u>>::value));
}

#endif

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline void msgpack_roundtrip(const T &v, msgpack_format f)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, v, f);
    T retval;
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    msgpack_convert(retval, oh.get(), f);
    BOOST_CHECK(v == retval);
}

struct msgpack_s11n_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using vector_type = static_vector<T, U::value>;
            BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, vector_type>::value));
            BOOST_CHECK((has_msgpack_pack<std::stringstream, vector_type>::value));
            BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, vector_type &>::value));
            BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const vector_type &>::value));
            BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, const vector_type &>::value));
            BOOST_CHECK((!has_msgpack_pack<void, const vector_type &>::value));
            BOOST_CHECK((has_msgpack_convert<vector_type>::value));
            BOOST_CHECK((has_msgpack_convert<vector_type &>::value));
            BOOST_CHECK((!has_msgpack_convert<const vector_type &>::value));
            std::uniform_int_distribution<unsigned> sdist(0u, U::value);
            std::uniform_int_distribution<int> edist(-10, 10);
            for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                for (int i = 0; i < ntrials; ++i) {
                    vector_type v;
                    auto s = sdist(rng);
                    for (decltype(s) j = 0; j < s; ++j) {
                        v.push_back(T(edist(rng)));
                        msgpack_roundtrip(v, f);
                    }
                }
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>());
    }
};

BOOST_AUTO_TEST_CASE(static_vector_msgpack_s11n_test)
{
    tuple_for_each(value_types{}, msgpack_s11n_tester());
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer, static_vector<no_s11n, 10u>>::value));
    BOOST_CHECK((!has_msgpack_pack<std::stringstream, static_vector<no_s11n, 10u>>::value));
    BOOST_CHECK((!has_msgpack_convert<static_vector<no_s11n, 10u>>::value));
}

#endif
