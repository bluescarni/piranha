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

#include "../src/mp_integer.hpp"

#define BOOST_TEST_MODULE mp_integer_04_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <atomic>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
// #include <boost/lexical_cast.hpp>
// #include <cstddef>
// #include <functional>
// #include <gmp.h>
#include <limits>
#include <random>
#include <sstream>
// #include <stdexcept>
// #include <string>
#include <thread>
#include <type_traits>
// #include <unordered_map>
// #include <vector>

// #include "../src/binomial.hpp"
#include "../src/config.hpp"
// #include "../src/exceptions.hpp"
#include "../src/init.hpp"
// #include "../src/math.hpp"
#include "../src/s11n.hpp"
// #include "../src/type_traits.hpp"

using namespace piranha;

using size_types = boost::mpl::vector<std::integral_constant<int, 0>, std::integral_constant<int, 8>,
                                      std::integral_constant<int, 16>, std::integral_constant<int, 32>
#if defined(PIRANHA_UINT128_T)
                                      ,
                                      std::integral_constant<int, 64>
#endif
                                      >;

constexpr int ntries = 1000;

template <typename OArchive, typename IArchive, typename T>
static inline T boost_roundtrip(const T &x, bool promote = false)
{
    std::stringstream ss;
    {
        OArchive oa(ss);
        boost_save(oa, x);
    }
    T retval;
    if (promote) {
        retval.promote();
    }
    {
        IArchive ia(ss);
        boost_load(ia, retval);
    }
    return retval;
}

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &)
    {
        using int_type = mp_integer<T::value>;
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const int_type &>::value));
        BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, const int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, int_type>::value));
        BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, int_type &>::value));
        BOOST_CHECK((!has_boost_save<const boost::archive::binary_oarchive &, const int_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, int_type>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, int_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::text_iarchive &, int_type &>::value));
        BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, int_type>::value));
        BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, int_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const int_type &>::value));
        BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive &, int_type &>::value));
        // A few checks with zero.
        BOOST_CHECK_EQUAL(
            (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(int_type{})),
            int_type{});
        int_type tmp;
        tmp.promote();
        BOOST_CHECK_EQUAL((boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp)),
                          int_type{});
        tmp = int_type{};
        BOOST_CHECK_EQUAL(
            (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp, true)), int_type{});
        tmp.promote();
        BOOST_CHECK_EQUAL(
            (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp, true)), int_type{});
        // Random multi-threaded testing.
        std::atomic<bool> status(true);
        auto checker = [&status](unsigned n) {
            std::mt19937 rng(static_cast<std::mt19937::result_type>(n));
            std::uniform_int_distribution<long long> dist(std::numeric_limits<long long>::min(),
                                                          std::numeric_limits<long long>::max());
            std::uniform_int_distribution<int> pdist(0, 1);
            for (int i = 0; i < ntries; ++i) {
                int_type cmp(dist(rng));
                if (pdist(rng) && cmp.is_static()) {
                    cmp.promote();
                }
                // Make it occupy a few mpz limbs, sometimes.
                if (pdist(rng)) {
                    cmp *= cmp;
                    cmp *= cmp;
                }
                // Randomly flip sign (useful if the code above was run, as that forces the value to be positive
                // because it squares the original value twice).
                if (pdist(rng)) {
                    cmp.negate();
                }
                if (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(cmp, pdist(rng))
                    != cmp) {
                    status.store(false);
                }
                if (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(cmp, pdist(rng))
                    != cmp) {
                    status.store(false);
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
        // Test various failure modes.
        std::stringstream ss;
        {
            boost::archive::binary_oarchive oa(ss);
            oa << true;
            oa << detail::mpz_size_t(1);
        }
        int_type n{1};
        try {
            boost::archive::binary_iarchive ia(ss);
            boost_load(ia, n);
        } catch (...) {
            BOOST_CHECK_EQUAL(n, 0);
        }
        ss.str("");
        {
            boost::archive::binary_oarchive oa(ss);
            oa << false;
            oa << detail::mpz_size_t(1);
        }
        n = int_type{1};
        n.promote();
        try {
            boost::archive::binary_iarchive ia(ss);
            boost_load(ia, n);
        } catch (...) {
            BOOST_CHECK_EQUAL(n, 0);
        }
    }
};

BOOST_AUTO_TEST_CASE(mp_integer_boost_s11n_test)
{
    init();
    boost::mpl::for_each<size_types>(boost_s11n_tester());
}

#if defined(PIRANHA_WITH_MSGPACK)

BOOST_AUTO_TEST_CASE(mp_integer_msgpack_s11n_test)
{
    // boost::mpl::for_each<size_types>(msgpack_s11n_tester());
}

#endif
