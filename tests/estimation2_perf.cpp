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

#include "../src/base_series_multiplier.hpp"

#define BOOST_TEST_MODULE estimation2_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iostream>
#include <random>
#include <string>

#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/polynomial.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;
using p_type = polynomial<double, k_monomial>;
using t_type = p_type::term_type;

static std::mt19937 rng;

static inline p_type random_poly(unsigned nvars, unsigned nterms, unsigned max_expo)
{
    p_type retval;
    symbol_set ss;
    for (auto i = 0u; i < nvars; ++i) {
        ss.add("x" + std::to_string(i));
    }
    retval.set_symbol_set(ss);
    std::uniform_int_distribution<int> cf_dist(-99,99);
    std::uniform_int_distribution<unsigned> expo_dist(0u,max_expo);
    std::vector<unsigned> expo_vector(safe_cast<std::vector<unsigned>::size_type>(nvars));
    for (auto i = 0u; i < nterms; ++i) {
        std::generate(expo_vector.begin(),expo_vector.end(),[&expo_dist](){return expo_dist(rng);});
        retval.insert(t_type{cf_dist(rng),t_type::key_type(expo_vector.begin(),expo_vector.end())});
    }
    return retval;
}

static inline void random_run(unsigned nvars, unsigned nterms, unsigned max_expo)
{
    const auto p = random_poly(nvars,nterms,max_expo);
    auto p2 = p * p;
    std::cout << "Sparsity: " << ((double)(p.size()) * p.size()) / p2.size() << '\n';
    std::cout << "Load factor: " << p2.table_load_factor() << '\n';
}

BOOST_AUTO_TEST_CASE(estimation_random_test)
{
    init();
    random_run(5,1500,30);
}
