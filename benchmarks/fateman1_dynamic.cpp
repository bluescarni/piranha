/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#include "fateman1.hpp"

#define BOOST_TEST_MODULE fateman1_test
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <limits>

#include <piranha/init.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/settings.hpp>

using namespace piranha;

// Fateman's polynomial multiplication test number 1. Calculate:
// f * (f+1)
// where f = (1+x+y+z+t)**20

BOOST_AUTO_TEST_CASE(fateman1_test)
{
    init();
    settings::set_thread_binding(true);
    using limb_t = typename detail::integer_union<0>::s_storage::limb_t;
    if (boost::unit_test::framework::master_test_suite().argc > 1) {
        settings::set_n_threads(
            boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
    }
    BOOST_CHECK_EQUAL(
        (fateman1<integer, kronecker_monomial<>>(static_cast<unsigned long long>(std::numeric_limits<limb_t>::max()))
             .size()),
        135751u);
}
