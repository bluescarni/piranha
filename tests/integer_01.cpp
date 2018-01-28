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

#include <piranha/integer.hpp>

#define BOOST_TEST_MODULE integer_01_test
#include <boost/test/included/unit_test.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <boost/algorithm/string/predicate.hpp>

#include <mp++/config.hpp>
#include <mp++/exceptions.hpp>
#include <mp++/integer.hpp>

#include <piranha/math.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using size_types = std::tuple<std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 2>,
                              std::integral_constant<std::size_t, 3>, std::integral_constant<std::size_t, 7>,
                              std::integral_constant<std::size_t, 10>>;

struct negate_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(has_negate<int_type>::value);
        BOOST_CHECK(has_negate<int_type &>::value);
        BOOST_CHECK(!has_negate<const int_type &>::value);
        BOOST_CHECK(!has_negate<const int_type>::value);
        int_type n;
        math::negate(n);
        BOOST_CHECK_EQUAL(n, 0);
        n = 4;
        math::negate(n);
        BOOST_CHECK_EQUAL(n, -4);
        n.promote();
        math::negate(n);
        BOOST_CHECK_EQUAL(n, 4);
    }
};

BOOST_AUTO_TEST_CASE(integer_negate_test)
{
    tuple_for_each(size_types{}, negate_tester{});
}

struct is_zero_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(has_is_zero<int_type>::value);
        BOOST_CHECK(has_is_zero<const int_type>::value);
        BOOST_CHECK(has_is_zero<int_type &>::value);
        BOOST_CHECK(has_is_zero<const int_type &>::value);
        int_type n;
        BOOST_CHECK(math::is_zero(n));
        n = 1;
        BOOST_CHECK(!math::is_zero(n));
        n = 101;
        BOOST_CHECK(!math::is_zero(n));
        n = -1;
        BOOST_CHECK(!math::is_zero(n));
        n = -101;
        BOOST_CHECK(!math::is_zero(n));
        n = 0;
        n.promote();
        BOOST_CHECK(math::is_zero(n));
        n = 1;
        BOOST_CHECK(!math::is_zero(n));
        n = 101;
        BOOST_CHECK(!math::is_zero(n));
        n = -1;
        BOOST_CHECK(!math::is_zero(n));
        n = -101;
        BOOST_CHECK(!math::is_zero(n));
    }
};

BOOST_AUTO_TEST_CASE(integer_is_zero_test)
{
    tuple_for_each(size_types{}, is_zero_tester{});
}

struct addmul_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(has_multiply_accumulate<int_type>::value);
        BOOST_CHECK(has_multiply_accumulate<int_type &>::value);
        BOOST_CHECK(!has_multiply_accumulate<const int_type &>::value);
        BOOST_CHECK(!has_multiply_accumulate<const int_type>::value);
        int_type a{1}, b{2}, c{3};
        math::multiply_accumulate(a, b, c);
        BOOST_CHECK_EQUAL(a, 7);
        b.promote();
        c = -5;
        math::multiply_accumulate(a, b, c);
        BOOST_CHECK_EQUAL(a, -3);
    }
};

BOOST_AUTO_TEST_CASE(integer_multiply_accumulate_test)
{
    tuple_for_each(size_types{}, addmul_tester{});
}

struct is_unitary_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(has_is_unitary<int_type>::value);
        BOOST_CHECK(has_is_unitary<const int_type>::value);
        BOOST_CHECK(has_is_unitary<int_type &>::value);
        BOOST_CHECK(has_is_unitary<const int_type &>::value);
        int_type n;
        BOOST_CHECK(!math::is_unitary(n));
        n = 1;
        BOOST_CHECK(math::is_unitary(n));
        n = -1;
        BOOST_CHECK(!math::is_unitary(n));
        n.promote();
        BOOST_CHECK(!math::is_unitary(n));
        n = 1;
        n.promote();
        BOOST_CHECK(math::is_unitary(n));
    }
};

BOOST_AUTO_TEST_CASE(integer_is_unitary_test)
{
    tuple_for_each(size_types{}, is_unitary_tester{});
}

struct abs_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(has_abs<int_type>::value);
        BOOST_CHECK(has_abs<const int_type>::value);
        BOOST_CHECK(has_abs<int_type &>::value);
        BOOST_CHECK(has_abs<const int_type &>::value);
        int_type n;
        BOOST_CHECK_EQUAL(math::abs(n), 0);
        n = -1;
        BOOST_CHECK_EQUAL(math::abs(n), 1);
        n = 123;
        n.promote();
        BOOST_CHECK_EQUAL(math::abs(n), 123);
    }
};

BOOST_AUTO_TEST_CASE(integer_abs_test)
{
    tuple_for_each(size_types{}, abs_tester{});
}

struct sin_cos_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK_EQUAL(math::sin(int_type()), 0);
        BOOST_CHECK_EQUAL(math::cos(int_type()), 1);
        BOOST_CHECK_EXCEPTION(math::sin(int_type(1)), std::invalid_argument, [](const std::invalid_argument &e) {
            return boost::contains(e.what(), "cannot compute the sine of a non-zero integer");
        });
        BOOST_CHECK_EXCEPTION(math::cos(int_type(1)), std::invalid_argument, [](const std::invalid_argument &e) {
            return boost::contains(e.what(), "cannot compute the cosine of a non-zero integer");
        });
        BOOST_CHECK((std::is_same<int_type, decltype(math::cos(int_type{}))>::value));
        BOOST_CHECK((std::is_same<int_type, decltype(math::sin(int_type{}))>::value));
        BOOST_CHECK(is_sine_type<int_type>::value);
        BOOST_CHECK(has_cosine<int_type>::value);
        BOOST_CHECK(is_sine_type<int_type &>::value);
        BOOST_CHECK(has_cosine<int_type &>::value);
        BOOST_CHECK(is_sine_type<const int_type &>::value);
        BOOST_CHECK(has_cosine<const int_type &>::value);
    }
};

BOOST_AUTO_TEST_CASE(integer_sin_cos_test)
{
    tuple_for_each(size_types{}, sin_cos_tester{});
}

struct partial_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(is_differentiable<int_type>::value);
        BOOST_CHECK(is_differentiable<int_type &>::value);
        BOOST_CHECK(is_differentiable<const int_type &>::value);
        BOOST_CHECK(is_differentiable<const int_type>::value);
        int_type n;
        BOOST_CHECK_EQUAL(math::partial(n, ""), 0);
        n = 5;
        BOOST_CHECK_EQUAL(math::partial(n, "abc"), 0);
        n = -5;
        BOOST_CHECK_EQUAL(math::partial(n, "def"), 0);
    }
};

BOOST_AUTO_TEST_CASE(integer_partial_test)
{
    tuple_for_each(size_types{}, partial_tester{});
}

struct factorial_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        int_type n;
        BOOST_CHECK(math::factorial(n) == 1);
        n = 1;
        BOOST_CHECK(math::factorial(n) == 1);
        n = 2;
        BOOST_CHECK(math::factorial(n) == 2);
        n = 3;
        BOOST_CHECK(math::factorial(n) == 6);
        n = 4;
        BOOST_CHECK(math::factorial(n) == 24);
        n = 5;
        BOOST_CHECK(math::factorial(n) == 24 * 5);
        BOOST_CHECK_EXCEPTION(math::factorial(int_type{-1}), std::domain_error, [](const std::domain_error &de) {
            return boost::contains(de.what(), "cannot compute the factorial of the negative integer -1");
        });
        BOOST_CHECK_EXCEPTION(math::factorial(int_type{-10}), std::domain_error, [](const std::domain_error &de) {
            return boost::contains(de.what(), "cannot compute the factorial of the negative integer -10");
        });
        n = std::numeric_limits<unsigned long>::max();
        ++n;
        BOOST_CHECK_THROW(math::factorial(n), std::overflow_error);
        n = 1000001ull;
        BOOST_CHECK_THROW(math::factorial(n), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(integer_factorial_test)
{
    tuple_for_each(size_types{}, factorial_tester{});
}

struct b_00 {
    b_00() = default;
    b_00(const b_00 &) = delete;
    b_00(b_00 &&) = delete;
};

struct b_01 {
    b_01() = default;
    b_01(const b_01 &) = default;
    b_01(b_01 &&) = default;
    ~b_01() = delete;
};

namespace piranha
{

namespace math
{

template <>
struct ipow_subs_impl<b_00, b_00> {
    b_00 operator()(const b_00 &, const std::string &, const integer &, const b_00 &) const;
};

template <>
struct ipow_subs_impl<b_01, b_01> {
    b_01 operator()(const b_01 &, const std::string &, const integer &, const b_01 &) const;
};
}
}

struct ipow_subs_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK((!has_ipow_subs<int_type, int_type>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, int>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, long>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, double>::value));
        BOOST_CHECK((!has_ipow_subs<int_type &, int_type>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, const int>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, void>::value));
        BOOST_CHECK((!has_ipow_subs<const int_type &, double>::value));
        BOOST_CHECK((!has_ipow_subs<void, void>::value));
    }
};

BOOST_AUTO_TEST_CASE(integer_ipow_subs_test)
{
    tuple_for_each(size_types{}, ipow_subs_tester{});
    BOOST_CHECK((!has_ipow_subs<b_00, b_00>::value));
    BOOST_CHECK((!has_ipow_subs<b_01, b_01>::value));
}

struct ternary_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK(has_add3<int_type>::value);
        BOOST_CHECK(has_add3<int_type &>::value);
        BOOST_CHECK(!has_add3<const int_type &>::value);
        BOOST_CHECK(!has_add3<const int_type>::value);
        BOOST_CHECK(has_sub3<int_type>::value);
        BOOST_CHECK(has_sub3<int_type &>::value);
        BOOST_CHECK(!has_sub3<const int_type &>::value);
        BOOST_CHECK(!has_sub3<const int_type>::value);
        BOOST_CHECK(has_mul3<int_type>::value);
        BOOST_CHECK(has_mul3<int_type &>::value);
        BOOST_CHECK(!has_mul3<const int_type &>::value);
        BOOST_CHECK(!has_mul3<const int_type>::value);
        BOOST_CHECK(has_div3<int_type>::value);
        BOOST_CHECK(has_div3<int_type &>::value);
        BOOST_CHECK(!has_div3<const int_type &>::value);
        BOOST_CHECK(!has_div3<const int_type>::value);
        int_type a, b{1}, c{-3};
        math::add3(a, b, c);
        BOOST_CHECK_EQUAL(a, -2);
        math::sub3(a, b, c);
        BOOST_CHECK_EQUAL(a, 4);
        math::mul3(a, b, c);
        BOOST_CHECK_EQUAL(a, -3);
        b = 6;
        c = -2;
        math::div3(a, b, c);
        BOOST_CHECK_EQUAL(a, -3);
        c = 0;
        BOOST_CHECK_THROW(math::div3(a, b, c), mppp::zero_division_error);
    }
};

BOOST_AUTO_TEST_CASE(integer_ternary_test)
{
    tuple_for_each(size_types{}, ternary_tester{});
}

struct gcd_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK((has_gcd<int_type>::value));
        BOOST_CHECK((has_gcd<int_type, int>::value));
        BOOST_CHECK((has_gcd<long, int_type>::value));
        BOOST_CHECK((has_gcd<char &, const int_type>::value));
        BOOST_CHECK((has_gcd<const char &, const int_type>::value));
        BOOST_CHECK((has_gcd3<int_type>::value));
        BOOST_CHECK((has_gcd3<int_type &>::value));
        BOOST_CHECK((!has_gcd3<const int_type &>::value));
        BOOST_CHECK((!has_gcd3<const int_type>::value));
        BOOST_CHECK((has_gcd<int_type, wchar_t>::value));
        BOOST_CHECK((has_gcd<wchar_t, int_type>::value));
        BOOST_CHECK((!has_gcd<int_type, void>::value));
        BOOST_CHECK((!has_gcd<void, int_type>::value));
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK((has_gcd<int_type, __int128_t>::value));
        BOOST_CHECK((has_gcd<__int128_t, int_type>::value));
        BOOST_CHECK((has_gcd<int_type, __uint128_t>::value));
        BOOST_CHECK((has_gcd<__uint128_t, int_type>::value));
#endif
        BOOST_CHECK_EQUAL(math::gcd(int_type{4}, int_type{6}), 2);
        BOOST_CHECK_EQUAL(math::gcd(int_type{0}, int_type{-6}), 6);
        BOOST_CHECK_EQUAL(math::gcd(int_type{6}, int_type{0}), 6);
        BOOST_CHECK_EQUAL(math::gcd(int_type{0}, int_type{0}), 0);
        BOOST_CHECK_EQUAL(math::gcd(-4, int_type{6}), 2);
        BOOST_CHECK_EQUAL(math::gcd(int_type{4}, -6ll), 2);
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK_EQUAL(math::gcd(__int128_t(-4), int_type{6}), 2);
        BOOST_CHECK_EQUAL(math::gcd(int_type{4}, __uint128_t(6)), 2);
#endif
        int_type n;
        math::gcd3(n, int_type{4}, int_type{6});
        BOOST_CHECK_EQUAL(n, 2);
        math::gcd3(n, int_type{0}, int_type{0});
        BOOST_CHECK_EQUAL(n, 0);
    }
};

BOOST_AUTO_TEST_CASE(integer_gcd_test)
{
    tuple_for_each(size_types{}, gcd_tester{});
    BOOST_CHECK((!has_gcd<mppp::integer<1>, mppp::integer<2>>::value));
    BOOST_CHECK((!has_gcd<mppp::integer<2>, mppp::integer<1>>::value));
}

BOOST_AUTO_TEST_CASE(integer_literal_test)
{
    auto n0 = 12345_z;
    BOOST_CHECK((std::is_same<integer, decltype(n0)>::value));
    BOOST_CHECK_EQUAL(n0, 12345);
    n0 = -456_z;
    BOOST_CHECK_EQUAL(n0, -456l);
    BOOST_CHECK_THROW((n0 = -1234.5_z), std::invalid_argument);
    BOOST_CHECK_EQUAL(n0, -456l);
}

using fp_types = std::tuple<float, double
#if defined(MPPP_WITH_MPFR)
                            ,
                            long double
#endif
                            >;

using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long>;

struct safe_cast_float_tester {
    template <typename S>
    struct runner {
        template <typename T>
        void operator()(const T &) const
        {
            using int_type = mppp::integer<S::value>;
            // Type trait.
            BOOST_CHECK((has_safe_cast<int_type, T>::value));
            BOOST_CHECK((has_safe_cast<int_type, T &>::value));
            BOOST_CHECK((has_safe_cast<const int_type, T &>::value));
            BOOST_CHECK((has_safe_cast<const int_type &, T &&>::value));
            BOOST_CHECK((!has_safe_cast<int_type, void>::value));
            BOOST_CHECK((!has_safe_cast<void, int_type>::value));

            // Simple testing.
            BOOST_CHECK_EQUAL(safe_cast<int_type>(T(0)), int_type{0});
            BOOST_CHECK_EQUAL(safe_cast<int_type>(T(-1)), int_type{-1});
            BOOST_CHECK_EQUAL(safe_cast<int_type>(T(1)), int_type{1});
            BOOST_CHECK_EXCEPTION(safe_cast<int_type>(T(1.5)), safe_cast_failure, [](const safe_cast_failure &e) {
                return boost::contains(e.what(), "the floating-point value with nonzero fractional part");
            });
            BOOST_CHECK_EXCEPTION(safe_cast<int_type>(T(-1.5)), safe_cast_failure, [](const safe_cast_failure &e) {
                return boost::contains(e.what(), "the floating-point value with nonzero fractional part");
            });

            // Non-finite values.
            using lim = std::numeric_limits<T>;
            if (lim::is_iec559) {
                BOOST_CHECK_EXCEPTION(safe_cast<int_type>(lim::quiet_NaN()), safe_cast_failure,
                                      [](const safe_cast_failure &e) {
                                          return boost::contains(e.what(), "the non-finite floating-point value");
                                      });
                BOOST_CHECK_EXCEPTION(safe_cast<int_type>(lim::infinity()), safe_cast_failure,
                                      [](const safe_cast_failure &e) {
                                          return boost::contains(e.what(), "the non-finite floating-point value");
                                      });
            }
        }
    };
    template <typename S>
    void operator()(const S &) const
    {
        tuple_for_each(fp_types{}, runner<S>{});
    }
};

BOOST_AUTO_TEST_CASE(integer_safe_cast_float_test)
{
    tuple_for_each(size_types{}, safe_cast_float_tester());
}

struct safe_cast_int_tester {
    template <typename S>
    struct runner {
        template <typename T>
        void operator()(const T &) const
        {
            using int_type = mppp::integer<S::value>;
            // Type trait.
            BOOST_CHECK((has_safe_cast<int_type, T>::value));
            BOOST_CHECK((has_safe_cast<int_type, T &>::value));
            BOOST_CHECK((has_safe_cast<const int_type, T &>::value));
            BOOST_CHECK((has_safe_cast<const int_type &, T &&>::value));
            BOOST_CHECK((has_safe_cast<T, int_type>::value));
            BOOST_CHECK((has_safe_cast<T &, int_type>::value));
            BOOST_CHECK((has_safe_cast<T &, const int_type>::value));
            BOOST_CHECK((has_safe_cast<T &&, const int_type &>::value));

            // Simple checks.
            BOOST_CHECK(safe_cast<int_type>(T(0)) == int_type{0});
            BOOST_CHECK(safe_cast<int_type>(T(1)) == int_type{1});
            BOOST_CHECK(safe_cast<int_type>(T(12)) == int_type{12});
            BOOST_CHECK(safe_cast<T>(int_type(0)) == T{0});
            BOOST_CHECK(safe_cast<T>(int_type(1)) == T{1});
            BOOST_CHECK(safe_cast<T>(int_type{12}) == T{12});

            // Failures.
            using lim = std::numeric_limits<T>;
            BOOST_CHECK_EXCEPTION(safe_cast<T>(int_type(lim::max()) + 1), safe_cast_failure,
                                  [](const safe_cast_failure &e) {
                                      return boost::contains(e.what(), "as the conversion would result in overflow");
                                  });
            BOOST_CHECK_EXCEPTION(safe_cast<T>(int_type(lim::min()) - 1), safe_cast_failure,
                                  [](const safe_cast_failure &e) {
                                      return boost::contains(e.what(), "as the conversion would result in overflow");
                                  });
        }
    };
    template <typename S>
    void operator()(const S &) const
    {
        tuple_for_each(int_types{}, runner<S>{});
        using int_type = mppp::integer<S::value>;
        BOOST_CHECK((has_safe_cast<int_type, wchar_t>::value));
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK((has_safe_cast<int_type, __int128_t>::value));
        BOOST_CHECK((has_safe_cast<int_type, __uint128_t>::value));
        BOOST_CHECK((has_safe_cast<__int128_t, int_type>::value));
        BOOST_CHECK((has_safe_cast<__uint128_t, int_type>::value));
        BOOST_CHECK(safe_cast<__int128_t>(int_type{12}) == 12);
        BOOST_CHECK(safe_cast<__uint128_t>(int_type{12}) == 12u);
        BOOST_CHECK(safe_cast<int_type>(__int128_t(12)) == 12);
        BOOST_CHECK(safe_cast<int_type>(__uint128_t(12)) == 12);
#endif
    }
};

BOOST_AUTO_TEST_CASE(integer_safe_cast_int_test)
{
    tuple_for_each(size_types{}, safe_cast_int_tester{});
}

struct sep_tester {
    template <typename T>
    using edict = symbol_fmap<T>;
    template <typename T>
    void operator()(const T &) const
    {
        using int_type = mppp::integer<T::value>;
        BOOST_CHECK_EQUAL(math::evaluate(int_type{12}, edict<int>{{"", 1}}), 12);
        BOOST_CHECK_EQUAL(math::evaluate(int_type{10}, edict<double>{{"", 1.321}}), 10);
        BOOST_CHECK((is_evaluable<int_type, int>::value));
        BOOST_CHECK((is_evaluable<int_type, double>::value));
#if defined(MPPP_WITH_MPFR)
        BOOST_CHECK((std::is_same<long double,
                                  decltype(math::evaluate(int_type{10}, edict<long double>{{"", 1.321l}}))>::value));
#else
        BOOST_CHECK(
            (std::is_same<int_type, decltype(math::evaluate(int_type{10}, edict<long double>{{"", 1.321l}}))>::value));
#endif
#if defined(MPPP_HAVE_GCC_INT128)
        BOOST_CHECK((is_evaluable<int_type, __int128_t>::value));
        BOOST_CHECK((is_evaluable<int_type, __uint128_t>::value));
        BOOST_CHECK((math::evaluate(int_type{12}, edict<__int128_t>{{"", 1}}) == 12));
        BOOST_CHECK((math::evaluate(int_type{12}, edict<__uint128_t>{{"", 1}}) == 12));
#endif
        BOOST_CHECK((std::is_same<double, decltype(math::evaluate(int_type{10}, edict<double>{{"", 1.321}}))>::value));
        BOOST_CHECK((!has_subs<int_type, int_type>::value));
        BOOST_CHECK((!has_subs<int_type, int>::value));
        BOOST_CHECK((!has_subs<int_type, long double>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, int>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, double>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, float>::value));
        BOOST_CHECK((!has_ipow_subs<int_type, unsigned short>::value));
    }
};

BOOST_AUTO_TEST_CASE(integer_sep_test)
{
    tuple_for_each(size_types{}, sep_tester());
}
