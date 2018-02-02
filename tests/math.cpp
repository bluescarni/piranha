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

#include <piranha/math.hpp>

#define BOOST_TEST_MODULE math_test
#include <boost/test/included/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <cmath>
#include <complex>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mp++/config.hpp>

#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/monomial.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

const boost::fusion::vector<char, short, int, long, long long, unsigned char, unsigned short, unsigned, unsigned long,
                            unsigned long long, float, double, long double>
arithmetic_values(static_cast<char>(-42), static_cast<short>(42), -42, 42L, -42LL, static_cast<unsigned char>(42),
                  static_cast<unsigned short>(42), 42U, 42UL, 42ULL, 23.456f, -23.456, 23.456L);

static std::mt19937 rng;
static const int ntries = 1000;

struct check_negate {
    template <typename T>
    void operator()(const T &value) const
    {
        if (std::is_signed<T>::value && std::is_integral<T>::value) {
            T negation(value);
            math::negate(negation);
            BOOST_CHECK_EQUAL(negation, -value);
        }
        BOOST_CHECK(has_negate<T>::value);
    }
};

struct no_negate {
};

struct no_negate2 {
    no_negate2 operator-() const;
    no_negate2(const no_negate2 &) = delete;
    no_negate2 &operator=(no_negate2 &) = delete;
};

struct yes_negate {
    yes_negate operator-() const;
};

BOOST_AUTO_TEST_CASE(math_negate_test)
{
    boost::fusion::for_each(arithmetic_values, check_negate());
    BOOST_CHECK(!has_negate<no_negate>::value);
    BOOST_CHECK(!has_negate<no_negate2>::value);
    BOOST_CHECK(has_negate<yes_negate>::value);
    BOOST_CHECK(has_negate<std::complex<double>>::value);
    BOOST_CHECK(has_negate<int &>::value);
    BOOST_CHECK(has_negate<int &&>::value);
    BOOST_CHECK(!has_negate<int const>::value);
    BOOST_CHECK(!has_negate<int const &>::value);
    BOOST_CHECK(!has_negate<std::complex<float> const &>::value);
}

struct no_fma {
};

struct check_multiply_accumulate {
    template <typename T>
    void operator()(const T &,
                    typename std::enable_if<!std::is_same<T, char>::value && !std::is_same<T, unsigned char>::value
                                            && !std::is_same<T, signed char>::value && !std::is_same<T, short>::value
                                            && !std::is_same<T, unsigned short>::value>::type * = nullptr) const
    {
        BOOST_CHECK(has_multiply_accumulate<T>::value);
        BOOST_CHECK(has_multiply_accumulate<T &>::value);
        BOOST_CHECK(!has_multiply_accumulate<const T>::value);
        BOOST_CHECK(!has_multiply_accumulate<const T &>::value);
        T x(2);
        math::multiply_accumulate(x, T(4), T(6));
        BOOST_CHECK_EQUAL(x, T(2) + T(4) * T(6));
        if ((std::is_signed<T>::value && std::is_integral<T>::value) || std::is_floating_point<T>::value) {
            x = T(-2);
            math::multiply_accumulate(x, T(5), T(-7));
            BOOST_CHECK_EQUAL(x, T(-2) + T(5) * T(-7));
        }
    }
    // NOTE: avoid testing with char and short types, the promotion rules will generate warnings about assigning int to
    // narrower types.
    template <typename T>
    void operator()(const T &,
                    typename std::enable_if<std::is_same<T, char>::value || std::is_same<T, unsigned char>::value
                                            || std::is_same<T, signed char>::value || std::is_same<T, short>::value
                                            || std::is_same<T, unsigned short>::value>::type * = nullptr) const
    {
    }
};

BOOST_AUTO_TEST_CASE(math_multiply_accumulate_test)
{
    boost::fusion::for_each(arithmetic_values, check_multiply_accumulate());
    BOOST_CHECK(!has_multiply_accumulate<no_fma>::value);
    BOOST_CHECK(!has_multiply_accumulate<no_fma const &>::value);
    BOOST_CHECK(!has_multiply_accumulate<no_fma &>::value);
}

BOOST_AUTO_TEST_CASE(math_partial_test)
{
    BOOST_CHECK(piranha::is_differentiable<int>::value);
    BOOST_CHECK(piranha::is_differentiable<long>::value);
    BOOST_CHECK(piranha::is_differentiable<double>::value);
    BOOST_CHECK(piranha::is_differentiable<double &>::value);
    BOOST_CHECK(piranha::is_differentiable<double &&>::value);
    BOOST_CHECK(piranha::is_differentiable<double const &>::value);
    BOOST_CHECK(!piranha::is_differentiable<std::string>::value);
    BOOST_CHECK(!piranha::is_differentiable<std::string &>::value);
    BOOST_CHECK_EQUAL(math::partial(1, ""), 0);
    BOOST_CHECK_EQUAL(math::partial(1., ""), double(0));
    BOOST_CHECK_EQUAL(math::partial(2L, ""), 0L);
    BOOST_CHECK_EQUAL(math::partial(2LL, std::string("")), 0LL);
}

BOOST_AUTO_TEST_CASE(math_evaluate_test)
{
    BOOST_CHECK_EQUAL(math::evaluate(5, symbol_fmap<double>{}), 5);
    BOOST_CHECK_EQUAL(math::evaluate(std::complex<float>(5., 4.), symbol_fmap<double>{}), std::complex<float>(5., 4.));
    BOOST_CHECK_EQUAL(math::evaluate(std::complex<double>(5., 4.), symbol_fmap<double>{}),
                      std::complex<double>(5., 4.));
    BOOST_CHECK_EQUAL(math::evaluate(std::complex<long double>(5., 4.), symbol_fmap<double>{}),
                      std::complex<long double>(5., 4.));
    BOOST_CHECK((std::is_same<decltype(math::evaluate(5, symbol_fmap<double>{})), double>::value));
    BOOST_CHECK((std::is_same<decltype(math::evaluate(std::complex<double>(3, 5), symbol_fmap<int>{})),
                              std::complex<double>>::value));
    BOOST_CHECK_EQUAL(math::evaluate(5., symbol_fmap<int>{}), 5.);
    BOOST_CHECK((std::is_same<decltype(math::evaluate(5., symbol_fmap<short>{})), double>::value));
    BOOST_CHECK_EQUAL(math::evaluate(5ul, symbol_fmap<double>{}), 5.);
    BOOST_CHECK((std::is_same<decltype(math::evaluate(5ul, symbol_fmap<short>{})), unsigned long>::value));
    // Test the syntax with explicit template parameter.
    BOOST_CHECK_EQUAL(math::evaluate<double>(5, {{"foo", 5}}), 5);
}

BOOST_AUTO_TEST_CASE(math_subs_test)
{
    BOOST_CHECK((!has_subs<double, double>::value));
    BOOST_CHECK((!has_subs<int, double>::value));
    BOOST_CHECK((!has_subs<int, char>::value));
    BOOST_CHECK((!has_subs<int &, char>::value));
    BOOST_CHECK((!has_subs<int, char &>::value));
    BOOST_CHECK((!has_subs<int &, const char &>::value));
    BOOST_CHECK((!has_subs<std::string, std::string>::value));
    BOOST_CHECK((!has_subs<std::string, int>::value));
    BOOST_CHECK((!has_subs<std::string &, int>::value));
    BOOST_CHECK((!has_subs<int, std::string>::value));
    BOOST_CHECK((!has_subs<int, std::string &&>::value));
    BOOST_CHECK((!has_subs<const int, std::string &&>::value));
}

BOOST_AUTO_TEST_CASE(math_integrate_test)
{
    BOOST_CHECK(!piranha::is_integrable<int>::value);
    BOOST_CHECK(!piranha::is_integrable<int const>::value);
    BOOST_CHECK(!piranha::is_integrable<int &>::value);
    BOOST_CHECK(!piranha::is_integrable<int const &>::value);
    BOOST_CHECK(!piranha::is_integrable<long>::value);
    BOOST_CHECK(!piranha::is_integrable<double>::value);
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK(!piranha::is_integrable<real>::value);
#endif
    BOOST_CHECK(!piranha::is_integrable<rational>::value);
    BOOST_CHECK(!piranha::is_integrable<std::string>::value);
}

BOOST_AUTO_TEST_CASE(math_pbracket_test)
{
    BOOST_CHECK(has_pbracket<int>::value);
    BOOST_CHECK(has_pbracket<double>::value);
    BOOST_CHECK(has_pbracket<long double>::value);
    BOOST_CHECK(!has_pbracket<std::string>::value);
    typedef polynomial<rational, monomial<int>> p_type;
    BOOST_CHECK(has_pbracket<p_type>::value);
    BOOST_CHECK_EQUAL(math::pbracket(p_type{}, p_type{}, std::vector<std::string>{}, std::vector<std::string>{}),
                      p_type(0));
    BOOST_CHECK_THROW(math::pbracket(p_type{}, p_type{}, {"p"}, std::vector<std::string>{}), std::invalid_argument);
    BOOST_CHECK_THROW(math::pbracket(p_type{}, p_type{}, {"p"}, {"q", "r"}), std::invalid_argument);
    BOOST_CHECK_THROW(math::pbracket(p_type{}, p_type{}, {"p", "p"}, {"q", "r"}), std::invalid_argument);
    BOOST_CHECK_THROW(math::pbracket(p_type{}, p_type{}, {"p", "q"}, {"q", "q"}), std::invalid_argument);
    BOOST_CHECK_EQUAL(math::pbracket(p_type{}, p_type{}, {"x", "y"}, {"a", "b"}), p_type(0));
    // Pendulum Hamiltonian.
    typedef poisson_series<polynomial<rational, monomial<int>>> ps_type;
    BOOST_CHECK(has_pbracket<ps_type>::value);
    auto m = ps_type{"m"}, p = ps_type{"p"}, l = ps_type{"l"}, g = ps_type{"g"}, th = ps_type{"theta"};
    auto H_p = p * p * (2 * m * l * l).pow(-1) + m * g * l * math::cos(th);
    BOOST_CHECK_EQUAL(math::pbracket(H_p, H_p, {"p"}, {"theta"}), 0);
    // Two body problem.
    auto x = ps_type{"x"}, y = ps_type{"y"}, z = ps_type{"z"};
    auto vx = ps_type{"vx"}, vy = ps_type{"vy"}, vz = ps_type{"vz"}, r = ps_type{"r"};
    auto H_2 = (vx * vx + vy * vy + vz * vz) / 2 - r.pow(-1);
    ps_type::register_custom_derivative(
        "x", [&x, &r](const ps_type &ps) { return ps.partial("x") - ps.partial("r") * x * r.pow(-3); });
    ps_type::register_custom_derivative(
        "y", [&y, &r](const ps_type &ps) { return ps.partial("y") - ps.partial("r") * y * r.pow(-3); });
    ps_type::register_custom_derivative(
        "z", [&z, &r](const ps_type &ps) { return ps.partial("z") - ps.partial("r") * z * r.pow(-3); });
    BOOST_CHECK_EQUAL(math::pbracket(H_2, H_2, {"vx", "vy", "vz"}, {"x", "y", "z"}), 0);
    // Angular momentum integral.
    auto Gx = y * vz - z * vy, Gy = z * vx - x * vz, Gz = x * vy - y * vx;
    BOOST_CHECK_EQUAL(math::pbracket(H_2, Gx, {"vx", "vy", "vz"}, {"x", "y", "z"}), 0);
    BOOST_CHECK_EQUAL(math::pbracket(H_2, Gy, {"vx", "vy", "vz"}, {"x", "y", "z"}), 0);
    BOOST_CHECK_EQUAL(math::pbracket(H_2, Gz, {"vx", "vy", "vz"}, {"x", "y", "z"}), 0);
    BOOST_CHECK(math::pbracket(H_2, Gz + x, {"vx", "vy", "vz"}, {"x", "y", "z"}) != 0);
}

BOOST_AUTO_TEST_CASE(math_abs_test)
{
    BOOST_CHECK(has_abs<int>::value);
    BOOST_CHECK(has_abs<int &>::value);
    BOOST_CHECK(has_abs<const int &>::value);
    BOOST_CHECK(has_abs<const int>::value);
    BOOST_CHECK(has_abs<float>::value);
    BOOST_CHECK(has_abs<double &&>::value);
    BOOST_CHECK(!has_abs<void>::value);
    BOOST_CHECK(!has_abs<std::string>::value);
    BOOST_CHECK_EQUAL(math::abs(static_cast<signed char>(4)), static_cast<signed char>(4));
    BOOST_CHECK_EQUAL(math::abs(static_cast<signed char>(-4)), static_cast<signed char>(4));
    BOOST_CHECK_EQUAL(math::abs(short(4)), short(4));
    BOOST_CHECK_EQUAL(math::abs(short(-4)), short(4));
    BOOST_CHECK_EQUAL(math::abs(4), 4);
    BOOST_CHECK_EQUAL(math::abs(-4), 4);
    BOOST_CHECK_EQUAL(math::abs(4l), 4l);
    BOOST_CHECK_EQUAL(math::abs(-4l), 4l);
    BOOST_CHECK_EQUAL(math::abs(4ll), 4ll);
    BOOST_CHECK_EQUAL(math::abs(-4ll), 4ll);
    BOOST_CHECK_EQUAL(math::abs(static_cast<unsigned char>(4)), static_cast<unsigned char>(4));
    BOOST_CHECK_EQUAL(math::abs(static_cast<unsigned short>(4)), static_cast<unsigned short>(4));
    BOOST_CHECK_EQUAL(math::abs(4u), 4u);
    BOOST_CHECK_EQUAL(math::abs(4lu), 4lu);
    ;
    BOOST_CHECK_EQUAL(math::abs(4llu), 4llu);
    BOOST_CHECK_EQUAL(math::abs(1.23f), 1.23f);
    BOOST_CHECK_EQUAL(math::abs(-1.23f), 1.23f);
    BOOST_CHECK_EQUAL(math::abs(1.23), 1.23);
    BOOST_CHECK_EQUAL(math::abs(-1.23), 1.23);
    BOOST_CHECK_EQUAL(math::abs(1.23l), 1.23l);
    BOOST_CHECK_EQUAL(math::abs(-1.23l), 1.23l);
}

BOOST_AUTO_TEST_CASE(math_has_t_degree_test)
{
    BOOST_CHECK(!has_t_degree<int>::value);
    BOOST_CHECK(!has_t_degree<const int>::value);
    BOOST_CHECK(!has_t_degree<int &>::value);
    BOOST_CHECK(!has_t_degree<const int &>::value);
    BOOST_CHECK(!has_t_degree<std::string>::value);
    BOOST_CHECK(!has_t_degree<double>::value);
}

BOOST_AUTO_TEST_CASE(math_has_t_ldegree_test)
{
    BOOST_CHECK(!has_t_ldegree<int>::value);
    BOOST_CHECK(!has_t_ldegree<int const>::value);
    BOOST_CHECK(!has_t_ldegree<int &>::value);
    BOOST_CHECK(!has_t_ldegree<const int &>::value);
    BOOST_CHECK(!has_t_ldegree<std::string>::value);
    BOOST_CHECK(!has_t_ldegree<double>::value);
}

BOOST_AUTO_TEST_CASE(math_has_t_order_test)
{
    BOOST_CHECK(!has_t_order<int>::value);
    BOOST_CHECK(!has_t_order<const int>::value);
    BOOST_CHECK(!has_t_order<int &>::value);
    BOOST_CHECK(!has_t_order<int const &>::value);
    BOOST_CHECK(!has_t_order<std::string>::value);
    BOOST_CHECK(!has_t_order<double>::value);
}

BOOST_AUTO_TEST_CASE(math_has_t_lorder_test)
{
    BOOST_CHECK(!has_t_lorder<int>::value);
    BOOST_CHECK(!has_t_lorder<const int>::value);
    BOOST_CHECK(!has_t_lorder<int &>::value);
    BOOST_CHECK(!has_t_lorder<int const &>::value);
    BOOST_CHECK(!has_t_lorder<std::string>::value);
    BOOST_CHECK(!has_t_lorder<double>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_degree_test)
{
    BOOST_CHECK(!key_has_t_degree<monomial<int>>::value);
    BOOST_CHECK(!key_has_t_degree<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_ldegree_test)
{
    BOOST_CHECK(!key_has_t_ldegree<monomial<int>>::value);
    BOOST_CHECK(!key_has_t_ldegree<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_order_test)
{
    BOOST_CHECK(!key_has_t_order<monomial<int>>::value);
    BOOST_CHECK(!key_has_t_order<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_lorder_test)
{
    BOOST_CHECK(!key_has_t_lorder<monomial<int>>::value);
    BOOST_CHECK(!key_has_t_lorder<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_t_subs_test)
{
    BOOST_CHECK((!has_t_subs<double, double, double>::value));
    BOOST_CHECK((!has_t_subs<int, double, double>::value));
    BOOST_CHECK((!has_t_subs<int, char, char>::value));
    BOOST_CHECK((!has_t_subs<int &, char, char>::value));
    BOOST_CHECK((!has_t_subs<int, char &, char &>::value));
    BOOST_CHECK((!has_t_subs<int, char &, short>::value));
    BOOST_CHECK((!has_t_subs<int &, const char &, const char &>::value));
    BOOST_CHECK((!has_t_subs<int &, const char &, const std::string &>::value));
    BOOST_CHECK((!has_t_subs<std::string, std::string, std::string>::value));
    BOOST_CHECK((!has_t_subs<std::string, int, int>::value));
    BOOST_CHECK((!has_t_subs<std::string &, int, int>::value));
    BOOST_CHECK((!has_t_subs<std::string &, int, std::string &&>::value));
}

BOOST_AUTO_TEST_CASE(math_canonical_test)
{
    typedef polynomial<rational, monomial<short>> p_type1;
    BOOST_CHECK_THROW(math::transformation_is_canonical({p_type1{"p"}, p_type1{"p"}}, {p_type1{"q"}}, {"p"}, {"q"}),
                      std::invalid_argument);
    BOOST_CHECK_THROW(math::transformation_is_canonical({p_type1{"p"}}, {p_type1{"q"}}, {"p", "x"}, {"q"}),
                      std::invalid_argument);
    BOOST_CHECK_THROW(math::transformation_is_canonical({p_type1{"p"}}, {p_type1{"q"}}, {"p", "x"}, {"q", "y"}),
                      std::invalid_argument);
    BOOST_CHECK((math::transformation_is_canonical({p_type1{"p"}}, {p_type1{"q"}}, {"p"}, {"q"})));
    p_type1 px{"px"}, py{"py"}, x{"x"}, y{"y"};
    BOOST_CHECK((math::transformation_is_canonical({py, px}, {y, x}, {"px", "py"}, {"x", "y"})));
    BOOST_CHECK((!math::transformation_is_canonical({py, px}, {x, y}, {"px", "py"}, {"x", "y"})));
    BOOST_CHECK((math::transformation_is_canonical({-x, -y}, {px, py}, {"px", "py"}, {"x", "y"})));
    BOOST_CHECK((!math::transformation_is_canonical({x, y}, {px, py}, {"px", "py"}, {"x", "y"})));
    BOOST_CHECK((math::transformation_is_canonical({px, px + py}, {x - y, y}, {"px", "py"}, {"x", "y"})));
    BOOST_CHECK((!math::transformation_is_canonical({px, px - py}, {x - y, y}, {"px", "py"}, {"x", "y"})));
    BOOST_CHECK((math::transformation_is_canonical(std::vector<p_type1>{px, px + py}, std::vector<p_type1>{x - y, y},
                                                   {"px", "py"}, {"x", "y"})));
    // Linear transformation.
    p_type1 L{"L"}, G{"G"}, H{"H"}, l{"l"}, g{"g"}, h{"h"};
    BOOST_CHECK((
        math::transformation_is_canonical({L + G + H, L + G, L}, {h, g - h, l - g}, {"L", "G", "H"}, {"l", "g", "h"})));
    // Unimodular matrices.
    BOOST_CHECK((math::transformation_is_canonical({L + 2 * G + 3 * H, -4 * G + H, 3 * G - H},
                                                   {l, 11 * l - g - 3 * h, 14 * l - g - 4 * h}, {"L", "G", "H"},
                                                   {"l", "g", "h"})));
    BOOST_CHECK((math::transformation_is_canonical(
        {2 * L + 3 * G + 2 * H, 4 * L + 2 * G + 3 * H, 9 * L + 6 * G + 7 * H},
        {-4 * l - g + 6 * h, -9 * l - 4 * g + 15 * h, 5 * l + 2 * g - 8 * h}, {"L", "G", "H"}, {"l", "g", "h"})));
    BOOST_CHECK((!math::transformation_is_canonical(
        {2 * L + 3 * G + 2 * H, 4 * L + 2 * G + 3 * H, 9 * L + 6 * G + 7 * H},
        {-4 * l - g + 6 * h, -9 * l - 4 * g + 15 * h, 5 * l + 2 * g - 7 * h}, {"L", "G", "H"}, {"l", "g", "h"})));
    BOOST_CHECK((math::transformation_is_canonical(
        std::vector<p_type1>{2 * L + 3 * G + 2 * H, 4 * L + 2 * G + 3 * H, 9 * L + 6 * G + 7 * H},
        std::vector<p_type1>{-4 * l - g + 6 * h, -9 * l - 4 * g + 15 * h, 5 * l + 2 * g - 8 * h}, {"L", "G", "H"},
        {"l", "g", "h"})));
    BOOST_CHECK((!math::transformation_is_canonical(
        std::vector<p_type1>{2 * L + 3 * G + 2 * H, 4 * L + 2 * G + 3 * H, 9 * L + 6 * G + 7 * H},
        std::vector<p_type1>{-4 * l - g + 6 * h, -9 * l - 4 * g + 15 * h, 5 * l + 2 * g - 7 * h}, {"L", "G", "H"},
        {"l", "g", "h"})));
    typedef poisson_series<p_type1> p_type2;
    // Poincare' variables.
    p_type2 P{"P"}, Q{"Q"}, p{"p"}, q{"q"}, P2{"P2"}, Q2{"Q2"};
    p_type2::register_custom_derivative(
        "P", [&P2](const p_type2 &arg) { return arg.partial("P") + arg.partial("P2") * math::pow(P2, -1); });
    p_type2::register_custom_derivative(
        "Q", [&Q2](const p_type2 &arg) { return arg.partial("Q") + arg.partial("Q2") * math::pow(Q2, -1); });
    BOOST_CHECK((math::transformation_is_canonical({P2 * math::cos(p), Q2 * math::cos(q)},
                                                   {P2 * math::sin(p), Q2 * math::sin(q)}, {"P", "Q"}, {"p", "q"})));
    BOOST_CHECK(
        (!math::transformation_is_canonical({P * Q * math::cos(p) * q, Q * P * math::sin(3 * q) * p * math::pow(q, -1)},
                                            {P * math::sin(p), Q * math::sin(q)}, {"P", "Q"}, {"p", "q"})));
    BOOST_CHECK((!math::transformation_is_canonical(std::vector<p_type2>{P2 * math::cos(p) * q, Q2 * math::cos(q) * p},
                                                    std::vector<p_type2>{P2 * math::sin(p), Q2 * math::sin(q)},
                                                    {"P", "Q"}, {"p", "q"})));
    BOOST_CHECK(has_transformation_is_canonical<p_type1>::value);
    BOOST_CHECK(has_transformation_is_canonical<p_type1 &>::value);
    BOOST_CHECK(has_transformation_is_canonical<p_type1 const &>::value);
    BOOST_CHECK(has_transformation_is_canonical<p_type2>::value);
    BOOST_CHECK(has_transformation_is_canonical<int>::value);
    BOOST_CHECK(has_transformation_is_canonical<double>::value);
    BOOST_CHECK(has_transformation_is_canonical<double &&>::value);
    BOOST_CHECK(!has_transformation_is_canonical<std::string>::value);
    BOOST_CHECK(!has_transformation_is_canonical<std::string &>::value);
    BOOST_CHECK(!has_transformation_is_canonical<std::string const &>::value);
}

// Non-evaluable, missing copy-ctor.
struct fake_ne {
    fake_ne &operator=(const fake_ne &) = delete;
    fake_ne(const fake_ne &) = delete;
};

BOOST_AUTO_TEST_CASE(math_is_evaluable_test)
{
    BOOST_CHECK((is_evaluable<int, int>::value));
    BOOST_CHECK((is_evaluable<double, double>::value));
    BOOST_CHECK((is_evaluable<double, int>::value));
    BOOST_CHECK((is_evaluable<int &, int>::value));
    BOOST_CHECK((is_evaluable<double, int>::value));
    BOOST_CHECK((is_evaluable<double &, int>::value));
    BOOST_CHECK((is_evaluable<double &&, int>::value));
    BOOST_CHECK((is_evaluable<std::string, int>::value));
    BOOST_CHECK((is_evaluable<std::set<int>, int>::value));
    BOOST_CHECK((is_evaluable<std::string &, int>::value));
    BOOST_CHECK((is_evaluable<std::set<int> &&, int>::value));
    BOOST_CHECK((!is_evaluable<fake_ne, int>::value));
    BOOST_CHECK((!is_evaluable<fake_ne &, int>::value));
    BOOST_CHECK((!is_evaluable<fake_ne const &, int>::value));
    BOOST_CHECK((!is_evaluable<fake_ne &&, int>::value));
}

namespace piranha
{
namespace math
{

// A few fake specialisations to test the degree truncation type traits.

// double-double, correct one.
template <>
struct truncate_degree_impl<double, double, void> {
    double operator()(const double &, const double &) const;
    double operator()(const double &, const double &, const symbol_fset &) const;
};

// double-float, missing one of the two overloads.
template <>
struct truncate_degree_impl<double, float, void> {
    double operator()(const double &, const float &) const;
};

// float-double, wrong return type.
template <>
struct truncate_degree_impl<float, double, void> {
    double operator()(const float &, const double &) const;
    double operator()(const float &, const double &, const symbol_fset &) const;
};
}
}

BOOST_AUTO_TEST_CASE(math_has_truncate_degree_test)
{
    BOOST_CHECK((!has_truncate_degree<float, float>::value));
    BOOST_CHECK((!has_truncate_degree<float &, float>::value));
    BOOST_CHECK((!has_truncate_degree<float &, const float &>::value));
    BOOST_CHECK((!has_truncate_degree<int &&, const float &>::value));
    BOOST_CHECK((!has_truncate_degree<int &&, long double>::value));
    BOOST_CHECK((!has_truncate_degree<std::string, long double>::value));
    BOOST_CHECK((!has_truncate_degree<std::string, std::vector<int>>::value));
    BOOST_CHECK((has_truncate_degree<double, double>::value));
    BOOST_CHECK((has_truncate_degree<const double, double>::value));
    BOOST_CHECK((has_truncate_degree<double &&, double>::value));
    BOOST_CHECK((has_truncate_degree<double &&, double const &>::value));
    BOOST_CHECK((!has_truncate_degree<double, float>::value));
    BOOST_CHECK((!has_truncate_degree<const double, float &>::value));
    BOOST_CHECK((!has_truncate_degree<double &&, float &>::value));
    BOOST_CHECK((!has_truncate_degree<float, double>::value));
    BOOST_CHECK((!has_truncate_degree<const float, double &>::value));
    BOOST_CHECK((!has_truncate_degree<float &&, double &>::value));
}

BOOST_AUTO_TEST_CASE(math_is_unitary_test)
{
    BOOST_CHECK(has_is_unitary<int>::value);
    BOOST_CHECK(has_is_unitary<const int>::value);
    BOOST_CHECK(has_is_unitary<int &>::value);
    BOOST_CHECK(has_is_unitary<float>::value);
    BOOST_CHECK(has_is_unitary<double>::value);
    BOOST_CHECK(has_is_unitary<double &&>::value);
    BOOST_CHECK(!has_is_unitary<std::string>::value);
    BOOST_CHECK(!has_is_unitary<std::string &>::value);
    BOOST_CHECK(!has_is_unitary<const std::string &>::value);
    BOOST_CHECK(math::is_unitary(1));
    BOOST_CHECK(math::is_unitary(1ull));
    BOOST_CHECK(math::is_unitary(char(1)));
    BOOST_CHECK(math::is_unitary(1.));
    BOOST_CHECK(math::is_unitary(1.f));
    BOOST_CHECK(!math::is_unitary(0));
    BOOST_CHECK(!math::is_unitary(-1));
    BOOST_CHECK(!math::is_unitary(2ull));
    BOOST_CHECK(!math::is_unitary(0.));
    BOOST_CHECK(!math::is_unitary(-1.));
    BOOST_CHECK(!math::is_unitary(2.f));
    BOOST_CHECK(!math::is_unitary(2.5f));
}

// Mock key subs method only for certain types.
struct mock_key {
    mock_key() = default;
    mock_key(const mock_key &) = default;
    mock_key(mock_key &&) noexcept;
    mock_key &operator=(const mock_key &) = default;
    mock_key &operator=(mock_key &&) noexcept;
    mock_key(const symbol_fset &);
    bool operator==(const mock_key &) const;
    bool operator!=(const mock_key &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const noexcept;
    mock_key merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
    void trim_identify(std::vector<char> &, const symbol_fset &) const;
    mock_key trim(const std::vector<char> &, const symbol_fset &) const;
    std::vector<std::pair<int, mock_key>> subs(const symbol_idx_fmap<int> &smap, const symbol_fset &args) const;
};

namespace std
{

template <>
struct hash<mock_key> {
    std::size_t operator()(const mock_key &) const;
};
}

BOOST_AUTO_TEST_CASE(math_key_has_subs_test)
{
    BOOST_CHECK((key_has_subs<mock_key, int>::value));
    BOOST_CHECK((!key_has_subs<mock_key, std::string>::value));
    BOOST_CHECK((!key_has_subs<mock_key, integer>::value));
    BOOST_CHECK((!key_has_subs<mock_key, rational>::value));
}

BOOST_AUTO_TEST_CASE(math_ternary_ops_test)
{
    {
        // Addition.
        BOOST_CHECK(has_add3<int>::value);
        BOOST_CHECK(has_add3<int &>::value);
        BOOST_CHECK(!has_add3<const int &>::value);
        int i1 = 0;
        math::add3(i1, 3, 4);
        BOOST_CHECK_EQUAL(i1, 7);
        BOOST_CHECK(has_add3<short>::value);
        short s1 = 1;
        math::add3(s1, short(3), short(-4));
        BOOST_CHECK_EQUAL(s1, -1);
        BOOST_CHECK(has_add3<float>::value);
        BOOST_CHECK(has_add3<double>::value);
        float f1 = 1.234f;
        math::add3(f1, 3.456f, 8.145f);
        BOOST_CHECK_EQUAL(f1, 3.456f + 8.145f);
        BOOST_CHECK(has_add3<std::string>::value);
        std::string foo;
        math::add3(foo, std::string("hello "), std::string("world"));
        BOOST_CHECK_EQUAL(foo, "hello world");
        BOOST_CHECK(!has_add3<std::vector<int>>::value);
        BOOST_CHECK(!has_add3<char *>::value);
    }
    {
        // Subtraction.
        BOOST_CHECK(has_sub3<int>::value);
        BOOST_CHECK(has_sub3<int &>::value);
        BOOST_CHECK(!has_sub3<const int &>::value);
        int i1 = 0;
        math::sub3(i1, 3, 4);
        BOOST_CHECK_EQUAL(i1, -1);
        BOOST_CHECK(has_sub3<short>::value);
        short s1 = 1;
        math::sub3(s1, short(3), short(-4));
        BOOST_CHECK_EQUAL(s1, 7);
        BOOST_CHECK(has_sub3<float>::value);
        BOOST_CHECK(has_sub3<double>::value);
        float f1 = 1.234f;
        math::sub3(f1, 3.456f, 8.145f);
        BOOST_CHECK_EQUAL(f1, 3.456f - 8.145f);
        BOOST_CHECK(!has_sub3<std::string>::value);
        BOOST_CHECK(!has_sub3<std::vector<int>>::value);
        BOOST_CHECK(!has_sub3<char *>::value);
    }
    {
        // Multiplication.
        BOOST_CHECK(has_mul3<int>::value);
        BOOST_CHECK(has_mul3<int &>::value);
        BOOST_CHECK(!has_mul3<const int &>::value);
        int i1 = 0;
        math::mul3(i1, 3, 4);
        BOOST_CHECK_EQUAL(i1, 12);
        BOOST_CHECK(has_mul3<short>::value);
        short s1 = 1;
        math::mul3(s1, short(3), short(-4));
        BOOST_CHECK_EQUAL(s1, -12);
        BOOST_CHECK(has_mul3<float>::value);
        BOOST_CHECK(has_mul3<double>::value);
        float f1 = 1.234f;
        math::mul3(f1, 3.456f, 8.145f);
        BOOST_CHECK_EQUAL(f1, 3.456f * 8.145f);
        BOOST_CHECK(!has_mul3<std::string>::value);
        BOOST_CHECK(!has_mul3<std::vector<int>>::value);
        BOOST_CHECK(!has_mul3<char *>::value);
    }
    {
        // Division.
        BOOST_CHECK(has_div3<int>::value);
        BOOST_CHECK(has_div3<int &>::value);
        BOOST_CHECK(!has_div3<const int &>::value);
        int i1 = 0;
        math::div3(i1, 6, 3);
        BOOST_CHECK_EQUAL(i1, 2);
        BOOST_CHECK(has_div3<short>::value);
        short s1 = -8;
        math::div3(s1, short(-8), short(2));
        BOOST_CHECK_EQUAL(s1, -4);
        BOOST_CHECK(has_div3<float>::value);
        BOOST_CHECK(has_div3<double>::value);
        float f1 = 1.234f;
        math::div3(f1, 3.456f, 8.145f);
        BOOST_CHECK_EQUAL(f1, 3.456f / 8.145f);
        BOOST_CHECK(!has_div3<std::string>::value);
        BOOST_CHECK(!has_div3<std::vector<int>>::value);
        BOOST_CHECK(!has_div3<char *>::value);
    }
}

// A fake GCD-enabled type.
struct mock_type {
};

namespace piranha
{
namespace math
{

template <typename T>
struct gcd_impl<T, T, typename std::enable_if<std::is_same<T, mock_type>::value>::type> {
    mock_type operator()(const mock_type &, const mock_type &) const
    {
        return mock_type{};
    }
};
}
}

BOOST_AUTO_TEST_CASE(math_gcd_test)
{
    using math::gcd;
    using math::gcd3;
    BOOST_CHECK_EQUAL(gcd(0, 0), 0);
    BOOST_CHECK_EQUAL(gcd(0, 12), 12);
    BOOST_CHECK_EQUAL(gcd(14, 0), 14);
    BOOST_CHECK_EQUAL(gcd(4, 3), 1);
    BOOST_CHECK_EQUAL(gcd(3, 4), 1);
    BOOST_CHECK_EQUAL(gcd(4, 6), 2);
    BOOST_CHECK_EQUAL(gcd(6, 4), 2);
    BOOST_CHECK_EQUAL(gcd(4, 25), 1);
    BOOST_CHECK_EQUAL(gcd(25, 4), 1);
    BOOST_CHECK_EQUAL(gcd(27, 54), 27);
    BOOST_CHECK_EQUAL(gcd(54, 27), 27);
    BOOST_CHECK_EQUAL(gcd(1, 54), 1);
    BOOST_CHECK_EQUAL(gcd(54, 1), 1);
    BOOST_CHECK_EQUAL(gcd(36, 24), 12);
    BOOST_CHECK_EQUAL(gcd(24, 36), 12);
    // Check compiler warnings with short ints.
    BOOST_CHECK((!std::is_same<decltype(gcd(short(54), short(27))), short>::value));
    BOOST_CHECK((!std::is_same<decltype(gcd(short(54), char(27))), short>::value));
    BOOST_CHECK_EQUAL(gcd(short(54), short(27)), short(27));
    BOOST_CHECK_EQUAL(gcd(short(27), short(53)), short(1));
    BOOST_CHECK(gcd(short(27), short(-54)) == short(27) || gcd(short(27), short(-54)) == short(-27));
    BOOST_CHECK(gcd(short(-54), short(27)) == short(27) || gcd(short(-54), short(27)) == short(-27));
    // Check with different signs.
    BOOST_CHECK(gcd(27, -54) == 27 || gcd(27, -54) == -27);
    BOOST_CHECK(gcd(-54, 27) == 27 || gcd(-54, 27) == -27);
    BOOST_CHECK(gcd(4, -25) == 1 || gcd(4, -25) == -1);
    BOOST_CHECK(gcd(-25, 4) == 1 || gcd(-25, 4) == -1);
    BOOST_CHECK(gcd(-25, 1) == -1 || gcd(-25, 1) == 1);
    BOOST_CHECK(gcd(25, -1) == -1 || gcd(25, -1) == 1);
    BOOST_CHECK(gcd(-24, 36) == -12 || gcd(-24, 36) == 12);
    BOOST_CHECK(gcd(24, -36) == -12 || gcd(24, -36) == 12);
    // Check with zeroes.
    BOOST_CHECK_EQUAL(gcd(54, 0), 54);
    BOOST_CHECK_EQUAL(gcd(0, 54), 54);
    BOOST_CHECK_EQUAL(gcd(0, 0), 0);
    // The ternary form, check particularily with respect to short ints.
    int out;
    gcd3(out, 12, 9);
    BOOST_CHECK_EQUAL(out, 3);
    short s_out;
    gcd3(s_out, short(12), short(9));
    char c_out;
    gcd3(c_out, char(12), char(9));
    // Random testing.
    std::uniform_int_distribution<int> int_dist(-detail::safe_abs_sint<int>::value, detail::safe_abs_sint<int>::value);
    for (int i = 0; i < ntries; ++i) {
        int a(int_dist(rng)), b(int_dist(rng));
        int c;
        int g = gcd(a, b);
        gcd3(c, a, b);
        if (g == 0) {
            continue;
        }
        BOOST_CHECK_EQUAL(c, g);
        BOOST_CHECK_EQUAL(a % g, 0);
        BOOST_CHECK_EQUAL(b % g, 0);
    }
    // Check the type traits.
    BOOST_CHECK((has_gcd<int>::value));
    BOOST_CHECK((!has_gcd<void>::value));
    BOOST_CHECK((has_gcd<int, long>::value));
    BOOST_CHECK((!has_gcd<int, void>::value));
    BOOST_CHECK((!has_gcd<void, int>::value));
    BOOST_CHECK((has_gcd<int &, char &>::value));
    BOOST_CHECK((!has_gcd<double>::value));
    BOOST_CHECK((!has_gcd<double, int>::value));
    BOOST_CHECK((!has_gcd<std::string>::value));
    BOOST_CHECK((!has_gcd<int, std::string>::value));
    BOOST_CHECK(has_gcd3<int>::value);
    BOOST_CHECK(has_gcd3<char>::value);
    BOOST_CHECK(has_gcd3<short>::value);
    BOOST_CHECK(has_gcd3<long long>::value);
    BOOST_CHECK(has_gcd3<long long &>::value);
    BOOST_CHECK(!has_gcd3<long long const>::value);
    BOOST_CHECK(!has_gcd3<void>::value);
    BOOST_CHECK(!has_gcd3<const long long &>::value);
    BOOST_CHECK(!has_gcd3<double>::value);
    BOOST_CHECK(!has_gcd3<double &&>::value);
    BOOST_CHECK(!has_gcd3<std::string>::value);
    // Try the mock type.
    BOOST_CHECK(has_gcd<mock_type>::value);
    BOOST_CHECK((!has_gcd<mock_type, int>::value));
    BOOST_CHECK((!has_gcd<int, mock_type>::value));
    BOOST_CHECK(has_gcd3<mock_type>::value);
    BOOST_CHECK_NO_THROW(gcd(mock_type{}, mock_type{}));
    mock_type m0;
    BOOST_CHECK_NO_THROW(gcd3(m0, mock_type{}, mock_type{}));
}
