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

#define BOOST_TEST_MODULE polynomial_04_test
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

#include <mp++/config.hpp>
#include <mp++/rational.hpp>

#include <piranha/base_series_multiplier.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/invert.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/monomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/series.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/settings.hpp>
#include <piranha/symbol_utils.hpp>

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
        this->m_symbol_set = symbol_fset{name};
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
    using call_enabler = typename std::enable_if<
        key_is_multipliable<typename T::term_type::cf_type, typename T::term_type::key_type>::value, int>::type;

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
    template <typename Cf, typename std::enable_if<mppp::is_rational<Cf>::value, int>::type = 0>
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
    template <typename Cf, typename std::enable_if<!mppp::is_rational<Cf>::value, int>::type = 0>
    void operator()(const Cf &)
    {
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_test)
{
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
        BOOST_CHECK_EQUAL(p_type1{"x"}.template subs<integer>({{"x", integer(1)}}), 1);
        BOOST_CHECK_EQUAL(p_type1{"x"}.template subs<p_type1>({{"x", p_type1{"x"}}}), p_type1{"x"});
        p_type1 x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL((math::pow(x, 2) + x * y + z).template subs<integer>({{"x", integer(3)}}), 9 + 3 * y + z);
        BOOST_CHECK_EQUAL((math::pow(x, 2) + x * y + z).template subs<rational>({{"y", rational(3, 2)}}),
                          x * x + x * rational(3, 2) + z);
        BOOST_CHECK_EQUAL((math::pow(x, 2) + x * y + z).template subs<rational>({{"k", rational(3, 2)}}),
                          x * x + x * y + z);
        BOOST_CHECK_EQUAL(math::pow(x, -1).template subs<p_type1>({{"x", math::pow(x, -1)}}), x);
        BOOST_CHECK_EQUAL(
            (math::pow(x, 2) + x * y + z)
                .template subs<rational>({{"x", rational(3, 2)}, {"y", rational(4, 5)}, {"z", -rational(6, 7)}}),
            math::evaluate<rational>(math::pow(x, 2) + x * y + z,
                                     {{"x", rational(3, 2)}, {"y", rational(4, 5)}, {"z", -rational(6, 7)}}));
        BOOST_CHECK_EQUAL(
            math::subs<rational>(math::pow(x, 2) + x * y + z,
                                 {{"x", rational(3, 2)}, {"y", rational(4, 5)}, {"z", -rational(6, 7)}}),
            math::evaluate<rational>(math::pow(x, 2) + x * y + z,
                                     {{"x", rational(3, 2)}, {"y", rational(4, 5)}, {"z", -rational(6, 7)}}));
        BOOST_CHECK((std::is_same<decltype(p_type1{"x"}.template subs<integer>({{"x", integer(1)}})), p_type1>::value));
        BOOST_CHECK(
            (std::is_same<decltype(p_type1{"x"}.template subs<rational>({{"x", rational(1)}})), p_type1>::value));
        BOOST_CHECK_EQUAL((math::pow(x, 2) + x * y + z).template subs<rational>({{"k", rational(3, 2)}}),
                          x * x + x * y + z);
        BOOST_CHECK_EQUAL((math::pow(y + 4 * z, 5) * math::pow(x, -1)).template subs<rational>({{"x", rational(3)}}),
                          (math::pow(y + 4 * z, 5)) / 3);
    }
#if defined(MPPP_WITH_MPFR)
    {
        typedef polynomial<real, monomial<int>> p_type2;
        BOOST_CHECK((has_subs<p_type2, rational>::value));
        BOOST_CHECK((has_subs<p_type2, double>::value));
        BOOST_CHECK((has_subs<p_type2, integer>::value));
        BOOST_CHECK((!has_subs<p_type2, std::string>::value));
        p_type2 x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL((x * x * x + y * y).template subs<real>({{"x", real(1.234)}}),
                          y * y + math::pow(real(1.234), 3));
        BOOST_CHECK_EQUAL((x * x * x + y * y).template subs<real>({{"x", real(1.234)}, {"y", real(-5.678)}}),
                          math::pow(real(-5.678), 2) + math::pow(real(1.234), 3));
        BOOST_CHECK_EQUAL(math::subs<real>(x * x * x + y * y, {{"x", real(1.234)}, {"y", real(-5.678)}}),
                          math::pow(real(-5.678), 2) + math::pow(real(1.234), 3));
    }
#endif
    typedef polynomial<integer, monomial<long>> p_type3;
    BOOST_CHECK((has_subs<p_type3, rational>::value));
    BOOST_CHECK((has_subs<p_type3, double>::value));
    BOOST_CHECK((has_subs<p_type3, integer>::value));
    BOOST_CHECK((!has_subs<p_type3, std::string>::value));
    p_type3 x{"x"}, y{"y"}, z{"z"};
    BOOST_CHECK_EQUAL(
        (x * x * x + y * y + z * y * x)
            .template subs<integer>({{"x", integer(2)}, {"y", integer(-3)}, {"z", integer(4)}, {"k", integer()}}),
        math::pow(integer(2), 3) + math::pow(integer(-3), 2) + integer(2) * integer(-3) * integer(4));
    BOOST_CHECK_EQUAL(math::subs<integer>(x * x * x + y * y + z * y * x,
                                          {{"x", integer(2)}, {"y", integer(-3)}, {"z", integer(4)}, {"k", integer()}}),
                      math::pow(integer(2), 3) + math::pow(integer(-3), 2) + integer(2) * integer(-3) * integer(4));
    BOOST_CHECK_EQUAL(
        (x * x * x + y * y + z * y * x)
            .template subs<integer>({{"x", integer(0)}, {"y", integer(0)}, {"z", integer(0)}, {"k", integer()}}),
        0);
}
