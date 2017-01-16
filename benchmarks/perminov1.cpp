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

#define BOOST_TEST_MODULE perminov1_test
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <fstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/init.hpp>
#include <piranha/kronecker_array.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/s11n.hpp>
#include <piranha/series.hpp>
#include <piranha/settings.hpp>

#include "simple_timer.hpp"

using namespace piranha;
namespace bfs = boost::filesystem;

// Perminov's polynomial multiplication test number 1. Calculate the truncated multiplication:
// f * g
// Where f = sin(2_l1).epst.bz2 and g = sin(l1-l3).epst.bz2 (from the test data dir).

// The root path for the tests directory.
static const bfs::path root_path(PIRANHA_TESTS_DIRECTORY);

BOOST_AUTO_TEST_CASE(perminov1_test)
{
    init();
    settings::set_thread_binding(true);
    if (boost::unit_test::framework::master_test_suite().argc > 1) {
        settings::set_n_threads(
            boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
    }

    using pt = polynomial<rational, monomial<rational>>;
    using epst = poisson_series<divisor_series<pt, divisor<short>>>;

    epst f, g;

    load_file(f, (root_path / "data" / "s2l1.mpackp.bz2").string());
    load_file(g, (root_path / "data" / "sl1l3.mpackp.bz2").string());

    pt::set_auto_truncate_degree(2, {"x1", "x2", "x3", "y1", "y2", "y3", "u1", "u2", "u3", "v1", "v2", "v3"});

    epst res;
    {
        simple_timer t;
        res = f * g;
    }

    BOOST_CHECK_EQUAL(res.size(), 2u);
    auto it = res._container().begin();
    BOOST_CHECK_EQUAL(it->m_cf._container().begin()->m_cf.size(), 177152u);
    ++it;
    BOOST_CHECK_EQUAL(it->m_cf._container().begin()->m_cf.size(), 177152u);
}
