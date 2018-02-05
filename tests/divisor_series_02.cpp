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

#include <piranha/divisor_series.hpp>

#define BOOST_TEST_MODULE divisor_series_02_test
#include <boost/test/included/unit_test.hpp>

#include <piranha/config.hpp>
#include <piranha/divisor.hpp>
#include <piranha/invert.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#include <piranha/s11n.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(divisor_series_empty_test) {}

#if defined(PIRANHA_WITH_BOOST_S11N)

BOOST_AUTO_TEST_CASE(divisor_series_boost_s11n_test)
{
    using p_type = divisor_series<polynomial<rational, monomial<char>>, divisor<short>>;
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, p_type>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, p_type>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, p_type &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const p_type &>::value));
    BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const p_type &>::value));
    BOOST_CHECK((!has_boost_save<void, const p_type &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, p_type>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, p_type>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, p_type>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, p_type &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const p_type &>::value));
    BOOST_CHECK((!has_boost_load<const boost::archive::binary_oarchive &, const p_type &>::value));
    BOOST_CHECK((!has_boost_load<void, const p_type &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, p_type>::value));
    p_type x{"x"}, y{"y"};
    const auto tmp = (x + y) * 3 * math::invert(y) + (x - y) + 1;
    std::stringstream ss;
    {
        boost::archive::binary_oarchive oa(ss);
        boost_save(oa, tmp);
    }
    {
        p_type retval;
        boost::archive::binary_iarchive ia(ss);
        boost_load(ia, retval);
        BOOST_CHECK_EQUAL(tmp, retval);
    }
}

#endif

#if defined(PIRANHA_WITH_MSGPACK)

BOOST_AUTO_TEST_CASE(divisor_series_msgpack_s11n_test)
{
    using p_type = divisor_series<polynomial<rational, monomial<char>>, divisor<short>>;
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, p_type>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, p_type>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, p_type &>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const p_type &>::value));
    BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer &, const p_type &>::value));
    BOOST_CHECK((!has_msgpack_pack<void, const p_type &>::value));
    BOOST_CHECK((has_msgpack_convert<p_type>::value));
    BOOST_CHECK((has_msgpack_convert<p_type &>::value));
    BOOST_CHECK((!has_msgpack_convert<const p_type &>::value));
    p_type x{"x"}, y{"y"};
    const auto tmp = (x + y) * 3 * math::invert(y) + (x - y) + 1;
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, tmp, msgpack_format::portable);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    p_type retval;
    msgpack_convert(retval, oh.get(), msgpack_format::portable);
    BOOST_CHECK(retval == tmp);
}

#endif
