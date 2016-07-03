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

#include "fateman2.hpp"

#define BOOST_TEST_MODULE fateman2_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>

#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/settings.hpp"

using namespace piranha;

// Fateman's polynomial multiplication test number 2. Calculate:
// f * (f+1)
// where f = (1+x+y+z+t)**30

BOOST_AUTO_TEST_CASE(fateman2_test) {
  init();
  if (boost::unit_test::framework::master_test_suite().argc > 1) {
    settings::set_n_threads(boost::lexical_cast<unsigned>(
        boost::unit_test::framework::master_test_suite().argv[1u]));
  }
  BOOST_CHECK_EQUAL((fateman2<integer, kronecker_monomial<>>().size()),
                    635376u);
}
