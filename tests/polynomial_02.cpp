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

#define BOOST_TEST_MODULE polynomial_02_test
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <limits>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <piranha/base_series_multiplier.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/init.hpp>
#include <piranha/invert.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/pow.hpp>
#include <piranha/real.hpp>
#include <piranha/s11n.hpp>
#include <piranha/series.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/settings.hpp>
#include <piranha/symbol.hpp>

using namespace piranha;

typedef boost::mpl::vector<double, integer, rational> cf_types;

template <typename Cf, typename Expo>
class polynomial_alt : public series<Cf, monomial<Expo>, polynomial_alt<Cf, Expo>>
{
    typedef series<Cf, monomial<Expo>, polynomial_alt<Cf, Expo>> base;

public:
    polynomial_alt() = default;
    polynomial_alt(const polynomial_alt &) = default;
    polynomial_alt(polynomial_alt &&) = default;
    explicit polynomial_alt(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set.add(symbol(name));
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{Expo(1)}));
    }
    PIRANHA_FORWARDING_CTOR(polynomial_alt, base)
    ~polynomial_alt() = default;
    polynomial_alt &operator=(const polynomial_alt &) = default;
    polynomial_alt &operator=(polynomial_alt &&) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(polynomial_alt, base)
};

namespace piranha
{

template <typename Cf, typename Expo>
class series_multiplier<polynomial_alt<Cf, Expo>, void> : public base_series_multiplier<polynomial_alt<Cf, Expo>>
{
    using base = base_series_multiplier<polynomial_alt<Cf, Expo>>;
    template <typename T>
    using call_enabler = typename std::enable_if<key_is_multipliable<typename T::term_type::cf_type,
                                                                     typename T::term_type::key_type>::value,
                                                 int>::type;

public:
    using base::base;
    template <typename T = polynomial_alt<Cf, Expo>, call_enabler<T> = 0>
    polynomial_alt<Cf, Expo> operator()() const
    {
        return this->plain_multiplication();
    }
};
}

struct multiplication_tester {
    template <typename Cf, typename std::enable_if<is_mp_rational<Cf>::value, int>::type = 0>
    void operator()(const Cf &)
    {
        typedef polynomial<Cf, monomial<int>> p_type;
        typedef polynomial_alt<Cf, int> p_type_alt;
        p_type x("x"), y("y"), z("z"), t("t"), u("u");
        // Dense case, default setup.
        auto f = 1 + x + y + z + t;
        auto tmp(f);
        for (int i = 1; i < 10; ++i) {
            f *= tmp;
        }
        auto g = f + 1;
        auto retval = f * g;
        BOOST_CHECK_EQUAL(retval.size(), 10626u);
        auto retval_alt = p_type_alt(f) * p_type_alt(g);
        BOOST_CHECK(retval == p_type{retval_alt});
        // Dense case, force number of threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * g;
            auto tmp_alt = p_type_alt(f) * p_type_alt(g);
            BOOST_CHECK_EQUAL(tmp1.size(), 10626u);
            BOOST_CHECK(tmp1 == retval);
            BOOST_CHECK(tmp1 == p_type{tmp_alt});
        }
        settings::reset_n_threads();
        // Dense case with cancellations, default setup.
        auto h = 1 - x + y + z + t;
        tmp = h;
        for (int i = 1; i < 10; ++i) {
            h *= tmp;
        }
        retval = f * h;
        retval_alt = p_type_alt(f) * p_type_alt(h);
        BOOST_CHECK_EQUAL(retval.size(), 5786u);
        BOOST_CHECK(retval == p_type{retval_alt});
        // Dense case with cancellations, force number of threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * h;
            auto tmp_alt = p_type_alt(f) * p_type_alt(h);
            BOOST_CHECK_EQUAL(tmp1.size(), 5786u);
            BOOST_CHECK(retval == tmp1);
            BOOST_CHECK(tmp_alt == p_type_alt{tmp1});
        }
        settings::reset_n_threads();
        // Sparse case, default.
        f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
        auto tmp_f(f);
        g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
        auto tmp_g(g);
        h = (-u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
        auto tmp_h(h);
        for (int i = 1; i < 8; ++i) {
            f *= tmp_f;
            g *= tmp_g;
            h *= tmp_h;
        }
        retval = f * g;
        BOOST_CHECK_EQUAL(retval.size(), 591235u);
        retval_alt = p_type_alt(f) * p_type_alt(g);
        BOOST_CHECK(retval == p_type{retval_alt});
        // Sparse case, force n threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * g;
            auto tmp_alt = p_type_alt(f) * p_type_alt(g);
            BOOST_CHECK_EQUAL(tmp1.size(), 591235u);
            BOOST_CHECK(retval == tmp1);
            BOOST_CHECK(tmp_alt == p_type_alt{tmp1});
        }
        settings::reset_n_threads();
        // Sparse case with cancellations, default.
        retval = f * h;
        BOOST_CHECK_EQUAL(retval.size(), 591184u);
        retval_alt = p_type_alt(f) * p_type_alt(h);
        BOOST_CHECK(retval_alt == p_type_alt{retval});
        // Sparse case with cancellations, force number of threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * h;
            auto tmp_alt = p_type_alt(f) * p_type_alt(h);
            BOOST_CHECK_EQUAL(tmp1.size(), 591184u);
            BOOST_CHECK(tmp1 == retval);
            BOOST_CHECK(tmp1 == p_type{tmp_alt});
        }
    }
    template <typename Cf, typename std::enable_if<!is_mp_rational<Cf>::value, int>::type = 0>
    void operator()(const Cf &)
    {
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_test)
{
    init();
    boost::mpl::for_each<cf_types>(multiplication_tester());
}

BOOST_AUTO_TEST_CASE(polynomial_subs_test)
{
    {
        typedef polynomial<rational, monomial<short>> p_type1;
        BOOST_CHECK((has_subs<p_type1, rational>::value));
        BOOST_CHECK((has_subs<p_type1, double>::value));
        BOOST_CHECK((has_subs<p_type1, integer>::value));
        BOOST_CHECK((!has_subs<p_type1, std::string>::value));
        BOOST_CHECK_EQUAL(p_type1{"x"}.subs("x", integer(1)), 1);
        BOOST_CHECK_EQUAL(p_type1{"x"}.subs("x", p_type1{"x"}), p_type1{"x"});
        p_type1 x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL((math::pow(x,2) + x * y + z).subs("x", integer(3)), 9 + 3 * y + z);
        BOOST_CHECK_EQUAL((math::pow(x,2) + x * y + z).subs("y", rational(3, 2)), x * x + x * rational(3, 2) + z);
        BOOST_CHECK_EQUAL((math::pow(x,2) + x * y + z).subs("k", rational(3, 2)), x * x + x * y + z);
        BOOST_CHECK_EQUAL(math::pow(x,-1).subs("x", math::pow(x,-1)), x);
        BOOST_CHECK_EQUAL(
            (math::pow(x,2) + x * y + z).subs("x", rational(3, 2)).subs("y", rational(4, 5)).subs("z", -rational(6, 7)),
            (math::pow(x,2) + x * y + z)
                .evaluate(std::unordered_map<std::string, rational>{
                    {"x", rational(3, 2)}, {"y", rational(4, 5)}, {"z", -rational(6, 7)}}));
        BOOST_CHECK_EQUAL(
            math::subs(math::pow(x,2) + x * y + z, "x", rational(3, 2)).subs("y", rational(4, 5)).subs("z", -rational(6, 7)),
            (math::pow(x,2) + x * y + z)
                .evaluate(std::unordered_map<std::string, rational>{
                    {"x", rational(3, 2)}, {"y", rational(4, 5)}, {"z", -rational(6, 7)}}));
        BOOST_CHECK((std::is_same<decltype(p_type1{"x"}.subs("x", integer(1))), p_type1>::value));
        BOOST_CHECK((std::is_same<decltype(p_type1{"x"}.subs("x", rational(1))), p_type1>::value));
        BOOST_CHECK_EQUAL((math::pow(x,2) + x * y + z).subs("k", rational(3, 2)), x * x + x * y + z);
        BOOST_CHECK_EQUAL((math::pow(y + 4 * z,5) * math::pow(x,-1)).subs("x", rational(3)), (math::pow(y + 4 * z,5)) / 3);
    }
    {
        typedef polynomial<real, monomial<int>> p_type2;
        BOOST_CHECK((has_subs<p_type2, rational>::value));
        BOOST_CHECK((has_subs<p_type2, double>::value));
        BOOST_CHECK((has_subs<p_type2, integer>::value));
        BOOST_CHECK((!has_subs<p_type2, std::string>::value));
        p_type2 x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL((x * x * x + y * y).subs("x", real(1.234)), y * y + math::pow(real(1.234), 3));
        BOOST_CHECK_EQUAL((x * x * x + y * y).subs("x", real(1.234)).subs("y", real(-5.678)),
                          math::pow(real(-5.678), 2) + math::pow(real(1.234), 3));
        BOOST_CHECK_EQUAL(math::subs(x * x * x + y * y, "x", real(1.234)).subs("y", real(-5.678)),
                          math::pow(real(-5.678), 2) + math::pow(real(1.234), 3));
    }
    typedef polynomial<integer, monomial<long>> p_type3;
    BOOST_CHECK((has_subs<p_type3, rational>::value));
    BOOST_CHECK((has_subs<p_type3, double>::value));
    BOOST_CHECK((has_subs<p_type3, integer>::value));
    BOOST_CHECK((!has_subs<p_type3, std::string>::value));
    p_type3 x{"x"}, y{"y"}, z{"z"};
    BOOST_CHECK_EQUAL((x * x * x + y * y + z * y * x)
                          .subs("x", integer(2))
                          .subs("y", integer(-3))
                          .subs("z", integer(4))
                          .subs("k", integer()),
                      math::pow(integer(2),3) + math::pow(integer(-3),2) + integer(2) * integer(-3) * integer(4));
    BOOST_CHECK_EQUAL(math::subs(x * x * x + y * y + z * y * x, "x", integer(2))
                          .subs("y", integer(-3))
                          .subs("z", integer(4))
                          .subs("k", integer()),
                      math::pow(integer(2),3) + math::pow(integer(-3),2) + integer(2) * integer(-3) * integer(4));
    BOOST_CHECK_EQUAL((x * x * x + y * y + z * y * x)
                          .subs("x", integer(0))
                          .subs("y", integer(0))
                          .subs("z", integer(0))
                          .subs("k", integer()),
                      0);
}

BOOST_AUTO_TEST_CASE(polynomial_integrate_test)
{
    // Simple echelon-1 polynomial.
    typedef polynomial<rational, monomial<short>> p_type1;
    BOOST_CHECK(is_integrable<p_type1>::value);
    BOOST_CHECK(is_integrable<p_type1 &>::value);
    BOOST_CHECK(is_integrable<const p_type1>::value);
    BOOST_CHECK(is_integrable<p_type1 const &>::value);
    p_type1 x("x"), y("y"), z("z");
    BOOST_CHECK_EQUAL(p_type1{}.integrate("x"), p_type1{});
    BOOST_CHECK_EQUAL(x.integrate("x"), x * x / 2);
    BOOST_CHECK_EQUAL(y.integrate("x"), x * y);
    BOOST_CHECK_EQUAL((x + 3 * y * x * x + z * y * x / 4).integrate("x"),
                      x * x / 2 + y * x * x * x + z * y * x * x / 8);
    BOOST_CHECK_THROW(x.pow(-1).integrate("x"), std::invalid_argument);
    BOOST_CHECK_EQUAL((x + 3 * y * x * x + z * y * x / 4).integrate("x").partial("x"),
                      x + 3 * y * x * x + z * y * x / 4);
    BOOST_CHECK_EQUAL((x + 3 * y * x * x + z * y * x / 4).integrate("y").partial("y"),
                      x + 3 * y * x * x + z * y * x / 4);
    BOOST_CHECK_EQUAL((x + 3 * y * x * x + z * y * x / 4).integrate("z").partial("z"),
                      x + 3 * y * x * x + z * y * x / 4);
    BOOST_CHECK_EQUAL(p_type1{4}.integrate("z"), 4 * z);
    BOOST_CHECK_EQUAL((x * y * z).pow(-5).integrate("x"), (y * z).pow(-5) * x.pow(-4) * rational(1, -4));
    // Polynomial with polynomial coefficient, no variable mixing.
    typedef polynomial<p_type1, monomial<short>> p_type11;
    BOOST_CHECK(is_integrable<p_type11>::value);
    BOOST_CHECK(is_integrable<p_type11 &>::value);
    BOOST_CHECK(is_integrable<const p_type11>::value);
    BOOST_CHECK(is_integrable<p_type11 const &>::value);
    p_type11 a("a"), b("b"), c("c");
    BOOST_CHECK_EQUAL((a * x).integrate("x"), a * x * x / 2);
    BOOST_CHECK_EQUAL((a * x).integrate("a"), a * a * x / 2);
    BOOST_CHECK_EQUAL((a * x * x + b * x / 15 - c * x * y).integrate("x"),
                      a * x * x * x / 3 + b * x * x / 30 - c * x * x * y / 2);
    BOOST_CHECK_EQUAL((a * ((x * x).pow(-1)) + b * x / 15 - a * y).integrate("x"),
                      -a * (x).pow(-1) + b * x * x / 30 - a * x * y);
    BOOST_CHECK_THROW((a * (x).pow(-1) + b * x / 15 - a * y).integrate("x"), std::invalid_argument);
    BOOST_CHECK_EQUAL((a * x * x + b * x / 15 - a * y).integrate("a"),
                      a * a * x * x / 2 + a * b * x / 15 - a * a * y / 2);
    BOOST_CHECK_EQUAL(math::integrate(a * x * x + b * x / 15 - a * y, "a"),
                      a * a * x * x / 2 + a * b * x / 15 - a * a * y / 2);
    BOOST_CHECK_EQUAL((7 * x * a.pow(-2) + b * x / 15 - a * y).integrate("a"),
                      -7 * x * a.pow(-1) + a * b * x / 15 - a * a * y / 2);
    BOOST_CHECK_EQUAL((7 * x * a.pow(-2) - a * y + b * x / 15).integrate("a"),
                      -7 * x * a.pow(-1) + a * b * x / 15 - a * a * y / 2);
    BOOST_CHECK_EQUAL(math::integrate(x.pow(4) * y * a.pow(4) + x * y * b, "x"),
                      x.pow(5) * y * a.pow(4) / 5 + x * x / 2 * y * b);
    // Variable mixing (integration by parts).
    p_type11 xx("x"), yy("y"), zz("z");
    BOOST_CHECK_EQUAL((x * xx).integrate("x"), x * x * xx / 2 - math::integrate(x * x / 2, "x"));
    BOOST_CHECK_EQUAL(((3 * x + y) * xx).integrate("x"),
                      (3 * x * x + 2 * x * y) * xx / 2 - math::integrate((3 * x * x + 2 * x * y) / 2, "x"));
    BOOST_CHECK_EQUAL((x * xx * xx).integrate("x"),
                      x * x * xx * xx / 2 - 2 * xx * x * x * x / 6 + 2 * x * x * x * x / 24);
    BOOST_CHECK_EQUAL(math::partial((x * xx * xx).integrate("x"), "x"), x * xx * xx);
    BOOST_CHECK_THROW((x.pow(-1) * xx * xx).integrate("x"), std::invalid_argument);
    BOOST_CHECK_THROW((x.pow(-2) * xx * xx).integrate("x"), std::invalid_argument);
    BOOST_CHECK_THROW((x.pow(-3) * xx * xx).integrate("x"), std::invalid_argument);
    BOOST_CHECK_EQUAL((x.pow(-4) * xx * xx).integrate("x"),
                      -x.pow(-3) / 3 * xx * xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6);
    BOOST_CHECK_EQUAL((x.pow(-4) * xx).integrate("x"), -x.pow(-3) / 3 * xx - x.pow(-2) / 6);
    BOOST_CHECK_EQUAL((y * x.pow(-4) * xx * xx).integrate("x"),
                      y * (-x.pow(-3) / 3 * xx * xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6));
    BOOST_CHECK_EQUAL(((y + z.pow(2) * y) * x.pow(-4) * xx * xx).integrate("x"),
                      (y + z.pow(2) * y) * (-x.pow(-3) / 3 * xx * xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6));
    BOOST_CHECK_EQUAL(((y + z.pow(2) * y) * x.pow(-4) * xx * xx - x.pow(-4) * xx).integrate("x"),
                      (y + z.pow(2) * y) * (-x.pow(-3) / 3 * xx * xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6)
                          - (-x.pow(-3) / 3 * xx - x.pow(-2) / 6));
    // Misc tests.
    BOOST_CHECK_EQUAL(math::partial((x + y + z).pow(10).integrate("x"), "x"), (x + y + z).pow(10));
    BOOST_CHECK_EQUAL(math::partial((x + y + z).pow(10).integrate("y"), "y"), (x + y + z).pow(10));
    BOOST_CHECK_EQUAL(math::partial((x + y + z).pow(10).integrate("z"), "z"), (x + y + z).pow(10));
    BOOST_CHECK_THROW((x * xx.pow(-1)).integrate("x"), std::invalid_argument);
    BOOST_CHECK_EQUAL((x * xx.pow(-1)).integrate("y"), x * xx.pow(-1) * yy);
    BOOST_CHECK_THROW((x * yy.pow(-1)).integrate("y"), std::invalid_argument);
    BOOST_CHECK_EQUAL((x * yy.pow(-2)).integrate("y"), -x * yy.pow(-1));
    // Non-integrable coefficient.
    typedef polynomial<polynomial_alt<rational, int>, monomial<int>> p_type_alt;
    p_type_alt n("n"), m("m");
    BOOST_CHECK_EQUAL(math::integrate(n * m + m, "n"), n * n * m / 2 + m * n);
    BOOST_CHECK_EQUAL(math::integrate(n * m + m, "m"), m * n * m / 2 + m * m / 2);
    BOOST_CHECK_THROW(math::integrate(p_type_alt{polynomial_alt<rational, int>{"m"}}, "m"), std::invalid_argument);
    BOOST_CHECK_EQUAL(math::integrate(p_type_alt{polynomial_alt<rational, int>{"n"}}, "m"),
                      (polynomial_alt<rational, int>{"n"} * m));
    BOOST_CHECK_EQUAL(math::integrate(p_type_alt{polynomial_alt<rational, int>{"m"}}, "n"),
                      (polynomial_alt<rational, int>{"m"} * n));
    // Check with rational exponents and the new type-deduction logic.
    using p_type2 = polynomial<integer, monomial<rational>>;
    using p_type3 = polynomial<int, monomial<rational>>;
    BOOST_CHECK(is_integrable<p_type2>::value);
    BOOST_CHECK(is_integrable<p_type3>::value);
    BOOST_CHECK((std::is_same<decltype(p_type2{}.integrate("x")), polynomial<rational, monomial<rational>>>::value));
    BOOST_CHECK((std::is_same<decltype(p_type3{}.integrate("x")), polynomial<rational, monomial<rational>>>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::integrate(p_type2{}, "x")), polynomial<rational, monomial<rational>>>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::integrate(p_type3{}, "x")), polynomial<rational, monomial<rational>>>::value));
    BOOST_CHECK_EQUAL(math::integrate(p_type2{"x"}.pow(3 / 4_q), "x"), 4 / 7_q * p_type2{"x"}.pow(7 / 4_q));
    BOOST_CHECK_EQUAL(math::integrate(3 * p_type3{"x"}.pow(3 / 4_q), "x"), 12 / 7_q * p_type3{"x"}.pow(7 / 4_q));
}

BOOST_AUTO_TEST_CASE(polynomial_ipow_subs_test)
{
    typedef polynomial<rational, monomial<int>> p_type1;
    BOOST_CHECK((has_ipow_subs<p_type1, p_type1>::value));
    BOOST_CHECK((has_ipow_subs<p_type1, integer>::value));
    {
        BOOST_CHECK_EQUAL(p_type1{"x"}.ipow_subs("x", integer(4), integer(1)), p_type1{"x"});
        BOOST_CHECK_EQUAL(p_type1{"x"}.ipow_subs("x", integer(1), p_type1{"x"}), p_type1{"x"});
        p_type1 x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).ipow_subs("x", integer(2), integer(3)), 3 + x * y + z);
        BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).ipow_subs("y", integer(1), rational(3, 2)),
                          x * x + x * rational(3, 2) + z);
        BOOST_CHECK_EQUAL((x.pow(7) + x.pow(2) * y + z).ipow_subs("x", integer(3), x), x.pow(3) + x.pow(2) * y + z);
        BOOST_CHECK_EQUAL((x.pow(6) + x.pow(2) * y + z).ipow_subs("x", integer(3), p_type1{}), x.pow(2) * y + z);
        BOOST_CHECK_EQUAL(
            (1 + 3 * x.pow(2) - 5 * y.pow(5)).pow(10).ipow_subs("x", integer(2), p_type1{"x2"}).subs("x2", x.pow(2)),
            (1 + 3 * x.pow(2) - 5 * y.pow(5)).pow(10));
        // Check with negative powers.
        BOOST_CHECK_EQUAL(x.pow(-5).ipow_subs("x", -2, 5), x.pow(-1) * 25);
        BOOST_CHECK_EQUAL(x.pow(-5).ipow_subs("y", -2, 5), x.pow(-5));
        BOOST_CHECK_EQUAL((x.pow(-5) * y * z).ipow_subs("x", -4, 5), x.pow(-1) * 5 * z * y);
    }
    {
        typedef polynomial<real, monomial<int>> p_type2;
        BOOST_CHECK((has_ipow_subs<p_type2, p_type2>::value));
        BOOST_CHECK((has_ipow_subs<p_type2, integer>::value));
        p_type2 x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL((x * x * x + y * y).ipow_subs("x", integer(1), real(1.234)),
                          y * y + math::pow(real(1.234), 3));
        BOOST_CHECK_EQUAL((x * x * x + y * y).ipow_subs("x", integer(3), real(1.234)), y * y + real(1.234));
        BOOST_CHECK_EQUAL(
            (x * x * x + y * y).ipow_subs("x", integer(2), real(1.234)).ipow_subs("y", integer(2), real(-5.678)),
            real(-5.678) + real(1.234) * x);
        BOOST_CHECK_EQUAL(
            math::ipow_subs(x * x * x + y * y, "x", integer(1), real(1.234)).ipow_subs("y", integer(1), real(-5.678)),
            math::pow(real(-5.678), 2) + math::pow(real(1.234), 3));
    }
    {
        typedef polynomial<integer, monomial<long>> p_type3;
        BOOST_CHECK((has_ipow_subs<p_type3, p_type3>::value));
        BOOST_CHECK((has_ipow_subs<p_type3, integer>::value));
        p_type3 x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z, "x", integer(2), y), x.pow(-7) + y + z);
        BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z, "x", integer(-2), y), x.pow(-1) * y.pow(3) + y + z);
        BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z, "x", integer(-7), z), y + 2 * z);
    }
    {
        // Some tests with rational exponents.
        typedef polynomial<rational, monomial<rational>> p_type4;
        BOOST_CHECK((has_ipow_subs<p_type4, p_type4>::value));
        BOOST_CHECK((has_ipow_subs<p_type4, integer>::value));
        p_type4 x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL(x * y * 2 * z.pow(7 / 3_q).ipow_subs("z", 2, 4), 4 * z.pow(1 / 3_q) * y * 2 * x);
        BOOST_CHECK_EQUAL(x * y * 2 * z.pow(-7 / 3_q).ipow_subs("z", -1, 4), 16 * z.pow(-1 / 3_q) * y * 2 * x);
    }
}

BOOST_AUTO_TEST_CASE(polynomial_serialization_test)
{
    typedef polynomial<integer, monomial<long>> stype;
    stype x("x"), y("y"), z = x + y, tmp;
    std::stringstream ss;
    {
        boost::archive::text_oarchive oa(ss);
        oa << z;
    }
    {
        boost::archive::text_iarchive ia(ss);
        ia >> tmp;
    }
    BOOST_CHECK_EQUAL(z, tmp);
}

BOOST_AUTO_TEST_CASE(polynomial_rebind_test)
{
    typedef polynomial<integer, monomial<long>> stype;
    BOOST_CHECK((series_is_rebindable<stype, double>::value));
    BOOST_CHECK((series_is_rebindable<stype, rational>::value));
    BOOST_CHECK((series_is_rebindable<stype, float>::value));
    BOOST_CHECK((std::is_same<series_rebind<stype, float>, polynomial<float, monomial<long>>>::value));
    BOOST_CHECK((std::is_same<series_rebind<stype, rational>, polynomial<rational, monomial<long>>>::value));
    BOOST_CHECK((std::is_same<series_rebind<stype, long double>, polynomial<long double, monomial<long>>>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_invert_test)
{
    using pt0 = polynomial<integer, monomial<long>>;
    BOOST_CHECK(is_invertible<pt0>::value);
    BOOST_CHECK((std::is_same<pt0, decltype(math::invert(pt0{}))>::value));
    BOOST_CHECK_EQUAL(math::invert(pt0{1}), 1);
    BOOST_CHECK_EQUAL(math::invert(pt0{2}), 0);
    BOOST_CHECK_THROW(math::invert(pt0{0}), zero_division_error);
    BOOST_CHECK_EQUAL(math::invert(pt0{"x"}), math::pow(pt0{"x"}, -1));
    using pt1 = polynomial<rational, monomial<long>>;
    BOOST_CHECK(is_invertible<pt1>::value);
    BOOST_CHECK((std::is_same<pt1, decltype(math::invert(pt1{}))>::value));
    BOOST_CHECK_EQUAL(math::invert(pt1{1}), 1);
    BOOST_CHECK_EQUAL(math::invert(pt1{2}), 1 / 2_q);
    BOOST_CHECK_EQUAL(math::invert(2 * pt1{"y"}), 1 / 2_q * pt1{"y"}.pow(-1));
    BOOST_CHECK_THROW(math::invert(pt1{0}), zero_division_error);
    BOOST_CHECK_THROW(math::invert(pt1{"x"} + pt1{"y"}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(polynomial_find_cf_test)
{
    using pt1 = polynomial<integer, k_monomial>;
    BOOST_CHECK_EQUAL(pt1{}.find_cf<int>({}), 0);
    BOOST_CHECK_THROW(pt1{}.find_cf({1}), std::invalid_argument);
    BOOST_CHECK_EQUAL(3 * pt1{"x"}.find_cf({1}), 3);
    BOOST_CHECK_EQUAL(3 * pt1{"x"}.find_cf({0}), 0);
    BOOST_CHECK_EQUAL(3 * pt1{"x"}.find_cf({2}), 0);
    BOOST_CHECK_THROW((3 * pt1{"x"} + 4 * pt1{"y"}).find_cf({2}), std::invalid_argument);
    BOOST_CHECK_EQUAL((3 * pt1{"x"} + 4 * pt1{"y"}).find_cf({1, 0}), 3);
    BOOST_CHECK_EQUAL((3 * pt1{"x"} + 4 * pt1{"y"}).find_cf({0, 1}), 4);
    BOOST_CHECK_EQUAL((3 * pt1{"x"} + 4 * pt1{"y"}).find_cf({1_z, 1_z}), 0);
    BOOST_CHECK_EQUAL((3 * pt1{"x"} + 4 * pt1{"y"}).find_cf(std::vector<integer>{1_z, 1_z}), 0);
    BOOST_CHECK_EQUAL((3 * pt1{"x"} + 4 * pt1{"y"}).find_cf(std::list<int>{0, 1}), 4);
    using pt2 = polynomial<integer, monomial<int>>;
    BOOST_CHECK_EQUAL(pt2{}.find_cf<int>({}), 0);
    BOOST_CHECK_THROW(pt2{}.find_cf({1}), std::invalid_argument);
    BOOST_CHECK_EQUAL(3 * pt2{"x"}.find_cf({1}), 3);
    BOOST_CHECK_EQUAL(3 * pt2{"x"}.find_cf({0}), 0);
    BOOST_CHECK_EQUAL(3 * pt2{"x"}.find_cf({2}), 0);
    BOOST_CHECK_THROW((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf({2}), std::invalid_argument);
    BOOST_CHECK_EQUAL((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf({1, 0}), 3);
    BOOST_CHECK_EQUAL((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf({0, 1}), 4);
    BOOST_CHECK_EQUAL((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf({1_z, 1_z}), 0);
    BOOST_CHECK_EQUAL((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf(std::vector<integer>{1_z, 1_z}), 0);
    BOOST_CHECK_EQUAL((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf(std::list<int>{0, 1}), 4);
    BOOST_CHECK_EQUAL((3 * pt2{"x"} + 4 * pt2{"y"}).find_cf(std::list<signed char>{0, 1}), 4);
    if (std::numeric_limits<long>::max() > std::numeric_limits<int>::max()) {
        BOOST_CHECK_THROW(pt2{"x"}.find_cf(std::list<long>{std::numeric_limits<long>::max()}), std::invalid_argument);
    }
}
