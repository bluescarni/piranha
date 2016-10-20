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

#include "../src/hash_set.hpp"

#define BOOST_TEST_MODULE hash_set_02_test
#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <random>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/s11n.hpp"
#include "../src/type_traits.hpp"

static const int ntries = 1000;

using namespace piranha;

static std::mt19937 rng;

using types = std::tuple<int, integer, rational>;

struct no_s11n {
    bool operator==(const no_s11n &) const;
};

namespace std
{

template <>
struct hash<no_s11n> {
    size_t operator()(const no_s11n &) const;
};
}

template <typename H>
static inline bool check_eq(const H &h1, const H &h2)
{
    if (h1.size() != h2.size()) {
        return false;
    }
    for (const auto &x : h1) {
        auto it = h2.find(x);
        if (it == h2.end()) {
            return false;
        }
    }
    return true;
}

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
    BOOST_CHECK(check_eq(x, retval));
}

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using h_type = hash_set<T>;
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, h_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, h_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const h_type &>::value));
        BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const h_type &>::value));
        BOOST_CHECK((!has_boost_save<void, const h_type &>::value));
        BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, h_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, h_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, h_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const h_type &>::value));
        BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, const h_type &>::value));
        BOOST_CHECK((!has_boost_load<void, const h_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, h_type>::value));
        using size_type = typename h_type::size_type;
        std::uniform_int_distribution<size_type> sdist(0, 10);
        std::uniform_int_distribution<int> vdist(-10, 10);
        for (int i = 0; i < ntries; ++i) {
            const auto size = sdist(rng);
            h_type h;
            for (size_type j = 0; j < size; ++j) {
                h.insert(T(vdist(rng)));
            }
            boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(h);
            boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(h);
        }
    }
};

BOOST_AUTO_TEST_CASE(hash_set_boost_s11n_test)
{
    init();
    tuple_for_each(types{}, boost_s11n_tester{});
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive, hash_set<no_s11n>>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, hash_set<no_s11n>>::value));
}

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline void msgpack_roundtrip(const T &x, msgpack_format f)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, x, f);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    BOOST_CHECK(check_eq(retval, x));
}

struct msgpack_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using h_type = hash_set<T>;
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, h_type>::value));
        BOOST_CHECK((has_msgpack_pack<std::stringstream, h_type &>::value));
        BOOST_CHECK((has_msgpack_pack<std::stringstream, const h_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<std::stringstream &, const h_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<void, const h_type &>::value));
        BOOST_CHECK((has_msgpack_convert<h_type>::value));
        BOOST_CHECK((has_msgpack_convert<h_type &>::value));
        BOOST_CHECK((!has_msgpack_convert<const h_type &>::value));
        using size_type = typename h_type::size_type;
        std::uniform_int_distribution<size_type> sdist(0, 10);
        std::uniform_int_distribution<int> vdist(-10, 10);
        for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
            for (int i = 0; i < ntries; ++i) {
                const auto size = sdist(rng);
                h_type h;
                for (size_type j = 0; j < size; ++j) {
                    h.insert(T(vdist(rng)));
                }
                msgpack_roundtrip(h, f);
            }
        }
        // Check failure modes.
        {
            // Duplicate elements.
            msgpack::sbuffer sbuf;
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, T(42), msgpack_format::binary);
            msgpack_pack(p, T(42), msgpack_format::binary);
            h_type h;
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            BOOST_CHECK_EXCEPTION(
                msgpack_convert(h, oh.get(), msgpack_format::binary), std::invalid_argument,
                [](const std::invalid_argument &iae) {
                    return boost::contains(
                        iae.what(),
                        "while deserializing a hash_set from a msgpack object a duplicate value was encountered");
                });
        }
    }
};

BOOST_AUTO_TEST_CASE(hash_set_msgpack_s11n_test)
{
    tuple_for_each(types{}, msgpack_s11n_tester{});
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer, hash_set<no_s11n>>::value));
    BOOST_CHECK((!has_msgpack_convert<hash_set<no_s11n>>::value));
}

#endif
