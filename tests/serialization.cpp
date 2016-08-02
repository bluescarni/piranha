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

#include "../src/serialization.hpp"

#define BOOST_TEST_MODULE serialization_test
#include <boost/test/unit_test.hpp>

#include <sstream>

#include "../src/config.hpp"
#include "../src/init.hpp"

using namespace piranha;

// using msgpack::packer;
// using msgpack::sbuffer;

BOOST_AUTO_TEST_CASE(serialization_boost_test_00)
{
    // Placeholder for now.
    init();
}

#if defined(PIRANHA_ENABLE_MSGPACK)

template <typename T>
using sw = detail::msgpack_stream_wrapper<T>;

BOOST_AUTO_TEST_CASE(serialization_msgpack_tt_test)
{
    BOOST_CHECK(is_msgpack_stream<std::ostringstream>::value);
    BOOST_CHECK(!is_msgpack_stream<std::ostringstream &>::value);
    BOOST_CHECK(!is_msgpack_stream<const std::ostringstream &>::value);
    BOOST_CHECK(!is_msgpack_stream<const std::ostringstream>::value);
    BOOST_CHECK(is_msgpack_stream<msgpack::sbuffer>::value);
    BOOST_CHECK(!is_msgpack_stream<float>::value);
    BOOST_CHECK(!is_msgpack_stream<const double>::value);
    BOOST_CHECK(is_msgpack_stream<sw<std::ostringstream>>::value);
    BOOST_CHECK(!is_msgpack_stream<sw<std::ostringstream> &>::value);
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, int>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, int>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream &, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream &&, int>::value));
    BOOST_CHECK((!has_msgpack_pack<std::ostringstream &&, int>::value));
}

BOOST_AUTO_TEST_CASE(serialization_test_00)
{
    // sbuffer sbuf2;
    // packer<sbuffer &> p2(sbuf2);
    // detail::msgpack_stream_wrapper<std::ofstream> sbuf("out.msgpack");
    // packer<detail::msgpack_stream_wrapper<std::ofstream>> p(sbuf);
    /*    msgpack_pack(p,1.23L,msgpack_format::binary);
        long double x;
        std::size_t offset = 0u;
        auto oh = msgpack::unpack(sbuf.data(),sbuf.size(),offset);
        msgpack_unpack(x,oh.get(),msgpack_format::binary);
        std::cout << x << '\n';*/
    /*    std::vector<long double> a = {1,2,3};
        msgpack_pack(p,a,msgpack_format::portable);
        std::cout << "sbuf size: " << sbuf.size() << '\n';
        std::vector<long double> b;
        std::size_t offset = 0u;
        auto oh = msgpack::unpack(sbuf.data(),sbuf.size(),offset);
        msgpack_unpack(b,oh.get(),msgpack_format::portable);
        std::cout << b.size() << '\n';
        std::cout << b[0] << ',' << b[1] << ',' << b[2] << '\n';
    */
    // using p_type = polynomial<double, monomial<int>>;
    // p_type x{"x"}, y{"y"};
    // auto ret = math::pow(1.1 + .1 * x + .3 * y, 10);
    //{
    /*std::ostringstream oss;
    packer<std::ostringstream> p(oss);*/
    // msgpack_pack(p, ret, msgpack_format::binary);
    // std::cout << sbuf.size() << '\n';
    //}
    // std::stringstream oss;
    //{
    // boost::archive::text_oarchive oa(oss);
    // oa << ret;
    //}
    // std::cout << oss.str() << '\n';
    // std::vector<char> vec;
    // std::copy(std::istreambuf_iterator<char>(oss), std::istreambuf_iterator<char>(), std::back_inserter(vec));
    // std::cout << vec.size() << '\n';
}

#endif
