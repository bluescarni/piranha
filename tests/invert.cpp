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

#include <piranha/invert.hpp>

#define BOOST_TEST_MODULE invert_test
#include <boost/test/included/unit_test.hpp>

#include <cmath>
#include <string>
#include <type_traits>

#include <mp++/config.hpp>

#include <piranha/integer.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/monomial.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif

using namespace piranha;

using math::invert;
using math::pow;

#if defined(MPPP_WITH_MPFR)

static inline real operator"" _r(const char *s)
{
    return real(s, 100);
}

#endif

BOOST_AUTO_TEST_CASE(invert_test_00)
{
    // Some tests with non-piranha types.
    BOOST_CHECK(is_invertible<float>::value);
    BOOST_CHECK(is_invertible<double>::value);
    BOOST_CHECK(is_invertible<long double>::value);
    BOOST_CHECK_EQUAL(math::pow(1.5f, -1), invert(1.5f));
    BOOST_CHECK((std::is_same<decltype(std::pow(1.5f, -1)), decltype(invert(1.5f))>::value));
    BOOST_CHECK_EQUAL(math::pow(1.5, -1), invert(1.5));
    BOOST_CHECK((std::is_same<double, decltype(invert(1.5))>::value));
    BOOST_CHECK_EQUAL(math::pow(1.5l, -1), invert(1.5l));
    BOOST_CHECK((std::is_same<long double, decltype(invert(1.5l))>::value));
    BOOST_CHECK(!is_invertible<std::string>::value);
    BOOST_CHECK(!is_invertible<void>::value);
    // Test with piranha's scalar types.
    BOOST_CHECK(is_invertible<integer>::value);
    BOOST_CHECK(is_invertible<rational>::value);
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK(is_invertible<real>::value);
#endif
    BOOST_CHECK_EQUAL(invert(1_z), 1);
    BOOST_CHECK_EQUAL(invert(-1_z), -1);
    BOOST_CHECK_EQUAL(invert(2_z), 0);
    BOOST_CHECK((std::is_same<integer, decltype(invert(1_z))>::value));
    BOOST_CHECK_EQUAL(invert(1_q), 1);
    BOOST_CHECK_EQUAL(invert(1 / 2_q), 2);
    BOOST_CHECK_EQUAL(invert(-2 / 3_q), -3 / 2_q);
    BOOST_CHECK((std::is_same<rational, decltype(invert(1_q))>::value));
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK_EQUAL(invert(1_r), 1);
    BOOST_CHECK_EQUAL(invert(1.5_r), math::pow(1.5_r, -1));
    BOOST_CHECK_EQUAL(invert(-2.5_r), math::pow(-2.5_r, -1));
    BOOST_CHECK((std::is_same<real, decltype(invert(1_r))>::value));
#endif
    // Test with some series types.
    using p_type = polynomial<rational, monomial<short>>;
    BOOST_CHECK(is_invertible<p_type>::value);
    p_type x{"x"};
    BOOST_CHECK_EQUAL(invert(x), pow(x, -1));
    BOOST_CHECK((std::is_same<p_type, decltype(invert(x))>::value));
    using ps_type = poisson_series<p_type>;
    BOOST_CHECK(is_invertible<ps_type>::value);
    ps_type a{"a"};
    BOOST_CHECK_EQUAL(invert(a), pow(a, -1));
    BOOST_CHECK((std::is_same<ps_type, decltype(invert(a))>::value));
}
