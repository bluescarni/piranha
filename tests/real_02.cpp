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

#include "../src/real.hpp"

#define BOOST_TEST_MODULE real_02_test
#include <boost/test/unit_test.hpp>

#include <limits>
#include <random>
#include <sstream>
#include <vector>

#include "../src/config.hpp"
#include "../src/detail/mpfr.hpp"
#include "../src/init.hpp"
#include "../src/s11n.hpp"

static std::mt19937 rng;
static const int ntries = 1000;

using namespace piranha;

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
    BOOST_CHECK_EQUAL(x.get_prec(), retval.get_prec());
}

BOOST_AUTO_TEST_CASE(real_boost_s11n_test)
{
    init();
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, real>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, real>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, real>::value));
    BOOST_CHECK((!has_boost_save<void, real>::value));
    BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, real>::value));
    BOOST_CHECK((!has_boost_load<void, real>::value));
    std::vector<::mpfr_prec_t> vprec{32, 64, 128, 256, 512};
    std::uniform_real_distribution<double> dist(0., 1.);
    for (auto prec : vprec) {
        for (auto i = 0; i < ntries; ++i) {
            boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(real(dist(rng), prec));
            boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(real(dist(rng), prec));
        }
        // Some special values.
        boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(real(0., prec));
        boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(real(0., prec));
        if (std::numeric_limits<double>::has_infinity) {
            boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                real(std::numeric_limits<double>::infinity(), prec));
            boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(
                real(std::numeric_limits<double>::infinity(), prec));
            boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                real(-std::numeric_limits<double>::infinity(), prec));
            boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(
                real(-std::numeric_limits<double>::infinity(), prec));
        }
        if (std::numeric_limits<double>::has_quiet_NaN) {
            {
                std::stringstream ss;
                {
                    boost::archive::binary_oarchive oa(ss);
                    boost_save(oa, real(std::numeric_limits<double>::quiet_NaN()));
                }
                real retval;
                {
                    boost::archive::binary_iarchive ia(ss);
                    boost_load(ia, retval);
                }
                BOOST_CHECK(retval.is_nan());
            }
            {
                std::stringstream ss;
                {
                    boost::archive::binary_oarchive oa(ss);
                    boost_save(oa, real(-std::numeric_limits<double>::quiet_NaN()));
                }
                real retval;
                {
                    boost::archive::binary_iarchive ia(ss);
                    boost_load(ia, retval);
                }
                BOOST_CHECK(retval.is_nan());
            }
            {
                std::stringstream ss;
                {
                    boost::archive::text_oarchive oa(ss);
                    boost_save(oa, real(std::numeric_limits<double>::quiet_NaN()));
                }
                real retval;
                {
                    boost::archive::text_iarchive ia(ss);
                    boost_load(ia, retval);
                }
                BOOST_CHECK(retval.is_nan());
            }
            {
                std::stringstream ss;
                {
                    boost::archive::text_oarchive oa(ss);
                    boost_save(oa, real(-std::numeric_limits<double>::quiet_NaN()));
                }
                real retval;
                {
                    boost::archive::text_iarchive ia(ss);
                    boost_load(ia, retval);
                }
                BOOST_CHECK(retval.is_nan());
            }
        }
    }
    // Check for exception safety in binary mode.
    {
        std::stringstream ss;
        {
            boost::archive::binary_oarchive oa(ss);
            boost_save(oa, ::mpfr_prec_t(100));
            boost_save(oa, decltype(std::declval<::mpfr_t &>()->_mpfr_sign)(0));
            boost_save(oa, decltype(std::declval<::mpfr_t &>()->_mpfr_exp)(0));
        }
        real retval{42};
        {
            boost::archive::binary_iarchive ia(ss);
            try {
                boost_load(ia, retval);
                BOOST_CHECK(false);
            } catch (...) {
                BOOST_CHECK_EQUAL(retval, 0);
            }
        }
    }
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
    BOOST_CHECK_EQUAL(x,retval);
    BOOST_CHECK_EQUAL(x.get_prec(), retval.get_prec());
}

BOOST_AUTO_TEST_CASE(real_msgpack_s11n_test)
{

}

#endif
