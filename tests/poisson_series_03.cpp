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

#include <piranha/poisson_series.hpp>

#define BOOST_TEST_MODULE poisson_series_03_test
#include <boost/test/included/unit_test.hpp>

#include <sstream>

#include <piranha/config.hpp>
#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/invert.hpp>
#include <piranha/math.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#include <piranha/s11n.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(poisson_series_boost_s11n_test)
{
    using pst1 = poisson_series<polynomial<rational, monomial<short>>>;
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, pst1>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, pst1>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, pst1 &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const pst1 &>::value));
    BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const pst1 &>::value));
    BOOST_CHECK((!has_boost_save<void, const pst1 &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, pst1>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, pst1>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, pst1>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, pst1 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const pst1 &>::value));
    BOOST_CHECK((!has_boost_load<const boost::archive::binary_oarchive &, const pst1 &>::value));
    BOOST_CHECK((!has_boost_load<void, const pst1 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, pst1>::value));
    {
        pst1 x{"x"}, y{"y"}, z{"z"};
        const auto tmp = (x + y) * 3 + z * math::cos(x - y) + 1;
        std::stringstream ss;
        {
            boost::archive::binary_oarchive oa(ss);
            boost_save(oa, tmp);
        }
        {
            pst1 retval;
            boost::archive::binary_iarchive ia(ss);
            boost_load(ia, retval);
            BOOST_CHECK_EQUAL(tmp, retval);
        }
    }
    using pst2 = poisson_series<divisor_series<polynomial<rational, monomial<short>>, divisor<int>>>;
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, pst2>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, pst2>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, pst2 &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const pst2 &>::value));
    BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const pst2 &>::value));
    BOOST_CHECK((!has_boost_save<void, const pst2 &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, pst2>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, pst2>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, pst2>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, pst2 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const pst2 &>::value));
    BOOST_CHECK((!has_boost_load<const boost::archive::binary_oarchive &, const pst2 &>::value));
    BOOST_CHECK((!has_boost_load<void, const pst2 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, pst2>::value));
    {
        pst2 x{"x"}, y{"y"}, z{"z"};
        const auto tmp = (x + y) * 3 * math::invert(z) + z * math::cos(x - y) + 1;
        std::stringstream ss;
        {
            boost::archive::binary_oarchive oa(ss);
            boost_save(oa, tmp);
        }
        {
            pst2 retval;
            boost::archive::binary_iarchive ia(ss);
            boost_load(ia, retval);
            BOOST_CHECK_EQUAL(tmp, retval);
        }
    }
}

#if defined(PIRANHA_WITH_MSGPACK)

BOOST_AUTO_TEST_CASE(poisson_series_msgpack_s11n_test)
{
    using pst1 = poisson_series<polynomial<rational, monomial<short>>>;
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, pst1>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, pst1>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, pst1 &>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const pst1 &>::value));
    BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer &, const pst1 &>::value));
    BOOST_CHECK((!has_msgpack_pack<void, const pst1 &>::value));
    BOOST_CHECK((has_msgpack_convert<pst1>::value));
    BOOST_CHECK((has_msgpack_convert<pst1 &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pst1 &>::value));
    {
        pst1 x{"x"}, y{"y"}, z{"z"};
        const auto tmp = (x + y) * 3 + z * math::cos(x - y) + 1;
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        msgpack_pack(p, tmp, msgpack_format::binary);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        pst1 retval;
        msgpack_convert(retval, oh.get(), msgpack_format::binary);
        BOOST_CHECK_EQUAL(tmp, retval);
        std::stringstream ss;
    }
    using pst2 = poisson_series<divisor_series<polynomial<rational, monomial<short>>, divisor<int>>>;
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, pst2>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, pst2>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, pst2 &>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const pst2 &>::value));
    BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer &, const pst2 &>::value));
    BOOST_CHECK((!has_msgpack_pack<void, const pst2 &>::value));
    BOOST_CHECK((has_msgpack_convert<pst2>::value));
    BOOST_CHECK((has_msgpack_convert<pst2 &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pst2 &>::value));
    {
        pst2 x{"x"}, y{"y"}, z{"z"};
        const auto tmp = (x + y) * 3 * math::invert(z) + z * math::cos(x - y) + 1;
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        msgpack_pack(p, tmp, msgpack_format::binary);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        pst2 retval;
        msgpack_convert(retval, oh.get(), msgpack_format::binary);
        BOOST_CHECK_EQUAL(tmp, retval);
        std::stringstream ss;
    }
}

#endif
