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

#include <piranha/polynomial.hpp>

#define BOOST_TEST_MODULE polynomial_truncation_test
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <piranha/detail/safe_integral_adder.hpp>
#include <piranha/init.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/real.hpp>
#include <piranha/settings.hpp>

using namespace piranha;

using cf_types = boost::mpl::vector<double, integer, rational>;
using key_types = boost::mpl::vector<monomial<int>, monomial<rational>, k_monomial>;

struct main_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            using pt = polynomial<Cf, Key>;
            BOOST_CHECK(detail::has_get_auto_truncate_degree<pt>::value);
            BOOST_CHECK((detail::has_set_auto_truncate_degree<pt, int>::value));
            BOOST_CHECK((!detail::has_set_auto_truncate_degree<pt, std::string>::value));
            settings::set_min_work_per_thread(1u);
            for (unsigned nt = 1u; nt <= 4u; ++nt) {
                settings::set_n_threads(nt);
                // First with no truncation.
                pt x{"x"}, y{"y"}, z{"z"}, t{"t"};
                auto tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 0);
                BOOST_CHECK_EQUAL((x + y + z + t) * (x + y + z + t), t * t + 2 * t * x + 2 * t * y + 2 * t * z + x * x
                                                                         + 2 * x * y + 2 * x * z + y * y + 2 * y * z
                                                                         + z * z);
                // Total degree truncation.
                pt::set_auto_truncate_degree(1);
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 1);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 1);
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x + 2 * y + 1);
                pt::set_auto_truncate_degree(2);
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 1);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 2);
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x + 2 * y + 1 + 2 * x * y + x * x + y * y);
                pt::set_auto_truncate_degree(3);
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 1);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 3);
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x + 2 * y + 1 + 2 * x * y + x * x + y * y);
                pt::set_auto_truncate_degree(0);
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 1);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 0);
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 1);
                pt::set_auto_truncate_degree(-1);
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 1);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), -1);
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 0);
                pt::set_auto_truncate_degree(1);
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 1);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 1);
                BOOST_CHECK_EQUAL((x + y + z + t) * (x + y + z + t), 0);
                // Try also with rational max degree, for fun.
                pt::set_auto_truncate_degree(1_q);
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x + 2 * y + 1);
                if (std::is_same<Key, monomial<rational>>::value) {
                    pt::set_auto_truncate_degree(1 / 2_q);
                    BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x + 2 * y + 1);
                } else {
                    BOOST_CHECK_THROW(pt::set_auto_truncate_degree(1 / 2_q), std::invalid_argument);
                }
                // Special checks when the degree is a C++ integral.
                if (std::is_same<monomial<int>, Key>::value) {
                    BOOST_CHECK((std::is_same<decltype(x.degree()), int>::value));
                    // NOTE: this is invalid_argument because the failure is in safe_cast.
                    BOOST_CHECK_THROW(pt::set_auto_truncate_degree(integer(std::numeric_limits<long long>::max()) + 1),
                                      std::invalid_argument);
                    // Check overflow in the limits logic.
                    constexpr auto max = std::numeric_limits<int>::max();
                    pt::set_auto_truncate_degree(max);
                    BOOST_CHECK_THROW((1 + x.pow(-1)) * x, std::overflow_error);
                    // This should not overflow, contrary to what it would seem like.
                    BOOST_CHECK_NO_THROW((x.pow(max / 2) * y.pow(max / 2) + 1) * (x.pow(max / 2) * y.pow(2)));
                    // This is what would happen if we used d1 + d2 <= M instead of d1 <= M - d2 in
                    // the truncation logic.
                    auto check = max / 2 + max / 2;
                    BOOST_CHECK_THROW(detail::safe_integral_adder(check, max / 2 + 2), std::overflow_error);
                }
                // Now partial degree.
                pt::set_auto_truncate_degree(1, {"x"});
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 2);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 1);
                BOOST_CHECK(std::get<2u>(tup) == std::vector<std::string>{"x"});
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x * y + 2 * x + y * y + 2 * y + 1);
                pt::set_auto_truncate_degree(1, {"z"});
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 2);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 1);
                BOOST_CHECK(std::get<2u>(tup) == std::vector<std::string>{"z"});
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), x * x + 2 * x * y + 2 * x + y * y + 2 * y + 1);
                pt::set_auto_truncate_degree(1, {"x", "y"});
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 2);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 1);
                BOOST_CHECK((std::get<2u>(tup) == std::vector<std::string>{"x", "y"}));
                BOOST_CHECK_EQUAL((x + y + 1) * (x + y + 1), 2 * x + 2 * y + 1);
                pt::set_auto_truncate_degree(1, {"x"});
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 2);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 1);
                BOOST_CHECK((std::get<2u>(tup) == std::vector<std::string>{"x"}));
                BOOST_CHECK_EQUAL((x + y + z + t) * (x + y + z + t), t * t + 2 * t * x + 2 * t * y + 2 * t * z
                                                                         + 2 * x * y + 2 * x * z + y * y + 2 * y * z
                                                                         + z * z);
                // Check that for another series type the truncation settings are untouched.
                auto tup2 = polynomial<real, Key>::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup2), 0);
                BOOST_CHECK_EQUAL(std::get<1u>(tup2), 0);
                BOOST_CHECK(std::get<2u>(tup2).empty());
                // Check the unsetting.
                pt::unset_auto_truncate_degree();
                tup = pt::get_auto_truncate_degree();
                BOOST_CHECK_EQUAL(std::get<0u>(tup), 0);
                BOOST_CHECK_EQUAL(std::get<1u>(tup), 0);
                BOOST_CHECK(std::get<2u>(tup).empty());
            }
            settings::reset_min_work_per_thread();
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<key_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_truncation_main_test)
{
    init();
    boost::mpl::for_each<cf_types>(main_tester());
}

BOOST_AUTO_TEST_CASE(polynomial_truncation_recursive_test)
{
    // A couple of simple truncation tests with recursive polynomials.
    using p1 = r_polynomial<1, integer, monomial<short>>;
    using p2 = r_polynomial<2, integer, monomial<short>>;
    using p3 = r_polynomial<3, integer, monomial<short>>;
    p1 x{"x"};
    p2 y{"y"};
    p3 z{"z"};
    p3::set_auto_truncate_degree(5);
    BOOST_CHECK(math::degree(x * y * z) == 3);
    BOOST_CHECK(math::degree(x * x * y * z) == 4);
    BOOST_CHECK(math::degree(x * x * x * y * z) == 5);
    BOOST_CHECK(x * x * x * y * y * z == 0);
    p3::unset_auto_truncate_degree();
    p2::set_auto_truncate_degree(4);
    BOOST_CHECK(math::degree(x * x * y * z) == 4);
    BOOST_CHECK(math::degree(x * x * y * y * z) == 5);
    BOOST_CHECK(x * x * y * y * y * z == 0);
    p2::unset_auto_truncate_degree();
    p1::set_auto_truncate_degree(4);
    BOOST_CHECK(math::degree(x * x * y * z) == 4);
    BOOST_CHECK(math::degree(x * x * x * y * z) == 5);
    BOOST_CHECK(math::degree(x * x * x * x * y * z) == 6);
    BOOST_CHECK(x * x * x * x * x * y * z == 0);
    p1::unset_auto_truncate_degree();
}
