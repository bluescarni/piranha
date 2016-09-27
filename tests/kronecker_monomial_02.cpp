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

#include "../src/kronecker_monomial.hpp"

#define BOOST_TEST_MODULE kronecker_monomial_02_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
// #include <array>
// #include <boost/lexical_cast.hpp>
// #include <boost/mpl/for_each.hpp>
// #include <boost/mpl/vector.hpp>
// #include <cstddef>
// #include <initializer_list>
// #include <iostream>
// #include <limits>
// #include <list>
// #include <set>
#include <mutex>
#include <sstream>
// #include <stdexcept>
#include <random>
#include <string>
#include <thread>
#include <tuple>
// #include <type_traits>
// #include <unordered_map>
// #include <unordered_set>
#include <vector>

// #include "../src/exceptions.hpp"
#include "../src/init.hpp"
// #include "../src/is_key.hpp"
// #include "../src/key_is_convertible.hpp"
// #include "../src/key_is_multipliable.hpp"
// #include "../src/kronecker_array.hpp"
// #include "../src/math.hpp"
// #include "../src/monomial.hpp"
// #include "../src/mp_integer.hpp"
// #include "../src/mp_rational.hpp"
// #include "../src/pow.hpp"
// #include "../src/real.hpp"
// #include "../src/serialization.hpp"
#include "../src/s11n.hpp"
// #include "../src/symbol.hpp"
// #include "../src/symbol_set.hpp"
// #include "../src/term.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

using int_types = std::tuple<signed char, int, long, long long>;

static const int ntries = 1000;

static std::mutex mut;

template <typename OArchive, typename IArchive, typename T>
static inline void boost_roundtrip(const T &x, const symbol_set &args, bool mt = false)
{
    std::stringstream ss;
    {
        OArchive oa(ss);
        x.boost_save(oa, args);
    }
    T retval;
    {
        IArchive ia(ss);
        retval.boost_load(ia, args);
    }
    if (mt) {
        std::lock_guard<std::mutex> lock(mut);
        BOOST_CHECK(x == retval);
    } else {
        BOOST_CHECK(x == retval);
    }
}

struct boost_s11n_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        const std::vector<std::string> names = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "l"};
        auto t_func = [&names](unsigned n) {
            std::uniform_int_distribution<unsigned> sdist(0, 10);
            std::uniform_int_distribution<int> edist(-10, 10);
            std::mt19937 rng(n);
            std::vector<T> expos;
            for (int i = 0; i < ntries; ++i) {
                auto s = sdist(rng);
                expos.resize(s);
                std::generate(expos.begin(), expos.end(), [&rng, &edist]() { return edist(rng); });
                k_type k;
                try {
                    k = k_type(expos);
                } catch (...) {
                    continue;
                }
                boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(
                    k, symbol_set(names.begin(), names.begin() + s), true);
                boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(
                    k, symbol_set(names.begin(), names.begin() + s), true);
            }
        };
        std::thread t0(t_func, 0);
        std::thread t1(t_func, 1);
        std::thread t2(t_func, 2);
        std::thread t3(t_func, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_boost_s11n_test)
{
    init();
    tuple_for_each(int_types{}, boost_s11n_tester());
}
