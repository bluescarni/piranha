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
#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/timer/timer.hpp>
#include <cstddef>
#include <fstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../src/divisor.hpp"
#include "../src/divisor_series.hpp"
#include "../src/init.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/serialization.hpp"
#include "../src/series.hpp"
#include "../src/settings.hpp"

using namespace piranha;
namespace bfs = boost::filesystem;

// Perminov's polynomial multiplication test number 1. Calculate the truncated
// multiplication:
// f * g
// Where f = sin(2_l1).epst.bz2 and g = sin(l1-l3).epst.bz2 (from the test data
// dir).

// The root path for the tests directory.
static const bfs::path root_path(PIRANHA_TESTS_DIRECTORY);

static inline bool check_limits() {
  using int_type = std::make_signed<std::size_t>::type;
  // First load the archived limits.txt file.
  std::ifstream in((root_path / "data" / "limits.txt").string());
  boost::archive::text_iarchive ia(in);
  std::vector<std::vector<int_type>> lims;
  ia >> lims;
  // Now get the current limits.
  std::vector<std::vector<int_type>> comp;
  for (const auto &t : kronecker_array<int_type>::get_limits()) {
    comp.push_back(std::get<0u>(t));
  }
  // Compare them.
  return comp == lims;
}

BOOST_AUTO_TEST_CASE(perminov1_test) {
  init();
  if (boost::unit_test::framework::master_test_suite().argc > 1) {
    settings::set_n_threads(boost::lexical_cast<unsigned>(
        boost::unit_test::framework::master_test_suite().argv[1u]));
  }

  if (!check_limits()) {
    std::cout << "This architecture is incompatible with the data files needed "
                 "for this test.\n";
    return;
  }

  using pt = polynomial<rational, monomial<rational>>;
  using epst = poisson_series<divisor_series<pt, divisor<short>>>;

  auto f = epst::load((root_path / "data" / "sin(2_l1).epst.bz2").string(),
                      file_compression::bzip2);
  auto g = epst::load((root_path / "data" / "sin(l1-l3).epst.bz2").string(),
                      file_compression::bzip2);

  pt::set_auto_truncate_degree(2, {"x1", "x2", "x3", "y1", "y2", "y3", "u1",
                                   "u2", "u3", "v1", "v2", "v3"});

  epst res;
  {
    boost::timer::auto_cpu_timer t;
    res = f * g;
  }

  BOOST_CHECK_EQUAL(res.size(), 2u);
  auto it = res._container().begin();
  BOOST_CHECK_EQUAL(it->m_cf._container().begin()->m_cf.size(), 177152u);
  ++it;
  BOOST_CHECK_EQUAL(it->m_cf._container().begin()->m_cf.size(), 177152u);
}
