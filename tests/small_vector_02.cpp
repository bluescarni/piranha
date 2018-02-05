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

#include <piranha/small_vector.hpp>

#define BOOST_TEST_MODULE small_vector_02_test
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

static const int ntries = 1000;
static std::mt19937 rng;

using namespace piranha;

using value_types = std::tuple<signed char, short, int, long, long long, integer, rational>;
using size_types = std::tuple<std::integral_constant<std::size_t, 0u>, std::integral_constant<std::size_t, 1u>,
                              std::integral_constant<std::size_t, 5u>, std::integral_constant<std::size_t, 10u>>;

struct no_s11n {
};

BOOST_AUTO_TEST_CASE(small_vector_empty_test) {}

#if defined(PIRANHA_WITH_BOOST_S11N)

template <typename OArchive, typename IArchive, typename V>
static inline void boost_round_trip(const V &v)
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

struct boost_s11n_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using v_type = small_vector<T, U>;
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, v_type>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, small_vector<v_type, U>>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, v_type &>::value));
            BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, const v_type &>::value));
            BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive const, const v_type &>::value));
            BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, const v_type &>::value));
            BOOST_CHECK((!has_boost_save<void, v_type>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, v_type>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, small_vector<v_type, U>>::value));
            BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, v_type &>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const v_type &>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive const, const v_type &>::value));
            BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, v_type>::value));
            BOOST_CHECK((!has_boost_load<void, v_type>::value));
            using size_type = typename v_type::size_type;
            std::uniform_int_distribution<unsigned> sdist(0u, 20u);
            std::uniform_int_distribution<int> edist(-10, 10);
            for (int i = 0; i < ntries; ++i) {
                auto size = static_cast<size_type>(sdist(rng));
                v_type v;
                for (decltype(size) j = 0; j < size; ++j) {
                    v.push_back(T(edist(rng)));
                }
                boost_round_trip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(v);
                boost_round_trip<boost::archive::text_oarchive, boost::archive::text_iarchive>(v);
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(small_vector_boost_s11n_test)
{
    tuple_for_each(value_types{}, boost_s11n_tester{});
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive, small_vector<no_s11n>>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, small_vector<no_s11n>>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive, small_vector<small_vector<no_s11n>>>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, small_vector<small_vector<no_s11n>>>::value));
}

#endif

#if defined(PIRANHA_WITH_MSGPACK)

using namespace msgpack;

template <typename Vector>
static inline void msgpack_round_trip(const Vector &v, msgpack_format f)
{
    sbuffer sbuf;
    packer<sbuffer> p(sbuf);
    msgpack_pack(p, v, f);
    auto oh = unpack(sbuf.data(), sbuf.size());
    Vector retval;
    msgpack_convert(retval, oh.get(), f);
    BOOST_CHECK(v == retval);
}

struct msgpack_s11n_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using v_type = small_vector<T, U>;
            BOOST_CHECK((has_msgpack_pack<sbuffer, v_type>::value));
            BOOST_CHECK((has_msgpack_pack<sbuffer, small_vector<v_type, U>>::value));
            BOOST_CHECK((has_msgpack_pack<sbuffer, v_type &>::value));
            BOOST_CHECK((has_msgpack_pack<sbuffer, const v_type &>::value));
            BOOST_CHECK((!has_msgpack_pack<sbuffer const, const v_type &>::value));
            BOOST_CHECK((!has_msgpack_pack<void, v_type>::value));
            BOOST_CHECK((has_msgpack_convert<v_type>::value));
            BOOST_CHECK((has_msgpack_convert<small_vector<v_type, U>>::value));
            BOOST_CHECK((has_msgpack_convert<v_type &>::value));
            BOOST_CHECK((!has_msgpack_convert<const v_type &>::value));
            BOOST_CHECK((!has_msgpack_convert<const v_type &>::value));
            using size_type = typename v_type::size_type;
            std::uniform_int_distribution<unsigned> sdist(0u, 20u);
            std::uniform_int_distribution<int> edist(-10, 10);
            for (int i = 0; i < ntries; ++i) {
                for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
                    auto size = static_cast<size_type>(sdist(rng));
                    v_type v;
                    for (decltype(size) j = 0; j < size; ++j) {
                        v.push_back(T(edist(rng)));
                    }
                    msgpack_round_trip(v, f);
                }
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(small_vector_msgpack_s11n_test)
{
    tuple_for_each(value_types{}, msgpack_s11n_tester{});
    BOOST_CHECK((!has_msgpack_pack<sbuffer, small_vector<no_s11n>>::value));
    BOOST_CHECK((!has_msgpack_convert<small_vector<no_s11n>>::value));
    BOOST_CHECK((!has_msgpack_pack<sbuffer, small_vector<small_vector<no_s11n>>>::value));
    BOOST_CHECK((!has_msgpack_convert<small_vector<small_vector<no_s11n>>>::value));
}

#endif
