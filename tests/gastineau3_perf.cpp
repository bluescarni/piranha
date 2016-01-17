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

#include "gastineau3.hpp"

#define BOOST_TEST_MODULE gastineau3_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/settings.hpp"

using namespace piranha;

// Gastineau's polynomial multiplication test number 2. Calculate:
// f * g
// where
// f = (1 + u**2 + v + w**2 + x - y**2)**28
// g = (1 + u + v**2 + w + x**2 + y**3)**28 + 1

BOOST_AUTO_TEST_CASE(gastineau3_test)
{
	environment env;
	if (boost::unit_test::framework::master_test_suite().argc > 1) {
		settings::set_n_threads(boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
	}
	BOOST_CHECK_EQUAL((gastineau3<integer,kronecker_monomial<>>().size()),144049555ull);
}
