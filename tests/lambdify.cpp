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

#include <piranha/lambdify.hpp>

#define BOOST_TEST_MODULE lambdify_test
#include <boost/test/included/unit_test.hpp>

#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <piranha/exceptions.hpp>
#include <piranha/init.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/real.hpp>

using namespace piranha;
using math::lambdify;
using math::evaluate;

static std::mt19937 rng;

static const int ntrials = 100;

struct callable_42 {
    double operator()(const std::vector<double> &) const
    {
        return 42.;
    }
};

struct callable_12 {
    double operator()(const std::vector<double> &) const
    {
        return 12.;
    }
};

struct callable_generic {
    template <typename T>
    T operator()(const std::vector<T> &) const
    {
        return T{0};
    }
};

BOOST_AUTO_TEST_CASE(lambdify_test_00)
{
    init();
    {
        using p_type = polynomial<integer, k_monomial>;
        p_type x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK((has_lambdify<p_type, integer>::value));
        auto l0 = lambdify<integer>(x + y + z, {"x", "y", "z"});
        BOOST_CHECK(!std::is_copy_assignable<decltype(l0)>::value);
        BOOST_CHECK(!std::is_move_assignable<decltype(l0)>::value);
        BOOST_CHECK(std::is_copy_constructible<decltype(l0)>::value);
        BOOST_CHECK(std::is_move_constructible<decltype(l0)>::value);
        BOOST_CHECK((std::is_same<decltype(l0({})), integer>::value));
        BOOST_CHECK_EQUAL(l0({1_z, 2_z, 3_z}), 6);
        auto l1 = lambdify<integer>(x + 2 * y + 3 * z, {"y", "z", "x"});
        BOOST_CHECK_EQUAL(l1({1_z, 2_z, 3_z}), 2 * 1 + 3 * 2 + 3);
        BOOST_CHECK_THROW(lambdify<integer>(x + 2 * y + 3 * z, {"y", "z", "x", "x"}), std::invalid_argument);
        BOOST_CHECK((has_lambdify<p_type, rational>::value));
        auto l2 = lambdify<rational>(x * x - 2 * y + 3 * z * z * z, {"x", "y", "z", "a"});
        BOOST_CHECK((std::is_same<decltype(l2({})), rational>::value));
        BOOST_CHECK_THROW(l2({1_q, 2_q, 3_q}), std::invalid_argument);
        BOOST_CHECK_THROW(l2({1_q, 2_q, 3_q, 4_q, 5_q}), std::invalid_argument);
        BOOST_CHECK_EQUAL(l2({1 / 7_q, -2 / 5_q, 2 / 3_q, 15_q}),
                          1 / 7_q * 1 / 7_q - 2 * -2 / 5_q + 3 * 2 / 3_q * 2 / 3_q * 2 / 3_q);
        BOOST_CHECK((has_lambdify<p_type, double>::value));
        auto l3 = lambdify<double>(x * x - 2 * y + 3 * z * z * z, {"x", "y", "z"});
        BOOST_CHECK((std::is_same<decltype(l3({})), double>::value));
        BOOST_CHECK((has_lambdify<p_type, p_type>::value));
        auto l4 = lambdify<p_type>(x * x - 2 * y + 3 * z * z * z, {"x", "y", "z"});
        BOOST_CHECK((std::is_same<decltype(l4({})), p_type>::value));
        // Try with copy construction as well.
        auto tmp = x - z;
        auto l5 = lambdify<double>(tmp, {"x", "y", "z"});
        BOOST_CHECK((std::is_same<decltype(l5({})), double>::value));
        BOOST_CHECK_EQUAL(l5({1., 2., 3.}), 1. - 3.);
        BOOST_CHECK_THROW(l5({1., 3.}), std::invalid_argument);
    }
    {
        BOOST_CHECK((has_lambdify<double, integer>::value));
        BOOST_CHECK((has_lambdify<double &&, integer>::value));
        BOOST_CHECK((has_lambdify<double &&, const integer>::value));
        BOOST_CHECK((has_lambdify<double &&, const integer &>::value));
        BOOST_CHECK((has_lambdify<double, std::string>::value));
        BOOST_CHECK((has_lambdify<double, rational>::value));
        auto l0 = lambdify<integer>(3.4, {});
        BOOST_CHECK((std::is_same<double, decltype(l0({}))>::value));
        BOOST_CHECK_EQUAL(l0({}), 3.4);
        BOOST_CHECK_THROW(l0({1_z, 2_z, 3_z}), std::invalid_argument);
    }
    {
        // Various checks with the extra symbol map.
        using p_type = polynomial<integer, k_monomial>;
        p_type x{"x"}, y{"y"}, z{"z"};
        auto l0 = lambdify<integer>(x + y + z, {"x"}, {{"z",
                                                        [](const std::vector<integer> &v) -> integer {
                                                            BOOST_CHECK_EQUAL(v.size(), 1u);
                                                            // z is 3*x.
                                                            return v[0] * 3_z;
                                                        }},
                                                       {"y", [](const std::vector<integer> &v) -> integer {
                                                            BOOST_CHECK_EQUAL(v.size(), 1u);
                                                            // y is 2*x.
                                                            return v[0] * 2_z;
                                                        }}});
        BOOST_CHECK_EQUAL(l0({1_z}), 6);
        BOOST_CHECK_EQUAL(l0({2_z}), 12);
        BOOST_CHECK_EQUAL(l0({0_z}), 0);
        BOOST_CHECK_EQUAL(l0({-3_z}), -18);
        auto l1 = lambdify<integer>(x + y + z, {"x"}, {{"z", [](const std::vector<integer> &v) -> integer {
                                                            BOOST_CHECK_EQUAL(v.size(), 1u);
                                                            return 3_z;
                                                        }}});
        // We cannot evaluate as the y evaluation value is missing.
        BOOST_CHECK_THROW(l1({1_z}), std::invalid_argument);
        // Too many values provided.
        BOOST_CHECK_THROW(l1({1_z, 2_z}), std::invalid_argument);
        // Check an init list that contains duplicates.
        BOOST_CHECK_EQUAL(lambdify<integer>(x + y, {"x"}, {{"y",
                                                            [](const std::vector<integer> &v) -> integer {
                                                                BOOST_CHECK_EQUAL(v.size(), 1u);
                                                                return 4_z;
                                                            }},
                                                           {"y",
                                                            [](const std::vector<integer> &v) -> integer {
                                                                BOOST_CHECK_EQUAL(v.size(), 1u);
                                                                return 3_z;
                                                            }}})({1_z}),
                          5);
        // Check with extra non-evaluated args.
        BOOST_CHECK_EQUAL(lambdify<integer>(x + y, {"x", "z"}, {{"y",
                                                                 [](const std::vector<integer> &v) -> integer {
                                                                     BOOST_CHECK_EQUAL(v.size(), 2u);
                                                                     return 4_z;
                                                                 }},
                                                                {"t",
                                                                 [](const std::vector<integer> &v) -> integer {
                                                                     BOOST_CHECK_EQUAL(v.size(), 2u);
                                                                     return 3_z;
                                                                 }}})({1_z, 123_z}),
                          5);
        // Check with extra symbol already in positional args.
        BOOST_CHECK_THROW(lambdify<integer>(x + y, {"x", "y"}, {{"y",
                                                                 [](const std::vector<integer> &v) -> integer {
                                                                     BOOST_CHECK_EQUAL(v.size(), 2u);
                                                                     return 4_z;
                                                                 }}})({1_z, 123_z}),
                          std::invalid_argument);
        // Another error check.
        BOOST_CHECK_THROW(lambdify<integer>(x + y, {"x"}, {{"y",
                                                            [](const std::vector<integer> &v) -> integer {
                                                                BOOST_CHECK_EQUAL(v.size(), 2u);
                                                                return 4_z;
                                                            }}})({1_z, 123_z}),
                          std::invalid_argument);
        // A test with only custom symbols.
        BOOST_CHECK_EQUAL(lambdify<integer>(x + y, {}, {{"x",
                                                         [](const std::vector<integer> &v) -> integer {
                                                             BOOST_CHECK_EQUAL(v.size(), 0u);
                                                             return 4_z;
                                                         }},
                                                        {"y",
                                                         [](const std::vector<integer> &v) -> integer {
                                                             BOOST_CHECK_EQUAL(v.size(), 0u);
                                                             return 3_z;
                                                         }}})({}),
                          7);
        // A couple of tests with nothing.
        BOOST_CHECK_EQUAL(lambdify<integer>(p_type{}, {})({}), 0);
        BOOST_CHECK_EQUAL(lambdify<integer>(p_type{}, {"x", "y"},
                                            {{"z", [](const std::vector<integer> &) { return 1_z; }}})({1_z, 2_z}),
                          0);
        // Check with non-lambda callables.
        BOOST_CHECK_EQUAL(lambdify<double>(x + y, {"x"}, {{"y", callable_42{}}})({1.}), 43.);
        BOOST_CHECK_EQUAL(lambdify<double>(x + y, {"x"}, {{"y", callable_generic{}}})({1.}), 1.);
        BOOST_CHECK_EQUAL(lambdify<integer>(x + y, {"x"}, {{"y", callable_generic{}}})({2_z}), 2);
        BOOST_CHECK_EQUAL(lambdify<double>(x + y, {"x"}, {{"y", callable_12{}}})({-1.}), 11.);
        BOOST_CHECK_EQUAL(lambdify<double>(x + y + z, {"x"}, {{"y", callable_12{}}, {"z", callable_42{}}})({-1.}),
                          -1 + 42. + 12.);
    }
}

BOOST_AUTO_TEST_CASE(lambdify_test_01)
{
    {
        // A few tests with copies and moves.
        using p_type = polynomial<integer, k_monomial>;
        p_type x{"x"}, y{"y"}, z{"z"};
        auto l0 = lambdify<integer>(x + y + z, {"x", "y", "z"});
        auto l1(l0);
        BOOST_CHECK_EQUAL(l0({1_z, 2_z, 3_z}), l1({1_z, 2_z, 3_z}));
        auto l2(std::move(l1));
        BOOST_CHECK_EQUAL(l0({1_z, 2_z, 3_z}), l2({1_z, 2_z, 3_z}));
        // Random testing.
        std::uniform_int_distribution<int> dist(-10, 10);
        const auto tmp = x * x - 6 * y + z * y * x;
        auto l = lambdify<integer>(tmp, {"y", "x", "z"});
        for (int i = 0; i < ntrials; ++i) {
            auto xn = integer(dist(rng)), yn = integer(dist(rng)), zn = integer(dist(rng));
            BOOST_CHECK_EQUAL(l({yn, xn, zn}), evaluate<integer>(tmp, {{"x", xn}, {"y", yn}, {"z", zn}}));
        }
    }
    {
        using p_type = polynomial<integer, k_monomial>;
        p_type x{"x"}, y{"y"}, z{"z"};
        auto l0 = lambdify<integer>(x + y + z, {"x", "y"}, {{"z", [](const std::vector<integer> &v) -> integer {
                                                                 BOOST_CHECK_EQUAL(v.size(), 2u);
                                                                 return v[0] * v[1];
                                                             }}});
        auto l1(l0);
        BOOST_CHECK_EQUAL(l0({1_z, 2_z}), l1({1_z, 2_z}));
        BOOST_CHECK_EQUAL(l0({1_z, 2_z}), 5);
        auto l2(std::move(l1));
        BOOST_CHECK_EQUAL(l0({1_z, 2_z}), l2({1_z, 2_z}));
        BOOST_CHECK_EQUAL(l0({1_z, 2_z}), 5);
        // Random testing.
        std::uniform_int_distribution<int> dist(-10, 10);
        const auto tmp = x * x - 6 * y + z * y * x;
        auto l = lambdify<integer>(tmp, {"y", "x"}, {{"z", [](const std::vector<integer> &v) -> integer {
                                                          BOOST_CHECK_EQUAL(v.size(), 2u);
                                                          return v[0] * v[1];
                                                      }}});
        for (int i = 0; i < ntrials; ++i) {
            auto xn = integer(dist(rng)), yn = integer(dist(rng));
            BOOST_CHECK_EQUAL(l({yn, xn}), evaluate<integer>(tmp, {{"x", xn}, {"y", yn}, {"z", xn * yn}}));
        }
    }
}

BOOST_AUTO_TEST_CASE(lambdify_test_02)
{
    // Test getters.
    using p_type = polynomial<integer, k_monomial>;
    p_type x{"x"}, y{"y"}, z{"z"};
    auto l0 = lambdify<integer>(x + y + z, {"z", "y", "x"});
    BOOST_CHECK(!std::is_copy_assignable<decltype(l0)>::value);
    BOOST_CHECK(!std::is_move_assignable<decltype(l0)>::value);
    BOOST_CHECK_EQUAL(x + y + z, l0.get_evaluable());
    const auto v = std::vector<std::string>({"z", "y", "x"});
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), l0.get_names().begin(), l0.get_names().end());
    auto en = l0.get_extra_names();
    BOOST_CHECK(en.empty());
    auto l1 = lambdify<integer>(x + y + z, {"z", "y", "x"}, {{"t", [](const std::vector<integer> &) { return 1_z; }}});
    en = l1.get_extra_names();
    BOOST_CHECK(en == std::vector<std::string>{"t"});
    auto l2 = lambdify<integer>(x + y + z, {"z", "y", "x"}, {{"t", [](const std::vector<integer> &) { return 1_z; }},
                                                             {"a", [](const std::vector<integer> &) { return 1_z; }}});
    en = l2.get_extra_names();
    BOOST_CHECK((en == std::vector<std::string>{"t", "a"} || en == std::vector<std::string>{"a", "t"}));
}
