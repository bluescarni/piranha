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

#include <piranha/rational_function.hpp>

#define BOOST_TEST_MODULE rational_function_03_test
#include <boost/test/included/unit_test.hpp>

#include <exception>
#include <initializer_list>
#include <sstream>
#include <tuple>

#include <piranha/init.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/s11n.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using key_types = std::tuple<k_monomial, monomial<unsigned char>, monomial<integer>>;

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
    BOOST_CHECK_EQUAL(x, retval);
}

struct boost_s11n_tester {
    template <typename Key>
    void operator()(const Key &) const
    {
        using r_type = rational_function<Key>;
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, r_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, r_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, const r_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const r_type &>::value));
        BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const r_type &>::value));
        BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive, const r_type &>::value));
        BOOST_CHECK((!has_boost_save<void, const r_type &>::value));
        BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, r_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, r_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, r_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, r_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const r_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const r_type &>::value));
        BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, const r_type &>::value));
        BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive, const r_type &>::value));
        BOOST_CHECK((!has_boost_load<void, const r_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, r_type>::value));
        r_type x{"x"}, y{"y"};
        boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(x / y);
        boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(x / y);
        boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>((x + y) / (x * x - y * y));
        boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>((x + y) / (x * x - y * y));
    }
};

BOOST_AUTO_TEST_CASE(rational_function_boost_s11n_test)
{
    init();
    tuple_for_each(key_types{}, boost_s11n_tester{});
}

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline void msgpack_roundtrip(const T &r, msgpack_format f)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, r, f);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    BOOST_CHECK_EQUAL(retval, r);
}

struct msgpack_s11n_tester {
    template <typename Key>
    void operator()(const Key &) const
    {
        using r_type = rational_function<Key>;
        using p_type = typename r_type::p_type;
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, r_type>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, r_type &>::value));
        BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const r_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, const r_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer &, const r_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer, const r_type &>::value));
        BOOST_CHECK((!has_msgpack_pack<void, const r_type &>::value));
        BOOST_CHECK((has_msgpack_convert<r_type>::value));
        BOOST_CHECK((has_msgpack_convert<r_type &>::value));
        BOOST_CHECK((!has_msgpack_convert<r_type const &>::value));
        BOOST_CHECK((!has_msgpack_convert<r_type const>::value));
        r_type x{"x"}, y{"y"};
        for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
            msgpack_roundtrip(x / y, f);
            msgpack_roundtrip((x + y) / (x * x - y * y), f);
        }
        // Check that portable format canonicalise.
        msgpack::sbuffer sbuf;
        {
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, p_type{"x"}, msgpack_format::portable);
            msgpack_pack(p, p_type{"x"}, msgpack_format::portable);
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            r_type retval;
            msgpack_convert(retval, oh.get(), msgpack_format::portable);
            BOOST_CHECK_EQUAL(retval, 1);
        }
        // Check that binary format does not canonicalise.
        sbuf.clear();
        {
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, p_type{"x"}, msgpack_format::binary);
            msgpack_pack(p, p_type{"x"}, msgpack_format::binary);
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            r_type retval;
            msgpack_convert(retval, oh.get(), msgpack_format::binary);
            BOOST_CHECK_EQUAL(retval.num(), p_type{"x"});
            BOOST_CHECK_EQUAL(retval.den(), p_type{"x"});
            retval.canonicalise();
        }
        // Exception safety.
        sbuf.clear();
        {
            msgpack::packer<msgpack::sbuffer> p(sbuf);
            p.pack_array(2);
            msgpack_pack(p, p_type{"x"}, msgpack_format::binary);
            msgpack_pack(p, 123, msgpack_format::binary);
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
            r_type retval{123};
            BOOST_CHECK_THROW(msgpack_convert(retval, oh.get(), msgpack_format::binary), std::exception);
            BOOST_CHECK_EQUAL(retval, 123);
        }
    }
};

BOOST_AUTO_TEST_CASE(rational_function_msgpack_s11n_test)
{
    tuple_for_each(key_types{}, msgpack_s11n_tester{});
}

#endif
