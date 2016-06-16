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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE symengine_expand2b_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>

#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/settings.hpp"

using namespace piranha;
using math::pow;

// Test taken from:
// https://github.com/sympy/symengine/blob/master/benchmarks/expand2b.cpp

BOOST_AUTO_TEST_CASE(symengine_expand2b_test)
{
	init();
	if (boost::unit_test::framework::master_test_suite().argc > 1) {
		settings::set_n_threads(boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
	}
	using p_type = polynomial<integer,k_monomial>;
	auto x = p_type{"x"}, y = p_type{"y"}, z = p_type{"z"}, w = p_type{"w"};
	auto f = pow(x+y+z+w,15);
	auto g = f + w;
	{
	boost::timer::auto_cpu_timer t;
	BOOST_CHECK_EQUAL((f * g).size(),6272u);
	}
}
