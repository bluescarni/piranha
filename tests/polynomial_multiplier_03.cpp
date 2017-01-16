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

#define BOOST_TEST_MODULE polynomial_multiplier_03_test
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <stdexcept>

#include <piranha/init.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/series_multiplier.hpp>

using namespace piranha;

using cf_types = boost::mpl::vector<double, integer, rational>;
using k_types = boost::mpl::vector<monomial<int>, monomial<integer>, monomial<rational>, k_monomial>;

template <typename T, typename = decltype(T::untruncated_multiplication(T{}, T{}))>
constexpr bool has_untruncated_multiplication()
{
    return true;
}

template <typename T, typename... Args>
constexpr bool has_untruncated_multiplication(Args &&...)
{
    return false;
}

template <typename T, typename = decltype(T::truncated_multiplication(T{}, T{}, 4)),
          typename = decltype(T::truncated_multiplication(T{}, T{}, 4, std::vector<std::string>{}))>
constexpr bool has_truncated_multiplication()
{
    return true;
}

template <typename T, typename... Args>
constexpr bool has_truncated_multiplication(Args &&...)
{
    return false;
}

struct um_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            // First let's test the multiplier's method.
            using p_type = polynomial<Cf, Key>;
            BOOST_CHECK(has_untruncated_multiplication<p_type>());
            p_type x{"x"}, y{"y"};
            auto s1 = x + y, s2 = x - y;
            const auto ret = s1 * s2, ret2 = (x + y) * x, ret3 = x * y, ret4 = x * x;
            series_multiplier<p_type> sm0(s1, s2);
            BOOST_CHECK_EQUAL(sm0._untruncated_multiplication(), (x + y) * (x - y));
            p_type::set_auto_truncate_degree(1);
            BOOST_CHECK_EQUAL(sm0._untruncated_multiplication(), ret);
            BOOST_CHECK_EQUAL(s1 * s2, 0);
            p_type::set_auto_truncate_degree(1, {"y"});
            BOOST_CHECK_EQUAL(sm0._untruncated_multiplication(), ret);
            BOOST_CHECK_EQUAL(s1 * s2, x * x);
            p_type::unset_auto_truncate_degree();
            // Now the static mult method.
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x + y, x - y), ret);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x + y, x), ret2);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x, x + y), ret2);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x, y), ret3);
            p_type::set_auto_truncate_degree(1);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x + y, x - y), ret);
            BOOST_CHECK_EQUAL((x + y) * (x - y), 0);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x + y, x), ret2);
            BOOST_CHECK_EQUAL((x + y) * x, 0);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x, x + y), ret2);
            BOOST_CHECK_EQUAL(x * (x + y), 0);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x, y), ret3);
            BOOST_CHECK_EQUAL(x * y, 0);
            p_type::set_auto_truncate_degree(1, {"y"});
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x + y, x - y), ret);
            BOOST_CHECK_EQUAL((x + y) * (x - y), ret4);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x + y, x), ret2);
            BOOST_CHECK_EQUAL((x + y) * x, ret2);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x, x + y), ret2);
            BOOST_CHECK_EQUAL(x * (x + y), ret2);
            BOOST_CHECK_EQUAL(p_type::untruncated_multiplication(x, y), ret3);
            BOOST_CHECK_EQUAL(x * y, ret3);
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
    BOOST_CHECK((!has_untruncated_multiplication<polynomial<short, k_monomial>>()));
    BOOST_CHECK((!has_untruncated_multiplication<polynomial<char, k_monomial>>()));
}

struct tm_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            // First let's test the multiplier's method.
            using p_type = polynomial<Cf, Key>;
            BOOST_CHECK(has_truncated_multiplication<p_type>());
            p_type x{"x"}, y{"y"};
            const auto res1 = x * y + y * y, res2 = x * x, res3 = y * y;
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, x - y, 1), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, y, 2), res1);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(y, x + y, 2), res1);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x, y, 1), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, x - y, 1, {"y"}), res2);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, y, 1, {"x", "y"}), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(y, x + y, 0, {"x"}), res3);
            p_type::set_auto_truncate_degree(0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, x - y, 1), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, y, 2), res1);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(y, x + y, 2), res1);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x, y, 1), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, x - y, 1, {"y"}), res2);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, y, 1, {"x", "y"}), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(y, x + y, 0, {"x"}), res3);
            p_type::set_auto_truncate_degree(1, {"y"});
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, x - y, 1), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, y, 2), res1);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(y, x + y, 2), res1);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x, y, 1), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, x - y, 1, {"y"}), res2);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(x + y, y, 1, {"x", "y"}), 0);
            BOOST_CHECK_EQUAL(p_type::truncated_multiplication(y, x + y, 0, {"x"}), res3);
            p_type::unset_auto_truncate_degree();
            // Test for the safe_cast failure.
            using dtype = decltype(x.degree());
            if (!std::is_same<dtype, rational>::value) {
                BOOST_CHECK_THROW(p_type::truncated_multiplication(x + y, x - y, 1 / 2_q), std::invalid_argument);
                BOOST_CHECK_THROW(p_type::truncated_multiplication(x + y, x - y, 1 / 2_q, {"x", "y"}),
                                  std::invalid_argument);
            }
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<k_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_truncated_test)
{
    boost::mpl::for_each<cf_types>(tm_tester());
    BOOST_CHECK((!has_truncated_multiplication<polynomial<short, k_monomial>>()));
    BOOST_CHECK((!has_truncated_multiplication<polynomial<char, k_monomial>>()));
}
