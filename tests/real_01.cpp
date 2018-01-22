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

#include <mp++/config.hpp>

#if defined(MPPP_WITH_MPFR)

#include <piranha/real.hpp>

#define BOOST_TEST_MODULE real_01_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <limits>
#include <random>
#include <sstream>
#include <string>

#include <piranha/integer.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/math.hpp>
#include <piranha/pow.hpp>
#include <piranha/rational.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/symbol_utils.hpp>

static std::mt19937 rng;
static const int ntries = 1000;

using namespace piranha;

static inline real operator"" _r(const char *s)
{
    return real(s, 100);
}

BOOST_AUTO_TEST_CASE(real_tt_test)
{
    BOOST_CHECK(is_cf<real>::value);
}

BOOST_AUTO_TEST_CASE(real_negate_test)
{
    BOOST_CHECK(has_negate<real>::value);
    real r1;
    BOOST_CHECK(!r1.signbit());
    math::negate(r1);
    BOOST_CHECK_EQUAL(r1, 0);
    BOOST_CHECK(r1.signbit());
    r1 = 123;
    math::negate(r1);
    BOOST_CHECK_EQUAL(r1, -123);
    math::negate(r1);
    BOOST_CHECK_EQUAL(r1, 123);
    r1 = real{"inf", 100};
    math::negate(r1);
    BOOST_CHECK_EQUAL(r1, -(real{"inf", 100}));
}

BOOST_AUTO_TEST_CASE(real_is_zero_test)
{
    BOOST_CHECK(has_is_zero<real>::value);
    real r1;
    BOOST_CHECK(math::is_zero(r1));
    r1.neg();
    BOOST_CHECK(math::is_zero(r1));
    r1 = 123;
    BOOST_CHECK(!math::is_zero(r1));
    r1 = real{"inf", 100};
    BOOST_CHECK(!math::is_zero(r1));
    r1 = -1;
    BOOST_CHECK(!math::is_zero(r1));
    r1 = real{"nan", 100};
    BOOST_CHECK(!math::is_zero(r1));
}

BOOST_AUTO_TEST_CASE(real_pow_test)
{
    BOOST_CHECK((is_exponentiable<real, real>::value));
    BOOST_CHECK((is_exponentiable<real, int>::value));
    BOOST_CHECK((is_exponentiable<int, real>::value));
    BOOST_CHECK((is_exponentiable<real, double>::value));
    BOOST_CHECK((is_exponentiable<double, real>::value));
    BOOST_CHECK((is_exponentiable<real, long double>::value));
    BOOST_CHECK((is_exponentiable<long double, real>::value));
    BOOST_CHECK((!is_exponentiable<void, real>::value));
    BOOST_CHECK((!is_exponentiable<real, void>::value));
    BOOST_CHECK((!is_exponentiable<std::string, real>::value));
    BOOST_CHECK((!is_exponentiable<real, std::string>::value));
#if defined(MPPP_HAVE_GCC_INT128)
    BOOST_CHECK((is_exponentiable<real, __int128_t>::value));
    BOOST_CHECK((is_exponentiable<__int128_t, real>::value));
    BOOST_CHECK((is_exponentiable<real, __uint128_t>::value));
    BOOST_CHECK((is_exponentiable<__uint128_t, real>::value));
#endif
    real r1{2}, r2{5};
    BOOST_CHECK_EQUAL(math::pow(r1, r2), 32);
    BOOST_CHECK_EQUAL(math::pow(r1, 5), 32);
    BOOST_CHECK_EQUAL(math::pow(2, r2), 32);
    BOOST_CHECK_EQUAL(math::pow(r1, 5.), 32);
    BOOST_CHECK_EQUAL(math::pow(2.l, r2), 32);
#if defined(MPPP_HAVE_GCC_INT128)
    BOOST_CHECK_EQUAL(math::pow(r1, __int128_t(5)), 32);
    BOOST_CHECK_EQUAL(math::pow(__uint128_t(2), r2), 32);
#endif
}

BOOST_AUTO_TEST_CASE(real_fma_test)
{
    real r0{1}, r1{4}, r2{-5};
    math::multiply_accumulate(r0, r1, r2);
    BOOST_CHECK_EQUAL(r0, -19);
    r0 = -5;
    r1 = -3;
    r2 = 6;
    math::multiply_accumulate(r0, r1, r2);
    BOOST_CHECK_EQUAL(r0, -23);
}

BOOST_AUTO_TEST_CASE(real_sin_cos_test)
{
    BOOST_CHECK_EQUAL(math::cos(real{0, 4}), 1);
    BOOST_CHECK_EQUAL(math::sin(real{0, 4}), 0);
}

BOOST_AUTO_TEST_CASE(real_partial_test)
{
    BOOST_CHECK_EQUAL(math::partial(real(), ""), 0);
    BOOST_CHECK_EQUAL(math::partial(real(1), std::string("")), 0);
    BOOST_CHECK_EQUAL(math::partial(real(-10), std::string("")), 0);
}

BOOST_AUTO_TEST_CASE(real_evaluate_test)
{
    BOOST_CHECK_EQUAL(math::evaluate(real(), symbol_fmap<integer>{}), real());
    BOOST_CHECK_EQUAL(math::evaluate(real(2), symbol_fmap<int>{}), real(2));
    BOOST_CHECK_EQUAL(math::evaluate(real(-3.5), symbol_fmap<double>{}), real(-3.5));
    BOOST_CHECK((std::is_same<decltype(math::evaluate(real(), symbol_fmap<real>{})), real>::value));
#if defined(MPPP_HAVE_GCC_INT128)
    BOOST_CHECK_EQUAL(math::evaluate(real(2), symbol_fmap<__int128_t>{}), real(2));
    BOOST_CHECK_EQUAL(math::evaluate(real(2), symbol_fmap<__uint128_t>{}), real(2));
#endif
}

BOOST_AUTO_TEST_CASE(real_abs_test)
{
    BOOST_CHECK_EQUAL(math::abs(real(42)), real(42));
    BOOST_CHECK_EQUAL(math::abs(real(-42)), real(42));
    BOOST_CHECK_EQUAL(math::abs(real("inf", 100)), real("inf", 100));
    BOOST_CHECK_EQUAL(math::abs(real("-inf", 100)), real("inf", 100));
    BOOST_CHECK(math::abs(real("-nan", 100)).nan_p());
}

BOOST_AUTO_TEST_CASE(real_safe_cast_test)
{
    BOOST_CHECK((has_safe_cast<int, real>::value));
    BOOST_CHECK((has_safe_cast<int &, real &&>::value));
    BOOST_CHECK((has_safe_cast<const int &, const real &>::value));
    BOOST_CHECK((has_safe_cast<const int, const real>::value));
    BOOST_CHECK((!has_safe_cast<void, real>::value));
    BOOST_CHECK((has_safe_cast<unsigned, real>::value));
    BOOST_CHECK((has_safe_cast<integer, real>::value));
    BOOST_CHECK((has_safe_cast<rational, real>::value));
    BOOST_CHECK((has_safe_cast<rational &, real>::value));
    BOOST_CHECK((has_safe_cast<rational &&, const real>::value));
    BOOST_CHECK((has_safe_cast<const rational &, const real &>::value));
    BOOST_CHECK((!has_safe_cast<double, real>::value));
    BOOST_CHECK((!has_safe_cast<float, real>::value));
    BOOST_CHECK((!has_safe_cast<real, int>::value));
    BOOST_CHECK((!has_safe_cast<real, float>::value));
    BOOST_CHECK((!has_safe_cast<real, integer>::value));
    BOOST_CHECK((!has_safe_cast<real, rational>::value));
    BOOST_CHECK((!has_safe_cast<real, void>::value));
    BOOST_CHECK_EQUAL(safe_cast<int>(3_r), 3);
    BOOST_CHECK_EQUAL(safe_cast<int>(-3_r), -3);
#if defined(MPPP_HAVE_GCC_INT128)
    BOOST_CHECK(safe_cast<__int128_t>(3_r) == 3);
    BOOST_CHECK(safe_cast<__uint128_t>(3_r) == 3u);
    BOOST_CHECK_THROW(safe_cast<__uint128_t>(-3_r), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<__uint128_t>(real{__uint128_t(-1)} * 2), safe_cast_failure);
#endif
    BOOST_CHECK_EQUAL(safe_cast<unsigned>(4_r), 4u);
    BOOST_CHECK_EQUAL(safe_cast<integer>(4_r), 4_z);
    BOOST_CHECK_EQUAL(safe_cast<integer>(-4_r), -4_z);
    BOOST_CHECK_EQUAL(safe_cast<rational>(4_r), 4_q);
    BOOST_CHECK_EQUAL(safe_cast<rational>(-4_r), -4_q);
    BOOST_CHECK_EQUAL(safe_cast<rational>(5_r / 2), 5_q / 2);
    BOOST_CHECK_EQUAL(safe_cast<rational>(-5_r / 2), -5_q / 2);
    // Various types of failures.
    BOOST_CHECK_EXCEPTION(safe_cast<int>(3.1_r), safe_cast_failure, [](const safe_cast_failure &e) {
        return boost::contains(e.what(), "as the real does not represent a finite integral value");
    });
    BOOST_CHECK_THROW(safe_cast<int>(-3.1_r), safe_cast_failure);
    BOOST_CHECK_EXCEPTION(safe_cast<int>(real{"inf", 100}), safe_cast_failure, [](const safe_cast_failure &e) {
        return boost::contains(e.what(), "as the real does not represent a finite integral value");
    });
    BOOST_CHECK_THROW(safe_cast<int>(real{"nan", 100}), safe_cast_failure);
    BOOST_CHECK_EXCEPTION(safe_cast<int>(real{std::numeric_limits<int>::max()} * 2), safe_cast_failure,
                          [](const safe_cast_failure &e) {
                              return boost::contains(e.what(), "as the conversion would result in overflow");
                          });
    BOOST_CHECK_THROW(safe_cast<int>(real{std::numeric_limits<int>::min()} * 2), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<unsigned>(3.1_r), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<unsigned>(-3_r), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<unsigned>(real{"inf", 100}), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<unsigned>(real{"nan", 100}), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<unsigned>(real{std::numeric_limits<unsigned>::max()} * 2), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<integer>(3.1_r), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<integer>(-3.1_r), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<integer>(real{"inf", 100}), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<integer>(real{"nan", 100}), safe_cast_failure);
    BOOST_CHECK_THROW(safe_cast<rational>(real{"inf", 100}), safe_cast_failure);
    BOOST_CHECK_EXCEPTION(safe_cast<rational>(real{"nan", 100}), safe_cast_failure, [](const safe_cast_failure &e) {
        return boost::contains(e.what(), "cannot convert the non-finite real value");
    });
}

#else

int main()
{
    return 0;
}

#endif
