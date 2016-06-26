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

#define BOOST_TEST_MODULE polynomial_multiplier_03_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/series_multiplier.hpp"

using namespace piranha;

using cf_types = boost::mpl::vector<double, integer, rational>;
using k_types = boost::mpl::vector<monomial<int>, monomial<integer>, monomial<rational>, k_monomial>;

struct um_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            using p_type = polynomial<Cf, Key>;
            p_type x{"x"}, y{"y"};
            auto s1 = x + y, s2 = x - y;
            series_multiplier<p_type> sm0(s1, s2);
            const auto ret = s1 * s2;
            BOOST_CHECK_EQUAL(sm0._untruncated_multiplication(), (x + y) * (x - y));
            p_type::set_auto_truncate_degree(1);
            BOOST_CHECK_EQUAL(sm0._untruncated_multiplication(), ret);
            BOOST_CHECK_EQUAL(s1 * s2, 0);
            p_type::set_auto_truncate_degree(1,{"y"});
            BOOST_CHECK_EQUAL(sm0._untruncated_multiplication(), ret);
            BOOST_CHECK_EQUAL(s1 * s2, x*x);
            p_type::unset_auto_truncate_degree();
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<k_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_untruncated_test)
{
    init();
    boost::mpl::for_each<cf_types>(um_tester());
}
