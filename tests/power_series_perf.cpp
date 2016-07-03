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

#define BOOST_TEST_MODULE power_series_test
#include <boost/test/unit_test.hpp>

#include <boost/timer/timer.hpp>
#include <iostream>

#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "pearce1.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(pearce1_test) {
  init();
  std::cout << "Timing multiplication:\n";
  auto ret1 = pearce1<integer, kronecker_monomial<>>();
  decltype(ret1) ret2;
  {
    std::cout << "Timing degree computation: ";
    boost::timer::auto_cpu_timer t;
    std::cout << ret1.degree() << '\n';
  }
  {
    std::cout << "Timing degree truncation:\n";
    boost::timer::auto_cpu_timer t;
    ret2 = ret1.truncate_degree(30);
  }
  {
    std::cout << "Timing new degree computation: ";
    boost::timer::auto_cpu_timer t;
    std::cout << ret2.degree() << '\n';
  }
  {
    std::cout << "Timing partial degree truncation:\n";
    boost::timer::auto_cpu_timer t;
    ret2 = ret1.truncate_degree(30, {"u", "z"});
  }
}
