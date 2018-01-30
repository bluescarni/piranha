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

#include <piranha/rational.hpp>

#define BOOST_TEST_MODULE rational_01_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <mp++/config.hpp>
#include <mp++/exceptions.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <piranha/config.hpp>
#include <piranha/integer.hpp>
#include <piranha/math.hpp>
#include <piranha/math/binomial.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/print_tex_coefficient.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

BOOST_AUTO_TEST_CASE(rational_literal_test)
{
    auto q0 = 123_q;
    BOOST_CHECK((std::is_same<rational, decltype(q0)>::value));
    BOOST_CHECK_EQUAL(q0, 123);
    q0 = -4_q;
    BOOST_CHECK_EQUAL(q0, -4);
    BOOST_CHECK_THROW((q0 = 123.45_q), std::invalid_argument);
    auto q1 = 3 / 4_q;
    BOOST_CHECK((std::is_same<rational, decltype(q1)>::value));
    BOOST_CHECK_EQUAL(q1, (rational{3, 4}));
    q1 = -4 / 2_q;
    BOOST_CHECK_EQUAL(q1, -2);
    BOOST_CHECK_THROW((q1 = -3 / 0_q), mppp::zero_division_error);
    BOOST_CHECK_EQUAL(q1, -2);
}

struct is_zero_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        q_type q;
        BOOST_CHECK(math::is_zero(q));
        q = 1;
        BOOST_CHECK(!math::is_zero(q));
        q = "-3/5";
        BOOST_CHECK(!math::is_zero(q));
        q = "1";
        q -= 1;
        BOOST_CHECK(math::is_zero(q));
    }
};

BOOST_AUTO_TEST_CASE(rational_is_zero_test)
{
    tuple_for_each(size_types{}, is_zero_tester());
}

struct pow_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        using int_type = typename q_type::int_t;
        // A few simple tests.
        BOOST_CHECK_EQUAL(math::pow(q_type(), 0), 1);
        BOOST_CHECK_EQUAL(math::pow(q_type(), 0u), 1);
        BOOST_CHECK_EQUAL(math::pow(q_type(), int_type()), 1);
        BOOST_CHECK_EQUAL(math::pow(q_type(), 1), 0);
        BOOST_CHECK_EQUAL(math::pow(q_type(), 2u), 0);
        BOOST_CHECK_EQUAL(math::pow(q_type(), 3), 0);
        BOOST_CHECK_EQUAL(math::pow(q_type(), 4ull), 0);
        BOOST_CHECK_EQUAL(math::pow(q_type(), int_type(5)), 0);
        BOOST_CHECK_EQUAL(math::pow(q_type(), static_cast<unsigned char>(5)), 0);
        BOOST_CHECK_THROW(math::pow(q_type(), -1), mppp::zero_division_error);
        BOOST_CHECK_THROW(math::pow(q_type(), char(-2)), mppp::zero_division_error);
        BOOST_CHECK_THROW(math::pow(q_type(), -3ll), mppp::zero_division_error);
        BOOST_CHECK_THROW(math::pow(q_type(), int_type(-3)), mppp::zero_division_error);
        BOOST_CHECK_EQUAL(math::pow(q_type(23, 45), 7), q_type(3404825447ull, 373669453125ull));
        BOOST_CHECK_EQUAL(math::pow(q_type(-23, 45), 7), q_type(-3404825447ll, 373669453125ull));
        BOOST_CHECK_EQUAL(math::pow(q_type(-23, 45), -7), q_type(373669453125ull, -3404825447ll));
        // Rational-fp.
        const auto radix = std::numeric_limits<double>::radix;
        BOOST_CHECK((is_exponentiable<q_type, float>::value));
        BOOST_CHECK((is_exponentiable<q_type, double>::value));
        BOOST_CHECK_EQUAL(math::pow(q_type(1, radix), 2.), std::pow(1. / radix, 2.));
        BOOST_CHECK((std::is_same<float, decltype(math::pow(q_type(1), 1.f))>::value));
#if defined(MPPP_WITH_MPFR)
        BOOST_CHECK((std::is_same<long double, decltype(math::pow(q_type(1, radix), 2.l))>::value));
        BOOST_CHECK((std::is_same<long double, decltype(math::pow(2.l, q_type(1, radix)))>::value));
#endif
        // Rational-rational.
        BOOST_CHECK((is_exponentiable<q_type, q_type>::value));
        BOOST_CHECK_THROW(math::pow(q_type(1, radix), q_type(1, radix)), std::domain_error);
        BOOST_CHECK_EQUAL(math::pow(q_type(2, 3), q_type(2)), q_type(4, 9));
        BOOST_CHECK_EQUAL(math::pow(q_type(2, 3), q_type(-2)), q_type(9, 4));
        BOOST_CHECK((std::is_same<q_type, decltype(math::pow(q_type(1, radix), q_type(1, radix)))>::value));
        // Special cases.
        BOOST_CHECK_EQUAL(math::pow(q_type(1), q_type(2, 3)), 1);
        BOOST_CHECK_EQUAL(math::pow(q_type(1), q_type(-2, 3)), 1);
        BOOST_CHECK_EQUAL(math::pow(q_type(0), q_type(2, 3)), 0);
        BOOST_CHECK_EQUAL(math::pow(q_type(0), q_type(0)), 1);
        BOOST_CHECK_THROW(math::pow(q_type(0), q_type(-2, 3)), mppp::zero_division_error);
        // Fp-rational.
        BOOST_CHECK((is_exponentiable<float, q_type>::value));
        BOOST_CHECK((is_exponentiable<double, q_type>::value));
        BOOST_CHECK((std::is_same<decltype(math::pow(2., q_type(1, radix))), double>::value));
        BOOST_CHECK((std::is_same<decltype(math::pow(2.f, q_type(1, radix))), float>::value));
        BOOST_CHECK_EQUAL(math::pow(2., q_type(1, radix)), std::pow(2., 1. / radix));
        // Integral-rational.
        BOOST_CHECK((is_exponentiable<int, q_type>::value));
        BOOST_CHECK((is_exponentiable<int_type, q_type>::value));
        BOOST_CHECK((std::is_same<q_type, decltype(math::pow(2, q_type(1, radix)))>::value));
        BOOST_CHECK((std::is_same<q_type, decltype(math::pow(int_type(2), q_type(1, radix)))>::value));
        BOOST_CHECK_THROW(math::pow(2, q_type(1, radix)), std::domain_error);
        BOOST_CHECK_THROW(math::pow(int_type(2), q_type(1, radix)), std::domain_error);
        BOOST_CHECK_EQUAL(math::pow(2, q_type(2)), 4);
        BOOST_CHECK_EQUAL(math::pow(int_type(3), q_type(2)), 9);
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK((is_exponentiable<__int128_t, q_type>::value));
        BOOST_CHECK((is_exponentiable<__uint128_t, q_type>::value));
        BOOST_CHECK((is_exponentiable<q_type, __int128_t>::value));
        BOOST_CHECK((is_exponentiable<q_type, __uint128_t>::value));
        BOOST_CHECK_EQUAL(math::pow(__int128_t(2), q_type(2)), 4);
        BOOST_CHECK_EQUAL(math::pow(__uint128_t(2), q_type(2)), 4);
        BOOST_CHECK_EQUAL(math::pow(q_type(2), __int128_t(2)), 4);
        BOOST_CHECK_EQUAL(math::pow(q_type(2), __uint128_t(2)), 4);
#endif
        // Special cases.
        BOOST_CHECK_EQUAL(math::pow(1, q_type(2, 3)), 1);
        BOOST_CHECK_EQUAL(math::pow(int_type(1), q_type(2, 3)), 1);
        BOOST_CHECK_EQUAL(math::pow(1, q_type(2, -3)), 1);
        BOOST_CHECK_EQUAL(math::pow(int_type(1), q_type(-2, 3)), 1);
        BOOST_CHECK_EQUAL(math::pow(0, q_type(2, 3)), 0);
        BOOST_CHECK_EQUAL(math::pow(int_type(0), q_type(2, 3)), 0);
        BOOST_CHECK_EQUAL(math::pow(0, q_type(0)), 1);
        BOOST_CHECK_EQUAL(math::pow(int_type(0), q_type(0, 3)), 1);
        BOOST_CHECK_THROW(math::pow(0, q_type(-1, 3)), mppp::zero_division_error);
        BOOST_CHECK_THROW(math::pow(int_type(0), q_type(-1, 3)), mppp::zero_division_error);
    }
};

BOOST_AUTO_TEST_CASE(rational_pow_test)
{
    tuple_for_each(size_types{}, pow_tester());
    BOOST_CHECK((!is_exponentiable<mppp::rational<1>, mppp::rational<2>>::value));
    BOOST_CHECK((!is_exponentiable<mppp::rational<1>, mppp::integer<2>>::value));
    BOOST_CHECK((!is_exponentiable<mppp::integer<2>, mppp::rational<1>>::value));
    BOOST_CHECK((!is_exponentiable<mppp::integer<2>, std::string>::value));
    BOOST_CHECK((!is_exponentiable<mppp::integer<2>, void>::value));
}

struct abs_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        BOOST_CHECK_EQUAL(math::abs(q_type(-4, 5)), q_type(4, 5));
        BOOST_CHECK_EQUAL(math::abs(q_type(4, -5)), q_type(4, 5));
        BOOST_CHECK_EQUAL(math::abs(q_type(-4, -5)), q_type(4, 5));
    }
};

BOOST_AUTO_TEST_CASE(rational_abs_test)
{
    tuple_for_each(size_types{}, abs_tester());
}

struct print_tex_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        BOOST_CHECK(has_print_tex_coefficient<q_type>::value);
        std::ostringstream ss;
        print_tex_coefficient(ss, q_type(0));
        BOOST_CHECK_EQUAL(ss.str(), "0");
        ss.str("");
        print_tex_coefficient(ss, q_type(-1));
        BOOST_CHECK_EQUAL(ss.str(), "-1");
        ss.str("");
        print_tex_coefficient(ss, q_type(1));
        BOOST_CHECK_EQUAL(ss.str(), "1");
        ss.str("");
        print_tex_coefficient(ss, q_type(1, 2));
        BOOST_CHECK_EQUAL(ss.str(), "\\frac{1}{2}");
        ss.str("");
        print_tex_coefficient(ss, q_type(1, -2));
        BOOST_CHECK_EQUAL(ss.str(), "-\\frac{1}{2}");
        ss.str("");
        print_tex_coefficient(ss, q_type(-14, 21));
        BOOST_CHECK_EQUAL(ss.str(), "-\\frac{2}{3}");
    }
};

BOOST_AUTO_TEST_CASE(rational_print_tex_test)
{
    tuple_for_each(size_types{}, print_tex_tester());
}

struct sin_cos_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        BOOST_CHECK_EQUAL(math::sin(q_type()), 0);
        BOOST_CHECK_EQUAL(math::cos(q_type()), 1);
        BOOST_CHECK((std::is_same<q_type, decltype(math::cos(q_type()))>::value));
        BOOST_CHECK((std::is_same<q_type, decltype(math::sin(q_type()))>::value));
        BOOST_CHECK_EXCEPTION(math::sin(q_type(1)), std::domain_error, [](const std::domain_error &e) {
            return boost::contains(e.what(), "cannot compute the sine of the non-zero rational 1");
        });
        BOOST_CHECK_EXCEPTION(math::cos(q_type(1)), std::domain_error, [](const std::domain_error &e) {
            return boost::contains(e.what(), "cannot compute the cosine of the non-zero rational 1");
        });
        BOOST_CHECK(is_sine_type<q_type>::value);
        BOOST_CHECK(is_cosine_type<q_type>::value);
    }
};

BOOST_AUTO_TEST_CASE(rational_sin_cos_test)
{
    tuple_for_each(size_types{}, sin_cos_tester());
}

struct sep_tester {
    template <typename T>
    using edict = symbol_fmap<T>;
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        BOOST_CHECK_EQUAL(math::partial(q_type{1}, ""), 0);
        BOOST_CHECK((std::is_same<q_type, decltype(math::partial(q_type{1}, ""))>::value));
        BOOST_CHECK(is_differentiable<q_type>::value);
        BOOST_CHECK_EQUAL(math::evaluate(q_type{12}, edict<int>{{"", 1}}), 12);
        BOOST_CHECK_EQUAL(math::evaluate(q_type{10}, edict<double>{{"", 1.321}}), 10);
        BOOST_CHECK((is_evaluable<q_type, int>::value));
        BOOST_CHECK((is_evaluable<q_type, double>::value));
        // NOTE: always evaluable with long double, but the return type will change.
        BOOST_CHECK((is_evaluable<q_type, long double>::value));
#if defined(MPPP_WITH_MPFR)
        BOOST_CHECK(
            (std::is_same<long double, decltype(math::evaluate(q_type{10}, edict<long double>{{"", 1.321l}}))>::value));
#else
        BOOST_CHECK(
            (std::is_same<q_type, decltype(math::evaluate(q_type{10}, edict<long double>{{"", 1.321l}}))>::value));
#endif
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK(
            (std::is_same<q_type, decltype(math::evaluate(q_type{10}, edict<__int128_t>{{"", __int128_t()}}))>::value));
        BOOST_CHECK((std::is_same<q_type, decltype(math::evaluate(q_type{10},
                                                                  edict<__uint128_t>{{"", __uint128_t()}}))>::value));
#endif
        BOOST_CHECK((std::is_same<double, decltype(math::evaluate(q_type{10}, edict<double>{{"", 1.321}}))>::value));
        BOOST_CHECK((!has_subs<q_type, q_type>::value));
        BOOST_CHECK((!has_subs<q_type, int>::value));
        BOOST_CHECK((!has_subs<q_type, long double>::value));
        BOOST_CHECK((!has_ipow_subs<q_type, int>::value));
        BOOST_CHECK((!has_ipow_subs<q_type, double>::value));
        BOOST_CHECK((!has_ipow_subs<q_type, float>::value));
        BOOST_CHECK((!has_ipow_subs<q_type, unsigned short>::value));
    }
};

BOOST_AUTO_TEST_CASE(rational_sep_test)
{
    tuple_for_each(size_types{}, sep_tester());
}

struct stream_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        q_type q(42, -5);
        std::stringstream ss;
        ss << q;
        BOOST_CHECK_EQUAL(ss.str(), "-42/5");
        q = 0;
        ss >> q;
        BOOST_CHECK_EQUAL(q, q_type(-42, 5));
    }
};

BOOST_AUTO_TEST_CASE(rational_stream_test)
{
    tuple_for_each(size_types{}, stream_tester());
}

struct safe_cast_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        using z_type = typename q_type::int_t;
        // From q conversions.
        BOOST_CHECK((has_safe_cast<int, q_type>::value));
        BOOST_CHECK((has_safe_cast<int &, const q_type>::value));
        BOOST_CHECK((has_safe_cast<int &&, const q_type &>::value));
        BOOST_CHECK((!has_safe_cast<void, const q_type &>::value));
        BOOST_CHECK((has_safe_cast<unsigned, q_type>::value));
        BOOST_CHECK((has_safe_cast<unsigned &, const q_type>::value));
        BOOST_CHECK((has_safe_cast<unsigned &&, const q_type &>::value));
        BOOST_CHECK((has_safe_cast<const unsigned &, const q_type &>::value));
        BOOST_CHECK((has_safe_cast<z_type, q_type>::value));
        BOOST_CHECK_EQUAL(safe_cast<int>(q_type{0}), 0);
        BOOST_CHECK_EQUAL(safe_cast<int>(q_type{-4}), -4);
        BOOST_CHECK_EQUAL(safe_cast<unsigned>(q_type{0}), 0u);
        BOOST_CHECK_EQUAL(safe_cast<unsigned>(q_type{42}), 42u);
        BOOST_CHECK_EQUAL(safe_cast<z_type>(q_type{0} / 2), 0);
        BOOST_CHECK_EQUAL(safe_cast<z_type>(q_type{-42} / 2), -21);
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK((has_safe_cast<__int128_t, q_type>::value));
        BOOST_CHECK((has_safe_cast<__uint128_t, q_type>::value));
        BOOST_CHECK((has_safe_cast<q_type, __int128_t>::value));
        BOOST_CHECK((has_safe_cast<q_type, __uint128_t>::value));
        BOOST_CHECK(safe_cast<__int128_t>(q_type{42}) == 42);
        BOOST_CHECK(safe_cast<__uint128_t>(q_type{42}) == 42u);
        BOOST_CHECK(safe_cast<q_type>(q_type{__int128_t(42)} == 42));
        BOOST_CHECK(safe_cast<q_type>(q_type{__uint128_t(42)} == 42u));
#endif
        // Various types of failures.
        BOOST_CHECK_EXCEPTION(safe_cast<int>(q_type{std::numeric_limits<int>::max()} + 1), safe_cast_failure,
                              [](const safe_cast_failure &e) {
                                  return boost::contains(e.what(), "as the conversion would result in overflow");
                              });
        BOOST_CHECK_EXCEPTION(safe_cast<int>(q_type{std::numeric_limits<int>::min()} - 1), safe_cast_failure,
                              [](const safe_cast_failure &e) {
                                  return boost::contains(e.what(), "as the conversion would result in overflow");
                              });
        BOOST_CHECK_EXCEPTION(safe_cast<int>(q_type{-4} / 3), safe_cast_failure, [](const safe_cast_failure &e) {
            return boost::contains(e.what(), "as the rational value has a non-unitary denominator");
        });
        BOOST_CHECK_EXCEPTION(safe_cast<z_type>(q_type{-4} / 3), safe_cast_failure, [](const safe_cast_failure &e) {
            return boost::contains(e.what(), "as the rational value has a non-unitary denominator");
        });
        BOOST_CHECK_THROW(safe_cast<unsigned>(q_type{-4}), safe_cast_failure);
        BOOST_CHECK_THROW(safe_cast<unsigned>(q_type{4} / 3), safe_cast_failure);
        BOOST_CHECK_THROW(safe_cast<z_type>(q_type{4} / 3), safe_cast_failure);
        // To q conversions.
        BOOST_CHECK((has_safe_cast<q_type, int>::value));
        BOOST_CHECK((has_safe_cast<const q_type, int &>::value));
        BOOST_CHECK((has_safe_cast<const q_type &, int &&>::value));
        BOOST_CHECK((has_safe_cast<const q_type &, const int &>::value));
        BOOST_CHECK((!has_safe_cast<q_type, void>::value));
        BOOST_CHECK((has_safe_cast<q_type, unsigned>::value));
        BOOST_CHECK((has_safe_cast<q_type, z_type>::value));
        BOOST_CHECK((has_safe_cast<q_type, double>::value));
        BOOST_CHECK_EQUAL(safe_cast<q_type>(-4), q_type{-4});
        BOOST_CHECK_EQUAL(safe_cast<q_type>(0), q_type{0});
        BOOST_CHECK_EQUAL(safe_cast<q_type>(4u), q_type{4});
        BOOST_CHECK_EQUAL(safe_cast<q_type>(0u), q_type{0u});
        BOOST_CHECK_EQUAL(safe_cast<q_type>(z_type{4}), q_type{4});
        BOOST_CHECK_EQUAL(safe_cast<q_type>(z_type{0}), q_type{0});
        // Floating point.
        static constexpr auto r = std::numeric_limits<double>::radix;
        BOOST_CHECK((has_safe_cast<q_type, double>::value));
        BOOST_CHECK((!has_safe_cast<double, q_type>::value));
        BOOST_CHECK_EQUAL(safe_cast<q_type>(1. / r), q_type(1, r));
        BOOST_CHECK_EQUAL(safe_cast<q_type>(-13. / (r * r * r)), q_type(-13, r * r * r));
#if defined(MPPP_WITH_MPFR)
        static constexpr auto rl = std::numeric_limits<long double>::radix;
        BOOST_CHECK((has_safe_cast<q_type, long double>::value));
        BOOST_CHECK_EQUAL(safe_cast<q_type>(1.l / rl), q_type(1, r));
#else
        BOOST_CHECK((!has_safe_cast<q_type, long double>::value));
#endif
        if (std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN) {
            BOOST_CHECK_EXCEPTION(safe_cast<q_type>(std::numeric_limits<double>::infinity()), safe_cast_failure,
                                  [](const safe_cast_failure &e) {
                                      return boost::contains(e.what(),
                                                             "cannot convert the non-finite floating-point value ");
                                  });
            BOOST_CHECK_EXCEPTION(safe_cast<q_type>(std::numeric_limits<double>::quiet_NaN()), safe_cast_failure,
                                  [](const safe_cast_failure &e) {
                                      return boost::contains(e.what(),
                                                             "cannot convert the non-finite floating-point value ");
                                  });
        }
    }
};

BOOST_AUTO_TEST_CASE(rational_safe_cast_test)
{
    tuple_for_each(size_types{}, safe_cast_tester{});
}

struct is_unitary_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        BOOST_CHECK(!math::is_unitary(q_type{}));
        BOOST_CHECK(!math::is_unitary(q_type{-1}));
        BOOST_CHECK(!math::is_unitary(q_type{-1, 5}));
        BOOST_CHECK(!math::is_unitary(q_type{1, 5}));
        BOOST_CHECK(!math::is_unitary(q_type{5, -5}));
        BOOST_CHECK(math::is_unitary(q_type{1}));
        BOOST_CHECK(math::is_unitary(q_type{-1, -1}));
        BOOST_CHECK(math::is_unitary(q_type{-5, -5}));
        BOOST_CHECK(math::is_unitary(q_type{5, 5}));
    }
};

BOOST_AUTO_TEST_CASE(rational_is_unitary_test)
{
    tuple_for_each(size_types{}, is_unitary_tester{});
}

struct negate_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using q_type = mppp::rational<T::value>;
        q_type q1;
        math::negate(q1);
        BOOST_CHECK_EQUAL(q1, 0);
        q1 = q_type{3, 4};
        math::negate(q1);
        BOOST_CHECK_EQUAL(q1, (q_type{3, -4}));
        math::negate(q1);
        BOOST_CHECK_EQUAL(q1, (q_type{3, 4}));
    }
};

BOOST_AUTO_TEST_CASE(rational_negate_test)
{
    tuple_for_each(size_types{}, negate_tester{});
}

struct rational_binomial_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using rat_type = mppp::rational<T::value>;
        using int_type = typename rat_type::int_t;
        BOOST_CHECK((are_binomial_types<rat_type, int_type>::value));
        BOOST_CHECK((are_binomial_types<rat_type, int_type &>::value));
        BOOST_CHECK((are_binomial_types<const rat_type, const int_type &>::value));
        BOOST_CHECK((are_binomial_types<rat_type, int>::value));
        BOOST_CHECK((are_binomial_types<rat_type, long long>::value));
        BOOST_CHECK((are_binomial_types<rat_type, unsigned long long>::value));
        BOOST_CHECK((!are_binomial_types<rat_type, void>::value));
        BOOST_CHECK((!are_binomial_types<int_type, rat_type>::value));
        BOOST_CHECK((!are_binomial_types<rat_type, double>::value));
        BOOST_CHECK((std::is_same<decltype(math::binomial(rat_type{7, 3}, 4)), rat_type>::value));
        BOOST_CHECK_EQUAL(math::binomial(rat_type{7, 3}, 4), (rat_type{-7, 243}));
        BOOST_CHECK_EQUAL(math::binomial(rat_type{7, -3}, int_type{4}), (rat_type{1820, 243}));
        BOOST_CHECK_EQUAL(math::binomial(rat_type{7, 3}, static_cast<signed char>(-4)), 0);
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK((are_binomial_types<rat_type, __int128_t>::value));
        BOOST_CHECK((are_binomial_types<rat_type, __uint128_t>::value));
        BOOST_CHECK((!are_binomial_types<__int128_t, rat_type>::value));
        BOOST_CHECK_EQUAL(math::binomial(rat_type{7, 3}, __int128_t(4)), (rat_type{-7, 243}));
        BOOST_CHECK_EQUAL(math::binomial(rat_type{7, 3}, __uint128_t(4)), (rat_type{-7, 243}));
#endif
    }
};

BOOST_AUTO_TEST_CASE(rational_binomial_test)
{
    tuple_for_each(size_types{}, rational_binomial_tester{});
}
