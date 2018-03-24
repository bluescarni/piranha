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

#include <piranha/poisson_series.hpp>

#define BOOST_TEST_MODULE poisson_series_01_test
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include <mp++/config.hpp>
#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/integer.hpp>
#include <piranha/invert.hpp>
#include <piranha/math.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/degree.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/series.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using cf_types = std::tuple<double, rational, polynomial<rational, monomial<int>>>;

// Mock coefficient.
struct mock_cf {
    mock_cf();
    mock_cf(const int &);
    mock_cf(const mock_cf &);
    mock_cf(mock_cf &&) noexcept;
    mock_cf &operator=(const mock_cf &);
    mock_cf &operator=(mock_cf &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const mock_cf &);
    mock_cf operator-() const;
    bool operator==(const mock_cf &) const;
    bool operator!=(const mock_cf &) const;
    mock_cf &operator+=(const mock_cf &);
    mock_cf &operator-=(const mock_cf &);
    mock_cf operator+(const mock_cf &) const;
    mock_cf operator-(const mock_cf &) const;
    mock_cf &operator*=(const mock_cf &);
    mock_cf operator*(const mock_cf &)const;
    mock_cf &operator/=(int);
    mock_cf operator/(const mock_cf &) const;
};

BOOST_AUTO_TEST_CASE(poisson_series_partial_test)
{
#if defined(MPPP_WITH_MPFR)
    mppp::real_set_default_prec(100);
#endif
    using math::partial;
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    BOOST_CHECK(is_differentiable<p_type1>::value);
    BOOST_CHECK(has_pbracket<p_type1>::value);
    BOOST_CHECK(has_transformation_is_canonical<p_type1>::value);
    p_type1 x{"x"}, y{"y"};
    BOOST_CHECK_EQUAL(partial(x * piranha::cos(y), "x"), piranha::cos(y));
    BOOST_CHECK_EQUAL(partial(x * piranha::cos(2 * x), "x"), piranha::cos(2 * x) - 2 * x * piranha::sin(2 * x));
    BOOST_CHECK_EQUAL(partial(x * piranha::cos(2 * x + y), "y"), -x * piranha::sin(2 * x + y));
    BOOST_CHECK_EQUAL(partial(rational(3, 2) * piranha::cos(2 * x + y), "x"), -3 * piranha::sin(2 * x + y));
    BOOST_CHECK_EQUAL(partial(rational(3, 2) * x * piranha::cos(y), "y"), -rational(3, 2) * x * piranha::sin(+y));
    BOOST_CHECK_EQUAL(partial(piranha::pow(x * piranha::cos(y), 5), "y"),
                      5 * piranha::sin(-y) * x * piranha::pow(x * piranha::cos(y), 4));
    BOOST_CHECK_EQUAL(partial(piranha::pow(x * piranha::cos(y), 5), "z"), 0);
    // y as implicit function of x: y = cos(x).
    p_type1::register_custom_derivative(
        "x", [x](const p_type1 &p) { return p.partial("x") - partial(p, "y") * piranha::sin(x); });
    BOOST_CHECK_EQUAL(partial(x + piranha::cos(y), "x"), 1 + piranha::sin(y) * piranha::sin(x));
    BOOST_CHECK_EQUAL(partial(x + x * piranha::cos(y), "x"),
                      1 + piranha::cos(y) + x * piranha::sin(y) * piranha::sin(x));
    BOOST_CHECK((!is_differentiable<poisson_series<polynomial<mock_cf, monomial<short>>>>::value));
    BOOST_CHECK((!has_pbracket<poisson_series<polynomial<mock_cf, monomial<short>>>>::value));
    BOOST_CHECK((!has_transformation_is_canonical<poisson_series<polynomial<mock_cf, monomial<short>>>>::value));
}

BOOST_AUTO_TEST_CASE(poisson_series_transform_filter_test)
{
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    typedef std::decay<decltype(*(p_type1{}.begin()))>::type pair_type;
    typedef std::decay<decltype(*(p_type1{}.begin()->first.begin()))>::type pair_type2;
    p_type1 x{"x"}, y{"y"};
    auto s = piranha::pow(1 + x + y, 3) * piranha::cos(x) + piranha::pow(y, 3) * piranha::sin(x);
    auto s_t = s.transform([](const pair_type &p) {
        return std::make_pair(p.first.filter([](const pair_type2 &p2) { return piranha::degree(p2.second) < 2; }),
                              p.second);
    });
    BOOST_CHECK_EQUAL(s_t, (3 * x + 3 * y + 1) * piranha::cos(x));
}

#if defined(MPPP_WITH_MPFR)

BOOST_AUTO_TEST_CASE(poisson_series_evaluate_test)
{
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    symbol_fmap<real> dict{{"x", real(1.234)}, {"y", real(5.678)}};
    p_type1 x{"x"}, y{"y"};
    auto s1 = (x + y) * piranha::cos(x + y);
    auto tmp1 = (real(1.234) * 1_q + real(5.678) * 1_q) * piranha::cos(real(1.234) * short(1) + real(5.678) * short(1));
    BOOST_CHECK_EQUAL(math::evaluate(s1, dict), tmp1);
    BOOST_CHECK((std::is_same<real, decltype(math::evaluate(s1, dict))>::value));
    auto s2 = piranha::pow(y, 3) * piranha::sin(x + y);
    auto tmp2 = (real(0) + real(1) * piranha::pow(real(1.234), 0) * piranha::pow(real(5.678), 3))
                * piranha::sin(real(0) + real(1) * real(1.234) + real(1) * real(5.678));
    BOOST_CHECK_EQUAL(tmp2, math::evaluate(s2, dict));
    BOOST_CHECK((std::is_same<real, decltype(math::evaluate(s2, dict))>::value));
    // NOTE: here it seems to be quite a brittle test: if one changes the order of the operands s1 and s2,
    // the test fails on my test machine due to differences of order epsilon. Most likely it's a matter
    // of ordering of the floating-point operations and it will depend on a ton of factors. Better just disable it,
    // and keep this in mind if other tests start failing similarly.
    // BOOST_CHECK_EQUAL(tmp1 + tmp2,(s2 + s1).evaluate(dict));
}

#endif

BOOST_AUTO_TEST_CASE(poisson_series_subs_test)
{
    using math::subs;
    {
#if defined(MPPP_WITH_MPFR)
        typedef poisson_series<polynomial<real, monomial<short>>> p_type1;
        BOOST_CHECK((has_subs<p_type1, rational>::value));
        BOOST_CHECK((has_subs<p_type1, double>::value));
        BOOST_CHECK((has_subs<p_type1, integer>::value));
        BOOST_CHECK((!has_subs<p_type1, std::string>::value));
        BOOST_CHECK(p_type1{}.template subs<integer>({{"x", integer(4)}}).empty());
        p_type1 x{"x"}, y{"y"};
        auto s = (x + y) * piranha::cos(x) + piranha::pow(y, 3) * piranha::sin(x);
        BOOST_CHECK_EQUAL(s.template subs<real>({{"x", real(1.234)}}),
                          (real(1.234) + y) * piranha::cos(real(1.234))
                              + piranha::pow(y, 3) * piranha::sin(real(1.234)));
        BOOST_CHECK((std::is_same<decltype(s.template subs<real>({})), p_type1>::value));
        BOOST_CHECK((std::is_same<decltype(s.template subs<rational>({})), p_type1>::value));
        s = (x + y) * piranha::cos(x + y) + piranha::pow(y, 3) * piranha::sin(x + y);
        real r(1.234);
        BOOST_CHECK_EQUAL(s.template subs<real>({{"x", r}}),
                          (r + y) * (piranha::cos(r) * piranha::cos(y) - piranha::sin(r) * piranha::sin(y))
                              + piranha::pow(y, 3)
                                    * (piranha::sin(r) * piranha::cos(y) + piranha::cos(r) * piranha::sin(y)));
        BOOST_CHECK_EQUAL(subs<real>(s, {{"x", r}}),
                          (r + y) * (piranha::cos(r) * piranha::cos(y) - piranha::sin(r) * piranha::sin(y))
                              + piranha::pow(y, 3)
                                    * (piranha::sin(r) * piranha::cos(y) + piranha::cos(r) * piranha::sin(y)));
        BOOST_CHECK_EQUAL(subs<real>(s, {{"z", r}}), s);
        s = (x + y) * piranha::cos(-x + y) + piranha::pow(y, 3) * piranha::sin(-x + y);
        BOOST_CHECK_EQUAL(s.template subs<real>({{"x", r}}),
                          (r + y) * (piranha::cos(r) * piranha::cos(y) + piranha::sin(r) * piranha::sin(y))
                              + piranha::pow(y, 3)
                                    * (-piranha::sin(r) * piranha::cos(y) + piranha::cos(r) * piranha::sin(y)));
        s = (x + y) * piranha::cos(-2 * x + y) + piranha::pow(y, 3) * piranha::sin(-5 * x + y);
        BOOST_CHECK_EQUAL(s.template subs<real>({{"x", r}}),
                          (r + y) * (piranha::cos(r * 2) * piranha::cos(y) + piranha::sin(r * 2) * piranha::sin(y))
                              + piranha::pow(y, 3)
                                    * (-piranha::sin(r * 5) * piranha::cos(y) + piranha::cos(r * 5) * piranha::sin(y)));
        s = (x + y) * piranha::cos(-2 * x + y) + piranha::pow(x, 3) * piranha::sin(-5 * x + y);
        BOOST_CHECK_EQUAL(s.template subs<real>({{"x", r}}),
                          (r + y) * (piranha::cos(r * 2) * piranha::cos(y) + piranha::sin(r * 2) * piranha::sin(y))
                              + piranha::pow(r, 3)
                                    * (-piranha::sin(r * 5) * piranha::cos(y) + piranha::cos(r * 5) * piranha::sin(y)));
        typedef poisson_series<polynomial<rational, monomial<short>>> p_type2;
        BOOST_CHECK((has_subs<p_type2, rational>::value));
        BOOST_CHECK((has_subs<p_type2, double>::value));
        BOOST_CHECK((has_subs<p_type2, integer>::value));
        BOOST_CHECK((!has_subs<p_type2, std::string>::value));
        p_type2 a{"a"}, b{"b"};
        auto t = a * piranha::cos(a + b) + b * piranha::sin(a);
        BOOST_CHECK_EQUAL(t.template subs<p_type2>({{"a", b}}), b * piranha::cos(b + b) + b * piranha::sin(b));
        BOOST_CHECK_EQUAL(subs<p_type2>(t, {{"a", a + b}}),
                          (a + b) * piranha::cos(a + b + b) + b * piranha::sin(a + b));
        t = a * piranha::cos(-3 * a + b) + b * piranha::sin(-5 * a - b);
        BOOST_CHECK_EQUAL(subs<p_type2>(t, {{"a", a + b}}),
                          (a + b) * piranha::cos(-3 * (a + b) + b) + b * piranha::sin(-5 * (a + b) - b));
        BOOST_CHECK_EQUAL(subs<p_type2>(t, {{"a", 2 * (a + b)}}),
                          2 * (a + b) * piranha::cos(-6 * (a + b) + b) + b * piranha::sin(-10 * (a + b) - b));
        BOOST_CHECK_EQUAL(subs<p_type2>(t, {{"b", -5 * a}}), a * piranha::cos(-3 * a - 5 * a));
        BOOST_CHECK(t.template subs<p_type2>({{"b", 5 * a}}).template subs<rational>({{"a", rational(0)}}).empty());
        BOOST_CHECK_EQUAL((a * piranha::cos(b)).template subs<rational>({{"b", rational(0)}}), a);
        BOOST_CHECK_EQUAL((a * piranha::sin(b)).template subs<rational>({{"b", rational(0)}}), rational(0));
        BOOST_CHECK((std::is_same<decltype(subs<p_type2>(t, {{"a", a + b}})), p_type2>::value));
        // This was a bug in the substitution routine.
        p_type2 c{"c"}, d{"d"};
        BOOST_CHECK_EQUAL(math::subs<p_type2>(a + piranha::cos(b) - piranha::cos(b), {{"b", c + d}}), a);
        // This was a manifestation of the bug in the rational ctor from float.
        BOOST_CHECK_EQUAL(math::subs<integer>(-3 * piranha::pow(c, 4), {{"J_2", 0_z}}), -3 * piranha::pow(c, 4));
        // Test substitution with integral after piranha::sin/cos additional overload.
        BOOST_CHECK_EQUAL(math::subs<int>(-3 * piranha::pow(c, 4), {{"J_2", 0}}), -3 * piranha::pow(c, 4));
#endif
    }
    {
        // Test with eps.
        using eps = poisson_series<divisor_series<polynomial<rational, monomial<short>>, divisor<short>>>;
        eps x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK((has_subs<eps, rational>::value));
        BOOST_CHECK_EQUAL(math::subs<eps>(x, {{"x", y}}), y);
        BOOST_CHECK_EQUAL(math::subs<eps>(x, {{"x", x * y}}), x * y);
        BOOST_CHECK_EQUAL(math::subs<eps>(x * piranha::pow(z, -1), {{"z", x * y}}), x * piranha::pow(x * y, -1));
        BOOST_CHECK_EQUAL(math::subs<eps>(x * piranha::cos(z + y), {{"z", x - 2 * y}}), x * piranha::cos(x - y));
        BOOST_CHECK_EQUAL(math::subs<eps>(x * piranha::cos(x + y), {{"x", 2 * x}}), 2 * x * piranha::cos(2 * x + y));
        BOOST_CHECK_EQUAL(math::subs<eps>(x * piranha::cos(x + y), {{"y", 2 * x}}), x * piranha::cos(x + 2 * x));
        // No subs on divisors implemented (yet?).
        BOOST_CHECK_EQUAL(math::subs<eps>(x * piranha::cos(x + y) * math::invert(x), {{"x", 2 * x}}),
                          2 * x * piranha::cos(2 * x + y) * math::invert(x));
    }
}

BOOST_AUTO_TEST_CASE(poisson_series_print_tex_test)
{
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    p_type1 x{"x"}, y{"y"};
    std::ostringstream oss;
    std::string s1 = "3\\frac{{x}}{{y}}\\cos{\\left({x}+{y}\\right)}";
    std::string s2 = "2\\frac{{x}^{2}}{{y}^{2}}\\cos{\\left(3{x}\\right)}";
    ((3 * x * y.pow(-1)) * cos(x + y)).print_tex(oss);
    BOOST_CHECK_EQUAL(oss.str(), s1);
    oss.str("");
    ((3 * x * y.pow(-1)) * cos(x + y) - (2 * x.pow(2) * y.pow(-2)) * cos(-3 * x)).print_tex(oss);
    BOOST_CHECK(oss.str() == s1 + "-" + s2 || oss.str() == std::string("-") + s2 + "+" + s1);
    std::string s3 = "\\left({x}+{y}\\right)";
    std::string s4 = "\\left({y}+{x}\\right)";
    oss.str("");
    ((x + y) * cos(x)).print_tex(oss);
    BOOST_CHECK(oss.str() == s3 + "\\cos{\\left({x}\\right)}" || oss.str() == s4 + "\\cos{\\left({x}\\right)}");
}

BOOST_AUTO_TEST_CASE(poisson_series_integrate_test)
{
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    p_type1 x{"x"}, y{"y"}, z{"z"};
    BOOST_CHECK_EQUAL(p_type1{}.integrate("x"), p_type1{});
    BOOST_CHECK_EQUAL(x.integrate("x"), x * x / 2);
    BOOST_CHECK_EQUAL(x.pow(-2).integrate("x"), -x.pow(-1));
    BOOST_CHECK_EQUAL(math::integrate((x + y) * cos(x) + cos(y), "x"), (x + y) * piranha::sin(x) + x * cos(y) + cos(x));
    BOOST_CHECK_EQUAL(math::integrate((x + y) * cos(x) + cos(y), "y"), y / 2 * (2 * x + y) * cos(x) + piranha::sin(y));
    BOOST_CHECK_EQUAL(math::integrate((x + y) * cos(x) + cos(x), "x"), (x + y + 1) * piranha::sin(x) + cos(x));
    BOOST_CHECK_THROW(math::integrate(x.pow(-1) * cos(x), "x"), std::invalid_argument);
    BOOST_CHECK_THROW(math::integrate(x.pow(-2) * cos(x + y) + x, "x"), std::invalid_argument);
    // Some examples computed with Wolfram alpha for checking.
    BOOST_CHECK_EQUAL(math::integrate(x.pow(-2) * cos(x + y) + x, "y"), piranha::sin(x + y) * x.pow(-2) + x * y);
    BOOST_CHECK_EQUAL(math::integrate(x.pow(5) * y.pow(4) * z.pow(3) * cos(5 * x + 4 * y + 3 * z), "x"),
                      y.pow(4) * z.pow(3) / 3125
                          * (5 * x * (125 * x.pow(4) - 100 * x * x + 24) * piranha::sin(5 * x + 4 * y + 3 * z)
                             + (625 * x.pow(4) - 300 * x * x + 24) * cos(5 * x + 4 * y + 3 * z)));
    BOOST_CHECK_EQUAL(math::integrate(x.pow(5) / 37 * y.pow(4) * z.pow(3) * cos(5 * x - 4 * y + 3 * z), "y"),
                      x.pow(5) * z.pow(3) / 4736
                          * (4 * y * (8 * y * y - 3) * cos(5 * x - 4 * y + 3 * z)
                             + (-32 * y.pow(4) + 24 * y * y - 3) * piranha::sin(5 * x - 4 * y + 3 * z)));
    BOOST_CHECK_EQUAL(
        math::partial(math::integrate(x.pow(5) / 37 * y.pow(4) * z.pow(3) * cos(5 * x - 4 * y + 3 * z), "y"), "y"),
        x.pow(5) / 37 * y.pow(4) * z.pow(3) * cos(5 * x - 4 * y + 3 * z));
    BOOST_CHECK_EQUAL(
        math::partial(
            math::partial(
                math::integrate(math::integrate(x.pow(5) / 37 * y.pow(4) * z.pow(3) * cos(5 * x - 4 * y + 3 * z), "y"),
                                "y"),
                "y"),
            "y"),
        x.pow(5) / 37 * y.pow(4) * z.pow(3) * cos(5 * x - 4 * y + 3 * z));
    BOOST_CHECK_EQUAL(math::integrate(rational(1, 37) * y.pow(4) * z.pow(3) * cos(5 * x - 4 * y + 3 * z), "x"),
                      rational(1, 185) * y.pow(4) * z.pow(3) * piranha::sin(5 * x - 4 * y + 3 * z));
    BOOST_CHECK_EQUAL(math::integrate(rational(1, 37) * x.pow(4) * z.pow(3) * cos(4 * y - 3 * z), "x"),
                      rational(1, 185) * x.pow(5) * z.pow(3) * cos(4 * y - 3 * z));
    BOOST_CHECK_EQUAL(math::integrate(y.pow(-5) * cos(4 * x - 3 * z) - x * y * y * piranha::sin(y).pow(4), "x"),
                      (piranha::sin(4 * x - 3 * z) - 2 * x * x * y.pow(7) * piranha::sin(y).pow(4))
                          * (4 * y.pow(5)).pow(-1));
    BOOST_CHECK_EQUAL((x * x * cos(x)).integrate("x"), (x * x - 2) * piranha::sin(x) + 2 * x * cos(x));
    BOOST_CHECK_EQUAL(((x * x + y) * cos(x) - y * cos(x)).integrate("x"),
                      (x * x - 2) * piranha::sin(x) + 2 * x * cos(x));
    BOOST_CHECK_EQUAL(((x * x + y) * cos(x) + y * cos(x) - x * piranha::sin(y)).integrate("x"),
                      -(x * x) / 2 * piranha::sin(y) + (x * x + 2 * y - 2) * piranha::sin(x) + 2 * x * cos(x));
    BOOST_CHECK_EQUAL(
        ((x * x * x + y * x) * cos(2 * x - 3 * y) + y * x.pow(4) * cos(x) - (x.pow(-5) * piranha::sin(y)))
            .integrate("x"),
        x.pow(-4) / 8
            * (32 * (x * x - 6) * x.pow(5) * y * cos(x) + x.pow(4) * (6 * x * x + 2 * y - 3) * cos(2 * x - 3 * y)
               + 2
                     * (x.pow(5) * (2 * x * x + 2 * y - 3) * piranha::sin(2 * x - 3 * y)
                        + 4 * (x.pow(4) - 12 * x * x + 24) * x.pow(4) * y * piranha::sin(x) + piranha::sin(y))));
    BOOST_CHECK_EQUAL(math::integrate((x.pow(-1) * cos(y) + x * y * cos(x)).pow(2), "x"),
                      x.pow(-1) / 24
                          * (4 * x.pow(4) * y * y + 6 * x.pow(3) * y * y * piranha::sin(2 * x)
                             + 6 * x * x * y * y * cos(2 * x) - 3 * x * y * y * piranha::sin(2 * x)
                             + 24 * x * y * piranha::sin(x - y) + 24 * x * y * piranha::sin(x + y) - 12 * cos(2 * y)
                             - 12));
    BOOST_CHECK_EQUAL(math::integrate((cos(y) * x.pow(-2) + x * x * y * cos(x)).pow(2), "x"),
                      x.pow(5) * y * y / 10 - (cos(y).pow(2)) * x.pow(-3) / 3
                          + rational(1, 4) * (2 * x * x - 3) * x * y * y * cos(2 * x)
                          + rational(1, 8) * (2 * x.pow(4) - 6 * x * x + 3) * y * y * piranha::sin(2 * x)
                          + 2 * y * piranha::sin(x) * cos(y));
    BOOST_CHECK_EQUAL(math::integrate((x * cos(y) + y * cos(x)).pow(2), "x"),
                      rational(1, 6) * x * (x * x * cos(2 * y) + x * x + 3 * y * y)
                          + rational(1, 4) * y * y * piranha::sin(2 * x) + 2 * y * cos(x) * cos(y)
                          + 2 * x * y * piranha::sin(x) * cos(y));
    BOOST_CHECK_EQUAL(math::integrate((x * y * cos(y) + y * cos(x)).pow(2), "x"),
                      rational(1, 12) * y * y
                          * (2 * x * (x * x * cos(2 * y) + x * x + 3) + 24 * cos(x) * cos(y)
                             + 24 * x * piranha::sin(x) * cos(y) + 3 * piranha::sin(2 * x)));
    BOOST_CHECK_EQUAL(
        math::integrate((x * y * cos(y) + y * cos(x) + x * x * cos(x)).pow(2), "x"),
        rational(1, 60)
            * (15 * x * (2 * x * x + 2 * y - 3) * cos(x).pow(2)
               + x
                     * (6 * x.pow(4) + 5 * x * x * y * y + 10 * x * x * y * y * cos(y).pow(2)
                        + 5 * x * x * y * y * cos(2 * y) + 20 * x * x * y
                        - 15 * (2 * x * x + 2 * y - 3) * piranha::sin(x).pow(2)
                        + 120 * y * (x * x + y - 6) * piranha::sin(x) * cos(y) + 30 * y * y)
               + 15 * cos(x)
                     * (8 * y * (3 * x * x + y - 6) * cos(y)
                        + (2 * x.pow(4) + x * x * (4 * y - 6) + 2 * y * y - 2 * y + 3) * piranha::sin(x))));
    // This would require sine/cosine integral special functions.
    BOOST_CHECK_THROW(math::integrate((x * y.pow(-1) * cos(y) + y * cos(x) + x * x * cos(x)).pow(2), "y"),
                      std::invalid_argument);
    // Check type trait.
    BOOST_CHECK(is_integrable<p_type1>::value);
    BOOST_CHECK(is_integrable<p_type1 &>::value);
    BOOST_CHECK(is_integrable<const p_type1>::value);
    BOOST_CHECK(is_integrable<p_type1 const &>::value);
    typedef poisson_series<rational> p_type2;
    BOOST_CHECK_EQUAL(p_type2{}.integrate("x"), p_type2{});
    BOOST_CHECK_THROW(p_type2{1}.integrate("x"), std::invalid_argument);
    BOOST_CHECK(is_integrable<p_type2>::value);
    BOOST_CHECK(is_integrable<p_type2 &>::value);
    BOOST_CHECK(is_integrable<const p_type2>::value);
    BOOST_CHECK(is_integrable<p_type2 const &>::value);
    // Check with rational exponents and the new type-deducing logic.
    using p_type3 = poisson_series<polynomial<rational, monomial<rational>>>;
    using p_type4 = poisson_series<polynomial<integer, monomial<rational>>>;
    using p_type5 = poisson_series<polynomial<int, monomial<integer>>>;
    BOOST_CHECK(is_integrable<p_type3>::value);
    BOOST_CHECK(is_integrable<p_type4>::value);
    BOOST_CHECK(is_integrable<p_type5>::value);
    BOOST_CHECK((std::is_same<decltype(math::integrate(p_type3{}, "x")), p_type3>::value));
    BOOST_CHECK((std::is_same<decltype(math::integrate(p_type4{}, "x")), p_type3>::value));
    BOOST_CHECK((std::is_same<decltype(math::integrate(p_type5{}, "x")),
                              poisson_series<polynomial<integer, monomial<integer>>>>::value));
    BOOST_CHECK_EQUAL(math::integrate(p_type4{"x"}.pow(3 / 4_q), "x"), 4 / 7_q * p_type3{"x"}.pow(7 / 4_q));
    BOOST_CHECK_EQUAL(math::integrate(p_type3{"x"}.pow(8 / 4_q) * piranha::cos(p_type3{"x"}), "x"),
                      (p_type3{"x"} * p_type3{"x"} - 2) * piranha::sin(p_type3{"x"})
                          + 2 * p_type3{"x"} * piranha::cos(p_type3{"x"}));
    BOOST_CHECK_THROW(math::integrate(p_type3{"x"}.pow(3 / 4_q) * piranha::cos(p_type3{"x"}), "x"),
                      std::invalid_argument);
    // Check about eps.
    using p_type6 = poisson_series<divisor_series<polynomial<rational, monomial<short>>, divisor<short>>>;
    BOOST_CHECK(is_integrable<p_type6>::value);
    p_type6 a{"a"}, b{"b"}, c{"c"};
    using math::invert;
    BOOST_CHECK((std::is_same<p_type6, decltype(math::integrate(a, "a"))>::value));
    BOOST_CHECK_EQUAL(math::integrate(a, "a"), a * a / 2);
    BOOST_CHECK_EQUAL(math::integrate(b, "a"), a * b);
    BOOST_CHECK_EQUAL(math::integrate(b + a, "a"), a * a / 2 + a * b);
    BOOST_CHECK_EQUAL(math::integrate(invert(b) + a, "a"), a * a / 2 + a * invert(b));
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b) * a, "a"), a * a / 2 * piranha::cos(b));
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b) * a, "b"), a * piranha::sin(b));
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b) * a, "b"), a * piranha::sin(b));
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b) * a * invert(c), "b"), a * piranha::sin(b) * invert(c));
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b) * a * invert(c), "a"), piranha::cos(b) * a * a / 2 * invert(c));
    // This will fail because we do not know how to integrate with respect to divisors.
    BOOST_CHECK_THROW(math::integrate(piranha::cos(b) * a * invert(c), "c"), std::invalid_argument);
    // This will fail because, at the moment, eps cannot deal with mixed poly/trig variables (though
    // normal Poisson series can, this is something we need to fix in the future).
    BOOST_CHECK_THROW(math::integrate(piranha::cos(a) * a * invert(c), "a"), std::invalid_argument);
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b - a + a) * (a - c + c) * invert(c - b + b), "b"),
                      a * piranha::sin(b) * invert(c));
    BOOST_CHECK_EQUAL(math::integrate(piranha::cos(b + c - c) * (a + b - b) * invert(c - a + a), "a"),
                      piranha::cos(b) * a * a / 2 * invert(c));
}
