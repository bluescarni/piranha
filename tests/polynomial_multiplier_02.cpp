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

#include <piranha/polynomial.hpp>

#define BOOST_TEST_MODULE polynomial_multiplier_02_test
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <piranha/init.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/settings.hpp>

using namespace piranha;

using cf_types = boost::mpl::vector<double, integer, rational>;
using k_types = boost::mpl::vector<monomial<int>, monomial<integer>, monomial<rational>,
                                   // This should ensure in the overflow tests below we have enough bit width (we
                                   // are testing with 3 variables).
                                   kronecker_monomial<std::int_least64_t>>;

struct st_vs_mt_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            if (std::is_same<Cf, double>::value
                && (!std::numeric_limits<double>::is_iec559 || std::numeric_limits<double>::digits < 53)) {
                return;
            }
            // Compute result in st mode.
            settings::set_n_threads(1u);
            using p_type = polynomial<Cf, Key>;
            p_type x("x"), y("y"), z("z"), t("t");
            auto f = 1 + x + y + z + t;
            auto tmp2 = f;
            for (int i = 1; i < 10; ++i) {
                f *= tmp2;
            }
            auto g = f + 1;
            auto st = f * g;
            // Now compute the same quantity in mt mode and check the result
            // is the same as st mode.
            for (auto i = 2u; i <= 4u; ++i) {
                settings::set_n_threads(i);
                auto mt = f * g;
                BOOST_CHECK(mt == st);
            }
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<k_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_st_vs_mt_test)
{
    init();
    boost::mpl::for_each<cf_types>(st_vs_mt_tester());
    settings::reset_n_threads();
}

BOOST_AUTO_TEST_CASE(polynomial_multiplier_different_cf_test)
{
    settings::set_n_threads(1u);
    using p_type1 = polynomial<std::size_t, k_monomial>;
    using p_type2 = polynomial<integer, k_monomial>;
    p_type1 x("x"), y("y"), z("z"), t("t");
    auto f = 1 + x + y + z + t;
    p_type2 tmp2(f);
    for (int i = 1; i < 10; ++i) {
        f *= tmp2;
    }
    auto g = f + 1;
    auto st = f * g;
    BOOST_CHECK_EQUAL(st.size(), 10626u);
}

BOOST_AUTO_TEST_CASE(polynomial_multiplier_multiplier_finalise_test)
{
    // Test proper handling of rational coefficients.
    using pt1 = polynomial<rational, k_monomial>;
    using pt2 = polynomial<integer, k_monomial>;
    {
        pt1 x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL(x * 4 / 3_q * y * 5 / 2_q, 10 / 3_q * x * y);
        BOOST_CHECK_EQUAL((x * 4 / 3_q + y * 5 / 2_q) * (x.pow(2) * 4 / 13_q - y * 5 / 17_q),
                          16 * x.pow(3) / 39 + 10 / 13_q * y * x * x - 20 * x * y / 51 - 25 * y * y / 34);
    }
    // Let's do a check with a fateman1-like: first compute the exact result using integers,
    // then with rationals, then check the two are consistent. Do it with 1 and 4 threads.
    for (unsigned nt = 1u; nt <= 4; nt += 3) {
        settings::set_n_threads(nt);
        pt2 cmp;
        {
            pt2 x("x"), y("y"), z("z"), t("t");
            auto f = 1 + x + y + z + t;
            auto tmp2 = f;
            for (int i = 1; i < 10; ++i) {
                f *= tmp2;
            }
            auto g = f + 1;
            cmp = f * g;
        }
        pt1 res;
        {
            pt1 x("x"), y("y"), z("z"), t("t");
            auto f = 1 + x + y + z + t;
            auto tmp2 = f;
            for (int i = 1; i < 10; ++i) {
                f *= tmp2;
            }
            auto g = f + 1;
            res = f / 2 * g / 3;
        }
        BOOST_CHECK_EQUAL(cmp, res * 6);
    }
    settings::reset_n_threads();
}
