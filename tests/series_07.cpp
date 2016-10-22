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

#include "../src/series.hpp"

#define BOOST_TEST_MODULE series_07_test
#include <boost/test/unit_test.hpp>

#include <limits>

#include "../src/init.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(series_zero_is_absorbing_test)
{
    init();
    {
        using pt1 = polynomial<double, monomial<int>>;
        using pt2 = polynomial<pt1, monomial<int>>;
        if (std::numeric_limits<double>::has_quiet_NaN || std::numeric_limits<double>::has_signaling_NaN) {
            BOOST_CHECK((!zero_is_absorbing<pt1>::value));
            BOOST_CHECK((!zero_is_absorbing<pt1 &>::value));
            BOOST_CHECK((!zero_is_absorbing<const pt1 &>::value));
            BOOST_CHECK((!zero_is_absorbing<const pt1>::value));
            BOOST_CHECK((!zero_is_absorbing<pt1 &&>::value));
            BOOST_CHECK((!zero_is_absorbing<pt2>::value));
            BOOST_CHECK((!zero_is_absorbing<pt2 &>::value));
            BOOST_CHECK((!zero_is_absorbing<const pt2 &>::value));
            BOOST_CHECK((!zero_is_absorbing<const pt2>::value));
            BOOST_CHECK((!zero_is_absorbing<pt2 &&>::value));
        }
    }
    {
        using pt1 = polynomial<real, monomial<int>>;
        using pt2 = polynomial<pt1, monomial<int>>;
        BOOST_CHECK((!zero_is_absorbing<pt1>::value));
        BOOST_CHECK((!zero_is_absorbing<pt1 &>::value));
        BOOST_CHECK((!zero_is_absorbing<const pt1 &>::value));
        BOOST_CHECK((!zero_is_absorbing<const pt1>::value));
        BOOST_CHECK((!zero_is_absorbing<pt1 &&>::value));
        BOOST_CHECK((!zero_is_absorbing<pt2>::value));
        BOOST_CHECK((!zero_is_absorbing<pt2 &>::value));
        BOOST_CHECK((!zero_is_absorbing<const pt2 &>::value));
        BOOST_CHECK((!zero_is_absorbing<const pt2>::value));
        BOOST_CHECK((!zero_is_absorbing<pt2 &&>::value));
    }
}

BOOST_AUTO_TEST_CASE(series_fp_coefficient_test)
{
    using pt1 = polynomial<double, monomial<int>>;
    pt1 x{"x"};
    if (std::numeric_limits<double>::is_iec559) {
        BOOST_CHECK((pt1(0.) * pt1(std::numeric_limits<double>::infinity())).size() == 1u);
        BOOST_CHECK((0. * pt1(std::numeric_limits<double>::infinity())).size() == 1u);
        BOOST_CHECK((pt1(std::numeric_limits<double>::infinity()) * 0.).size() == 1u);
        BOOST_CHECK((pt1(0.) * std::numeric_limits<double>::infinity()).size() == 1u);
        BOOST_CHECK((std::numeric_limits<double>::infinity() * pt1(0.)).size() == 1u);
        BOOST_CHECK((pt1(0.) * pt1(-std::numeric_limits<double>::infinity())).size() == 1u);
        BOOST_CHECK((0. * pt1(-std::numeric_limits<double>::infinity())).size() == 1u);
        BOOST_CHECK((pt1(-std::numeric_limits<double>::infinity()) * 0.).size() == 1u);
        BOOST_CHECK((pt1(0.) * -std::numeric_limits<double>::infinity()).size() == 1u);
        BOOST_CHECK((-std::numeric_limits<double>::infinity() * pt1(0.)).size() == 1u);
        BOOST_CHECK((pt1(0.) * pt1(0.)).size() == 0u);
        BOOST_CHECK((pt1(0.) * pt1(1.)).size() == 0u);
        BOOST_CHECK((pt1(1.) * pt1(0.)).size() == 0u);
        BOOST_CHECK((pt1(0.) * (pt1(std::numeric_limits<double>::infinity()) + x)).size() == 1u);
        BOOST_CHECK((0. * (pt1(std::numeric_limits<double>::infinity()) - x)).size() == 1u);
        BOOST_CHECK(((pt1(std::numeric_limits<double>::infinity()) + x) * 0.).size() == 1u);
        BOOST_CHECK((pt1(0.) * (pt1(-std::numeric_limits<double>::infinity()) + x)).size() == 1u);
        BOOST_CHECK((0. * (pt1(-std::numeric_limits<double>::infinity()) - x)).size() == 1u);
        BOOST_CHECK(((pt1(-std::numeric_limits<double>::infinity()) + x) * 0.).size() == 1u);
    }
    using pt2 = polynomial<real, monomial<int>>;
    pt2 y{"y"};
    BOOST_CHECK((pt2(0.) * pt2(real{"inf"})).size() == 1u);
    BOOST_CHECK((0. * pt2(real{"inf"})).size() == 1u);
    BOOST_CHECK((pt2(real{"inf"}) * 0.).size() == 1u);
    BOOST_CHECK((pt2(0.) * real{"inf"}).size() == 1u);
    BOOST_CHECK((real{"inf"} * pt2(0.)).size() == 1u);
    BOOST_CHECK((pt2(0.) * pt2(-real{"inf"})).size() == 1u);
    BOOST_CHECK((0. * pt2(-real{"inf"})).size() == 1u);
    BOOST_CHECK((pt2(-real{"inf"}) * 0.).size() == 1u);
    BOOST_CHECK((pt2(0.) * -real{"inf"}).size() == 1u);
    BOOST_CHECK((-real{"inf"} * pt2(0.)).size() == 1u);
    BOOST_CHECK((pt2(0.) * pt2(0.)).size() == 0u);
    BOOST_CHECK((pt2(0.) * pt2(1.)).size() == 0u);
    BOOST_CHECK((pt2(1.) * pt2(0.)).size() == 0u);
    BOOST_CHECK((pt2(0.) * (pt2(real{"inf"}) + x)).size() == 1u);
    BOOST_CHECK((0. * (pt2(real{"inf"}) - x)).size() == 1u);
    BOOST_CHECK(((pt2(real{"inf"}) + x) * 0.).size() == 1u);
    BOOST_CHECK((pt2(0.) * (pt2(-real{"inf"}) + x)).size() == 1u);
    BOOST_CHECK((0. * (pt2(-real{"inf"}) - x)).size() == 1u);
    BOOST_CHECK(((pt2(-real{"inf"}) + x) * 0.).size() == 1u);
}
