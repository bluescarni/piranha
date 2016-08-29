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

#include "../src/mp_rational.hpp"

#define BOOST_TEST_MODULE mp_rational_02_test
#include <boost/test/unit_test.hpp>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <random>
#include <sstream>
#include <type_traits>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/s11n.hpp"

using namespace piranha;

using boost::archive::binary_oarchive;
using boost::archive::binary_iarchive;
using boost::archive::text_oarchive;
using boost::archive::text_iarchive;
using boost::archive::xml_oarchive;
using boost::archive::xml_iarchive;

using size_types = boost::mpl::vector<std::integral_constant<int, 0>, std::integral_constant<int, 8>,
                                      std::integral_constant<int, 16>, std::integral_constant<int, 32>
#if defined(PIRANHA_UINT128_T)
                                      ,
                                      std::integral_constant<int, 64>
#endif
                                      >;

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
    BOOST_CHECK_EQUAL(retval, x);
}

static const int ntrials = 1000;

static std::mt19937 rng;

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &)
    {
        using q_type = mp_rational<T::value>;
        BOOST_CHECK((has_boost_save<binary_oarchive, q_type>::value));
        BOOST_CHECK((has_boost_save<binary_oarchive &, q_type>::value));
        BOOST_CHECK((has_boost_save<binary_oarchive &, q_type &>::value));
        BOOST_CHECK((has_boost_save<binary_oarchive &, const q_type &>::value));
        BOOST_CHECK((!has_boost_save<const binary_oarchive &, const q_type &>::value));
        BOOST_CHECK((!has_boost_save<xml_oarchive, q_type>::value));
        BOOST_CHECK((!has_boost_save<binary_iarchive, q_type>::value));
        BOOST_CHECK((has_boost_load<binary_iarchive, q_type>::value));
        BOOST_CHECK((has_boost_load<binary_iarchive &, q_type>::value));
        BOOST_CHECK((has_boost_load<binary_iarchive &, q_type &>::value));
        BOOST_CHECK((!has_boost_load<binary_iarchive &, const q_type &>::value));
        BOOST_CHECK((!has_boost_load<const binary_iarchive &, const q_type &>::value));
        BOOST_CHECK((!has_boost_load<xml_iarchive, q_type>::value));
        BOOST_CHECK((!has_boost_load<binary_oarchive, q_type>::value));
        // A few simple checks.
        boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{});
        boost_roundtrip<text_oarchive, text_iarchive>(q_type{});
        boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{-1});
        boost_roundtrip<text_oarchive, text_iarchive>(q_type{23});
        boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{-1, 5});
        boost_roundtrip<text_oarchive, text_iarchive>(q_type{23, 67});
        // Random testing.
        std::uniform_int_distribution<int> dist(-1000, 1000);
        for (auto i = 0; i < ntrials; ++i) {
            auto num = dist(rng);
            auto den = dist(rng);
            if (den == 0) {
                continue;
            }
            boost_roundtrip<binary_oarchive, binary_iarchive>(q_type{num, den});
            boost_roundtrip<text_oarchive, text_iarchive>(q_type{num, den});
        }
    }
};

BOOST_AUTO_TEST_CASE(mp_rational_boost_s11n_test)
{
    init();
    boost::mpl::for_each<size_types>(boost_s11n_tester());
}
