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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE polynomial_05_test
#include <boost/test/included/unit_test.hpp>

#include <sstream>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/pow.hpp"
#include "../src/s11n.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(polynomial_truncation_pow_cache_test)
{
    init();
    using p_type = polynomial<integer, monomial<int>>;
    p_type x{"x"}, y{"y"};
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y + x * x + y * y + 2 * x * y);
    // Always test twice to exercise the other branch of the pow cache clearing function.
    p_type::set_auto_truncate_degree(1);
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y);
    p_type::set_auto_truncate_degree(1);
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y);
    p_type::set_auto_truncate_degree(1, {"x"});
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y + y * y + 2 * x * y);
    p_type::set_auto_truncate_degree(1, {"x"});
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y + y * y + 2 * x * y);
    p_type::set_auto_truncate_degree(1, {"y"});
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y + x * x + 2 * x * y);
    p_type::set_auto_truncate_degree(1, {"y"});
    BOOST_CHECK_EQUAL(math::pow(x + y + 1, 2), 2 * x + 1 + 2 * y + x * x + 2 * x * y);
}

BOOST_AUTO_TEST_CASE(polynomial_boost_s11n_test)
{
    using p_type = polynomial<integer, monomial<int>>;
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
    using pp_type = polynomial<p_type, monomial<int>>;
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, pp_type>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, pp_type>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, pp_type &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const pp_type &>::value));
    BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const pp_type &>::value));
    BOOST_CHECK((!has_boost_save<void, const pp_type &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, pp_type>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, pp_type>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, pp_type>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, pp_type &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const pp_type &>::value));
    BOOST_CHECK((!has_boost_load<const boost::archive::binary_oarchive &, const pp_type &>::value));
    BOOST_CHECK((!has_boost_load<void, const pp_type &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, pp_type>::value));
    p_type x{"x"}, y{"y"};
    const auto tmp = (x + y) * 3 * (x - y) + 1;
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
    ss.str("");
    ss.clear();
    pp_type xx{"x"}, yy{"y"};
    const auto ttmp = (xx + yy) * 3 * (xx - yy) + 1;
    {
        boost::archive::binary_oarchive oa(ss);
        boost_save(oa, ttmp);
    }
    {
        pp_type retval;
        boost::archive::binary_iarchive ia(ss);
        boost_load(ia, retval);
        BOOST_CHECK_EQUAL(ttmp, retval);
    }
}

#if defined(PIRANHA_WITH_MSGPACK)

BOOST_AUTO_TEST_CASE(polynomial_msgpack_s11n_test)
{
    using p_type = polynomial<integer, monomial<int>>;
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, p_type>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, p_type>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, p_type &>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const p_type &>::value));
    BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer &, const p_type &>::value));
    BOOST_CHECK((!has_msgpack_pack<void, const p_type &>::value));
    BOOST_CHECK((has_msgpack_convert<p_type>::value));
    BOOST_CHECK((has_msgpack_convert<p_type &>::value));
    BOOST_CHECK((!has_msgpack_convert<const p_type &>::value));
    using pp_type = polynomial<p_type, monomial<int>>;
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, pp_type>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, pp_type>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, pp_type &>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const pp_type &>::value));
    BOOST_CHECK((!has_msgpack_pack<const msgpack::sbuffer &, const pp_type &>::value));
    BOOST_CHECK((!has_msgpack_pack<void, const pp_type &>::value));
    BOOST_CHECK((has_msgpack_convert<pp_type>::value));
    BOOST_CHECK((has_msgpack_convert<pp_type &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pp_type &>::value));
    {
        p_type x{"x"}, y{"y"};
        const auto tmp = (x + y) * 3 * (x - y) + 1;
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        msgpack_pack(p, tmp, msgpack_format::portable);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        p_type retval;
        msgpack_convert(retval, oh.get(), msgpack_format::portable);
        BOOST_CHECK(retval == tmp);
    }
    {
        pp_type xx{"x"}, yy{"y"};
        const auto ttmp = (xx + yy) * 3 * (xx - yy) + 1;
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        msgpack_pack(p, ttmp, msgpack_format::portable);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        pp_type retval;
        msgpack_convert(retval, oh.get(), msgpack_format::portable);
        BOOST_CHECK(retval == ttmp);
    }
}

#endif
