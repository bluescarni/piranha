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

#define BOOST_TEST_MODULE polynomial_05_test
#include <boost/test/unit_test.hpp>

#include "../src/init.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/pow.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(polynomial_truncation_pow_cache_test)
{
    init();
    using p_type = polynomial<integer, monomial<int>>;
    p_type x{"x"}, y{"y"};
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y+x*x+y*y+2*x*y);
    // Always test twice to exercise the other branch of the pow cache clearing function.
    p_type::set_auto_truncate_degree(1);
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y);
    p_type::set_auto_truncate_degree(1);
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y);
    p_type::set_auto_truncate_degree(1,{"x"});
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y+y*y+2*x*y);
    p_type::set_auto_truncate_degree(1,{"x"});
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y+y*y+2*x*y);
    p_type::set_auto_truncate_degree(1,{"y"});
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y+x*x+2*x*y);
    p_type::set_auto_truncate_degree(1,{"y"});
    BOOST_CHECK_EQUAL(math::pow(x+y+1,2),2*x+1+2*y+x*x+2*x*y);
}
