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

#include "../src/s11n.hpp"

#define BOOST_TEST_MODULE s11n_test
#include <boost/test/unit_test.hpp>

#include <boost/timer/timer.hpp>
#include <iostream>
#include <sstream>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/polynomial.hpp"
#include "pearce1.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(s11n_series_test)
{
    init();
    std::cout << "Multiplication time: ";
    const auto res = pearce1<integer, monomial<signed char>>();
    using pt = decltype(res * res);
    std::stringstream ss;
    {
        boost::archive::binary_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        boost_save(oa, res);
        std::cout << "Boost save, binary, timing: ";
    }
    std::cout << "Boost save, binary, size: " << ss.tellp() << '\n';
    pt tmp;
    {
        boost::archive::binary_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        boost_load(ia, tmp);
        std::cout << "Boost load, binary, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    {
        boost::archive::text_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        boost_save(oa, res);
        std::cout << "Boost save, text, timing: ";
    }
    std::cout << "Boost save, text, size: " << ss.tellp() << '\n';
    tmp = pt{};
    {
        boost::archive::text_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        boost_load(ia, tmp);
        std::cout << "Boost load, text, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    {
        boost::archive::text_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        oa << res;
        std::cout << "Old save, text, timing: ";
    }
    std::cout << "Old save, text, size: " << ss.tellp() << '\n';
    tmp = pt{};
    {
        boost::archive::text_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        ia >> tmp;
        std::cout << "Old load, text, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    {
        boost::archive::binary_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        oa << res;
        std::cout << "Old save, binary, timing: ";
    }
    std::cout << "Old save, binary, size: " << ss.tellp() << '\n';
    tmp = pt{};
    {
        boost::archive::binary_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        ia >> tmp;
        std::cout << "Old load, binary, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
#if defined(PIRANHA_WITH_MSGPACK)
    msgpack::sbuffer sbuf;
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        boost::timer::auto_cpu_timer t;
        msgpack_pack(p, res, msgpack_format::binary);
        std::cout << "msgpack pack, sbuffer, binary, timing: ";
    }
    std::cout << "msgpack pack, binary, size: " << sbuf.size() << '\n';
    {
        boost::timer::auto_cpu_timer t;
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        msgpack_convert(tmp, oh.get(), msgpack_format::binary);
        std::cout << "msgpack convert, sbuffer, binary, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
#endif
}
