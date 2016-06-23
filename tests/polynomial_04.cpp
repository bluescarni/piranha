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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE polynomial_04_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

static std::mt19937 rng;
static const int ntrials = 300;

using key_types = boost::mpl::vector<monomial<short>, monomial<integer>, k_monomial>;

template <typename Poly>
inline static Poly rn_poly(const Poly &x, const Poly &y, const Poly &z, std::uniform_int_distribution<int> &dist)
{
    int nterms = dist(rng);
    Poly retval;
    for (int i = 0; i < nterms; ++i) {
        int m = dist(rng);
        retval += m * (m % 2 ? 1 : -1) * x.pow(dist(rng)) * y.pow(dist(rng)) * z.pow(dist(rng));
    }
    return retval;
}

template <typename Poly>
inline static Poly rq_poly(const Poly &x, const Poly &y, const Poly &z, std::uniform_int_distribution<int> &dist)
{
    int nterms = dist(rng);
    Poly retval;
    for (int i = 0; i < nterms; ++i) {
        int m = dist(rng), n = dist(rng);
        retval += (m * (m % 2 ? 1 : -1) * x.pow(dist(rng)) * y.pow(dist(rng)) * z.pow(dist(rng))) / (n == 0 ? 1 : n);
    }
    return retval;
}

struct division_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        using pq_type = polynomial<rational, Key>;
        p_type x{"x"}, y{"y"}, z{"z"};
        pq_type xq{"x"}, yq{"y"}, zq{"z"};
        // Zero division.
        BOOST_CHECK_THROW(p_type{1} / p_type{}, zero_division_error);
        BOOST_CHECK_THROW(x / p_type{}, zero_division_error);
        BOOST_CHECK_THROW(x / (x - x), zero_division_error);
        BOOST_CHECK_THROW((x + y - y) / (x - x + y - y), zero_division_error);
        // Null numerator.
        BOOST_CHECK_EQUAL(p_type{0} / p_type{-2}, 0);
        auto res = (x - x) / p_type{2};
        BOOST_CHECK_EQUAL(res.size(), 0u);
        BOOST_CHECK(res.get_symbol_set() == symbol_set{symbol{"x"}});
        res = (x - x + y - y) / p_type{2};
        BOOST_CHECK_EQUAL(res.size(), 0u);
        BOOST_CHECK((res.get_symbol_set() == symbol_set{symbol{"x"}, symbol{"y"}}));
        res = (x - x + y - y) / (2 + x - x + y - y);
        BOOST_CHECK_EQUAL(res.size(), 0u);
        BOOST_CHECK((res.get_symbol_set() == symbol_set{symbol{"x"}, symbol{"y"}}));
        // Coefficient polys.
        BOOST_CHECK_EQUAL(p_type{12} / p_type{-4}, -3);
        BOOST_CHECK_EQUAL(p_type{24} / p_type{3}, 8);
        BOOST_CHECK_THROW(p_type{12} / p_type{11}, math::inexact_division);
        BOOST_CHECK_EQUAL(pq_type{12} / pq_type{-11}, -12 / 11_q);
        res = (x - x + y - y + 6) / p_type{2};
        BOOST_CHECK_EQUAL(res, 3);
        BOOST_CHECK((res.get_symbol_set() == symbol_set{symbol{"x"}, symbol{"y"}}));
        // Simple univariate tests.
        BOOST_CHECK_EQUAL(x / x, 1);
        x /= x;
        BOOST_CHECK_EQUAL(x, 1);
        x = p_type{"x"};
        BOOST_CHECK_EQUAL(x * x / x, x);
        BOOST_CHECK_EQUAL(x * x * x / x, x * x);
        BOOST_CHECK_EQUAL(2 * x / p_type{2}, x);
        BOOST_CHECK_THROW(x / p_type{2}, math::inexact_division);
        BOOST_CHECK_EQUAL(xq / pq_type{2}, xq / 2);
        BOOST_CHECK_EQUAL((x + 1) * (x - 2) / (x + 1), x - 2);
        BOOST_CHECK_EQUAL((x + 1) * (x - 2) * (x + 3) / ((x + 1) * (x + 3)), x - 2);
        BOOST_CHECK_THROW((x + 1) * (x - 2) / (x + 4), math::inexact_division);
        // Negative exponents.
        BOOST_CHECK_THROW(x / x.pow(-1), std::invalid_argument);
        BOOST_CHECK_THROW(x.pow(-1) / x, std::invalid_argument);
        BOOST_CHECK_THROW((x.pow(-1) + y * x) / x, std::invalid_argument);
        // Will work with zero numerator.
        BOOST_CHECK_EQUAL((x - x) / (x + y.pow(-1)), 0);
        // Simple multivariate tests.
        BOOST_CHECK_EQUAL((2 * x * (x - y)) / x, 2 * x - 2 * y);
        BOOST_CHECK_EQUAL((2 * x * z * (x - y) * (x * x - y)) / (x - y), 2 * x * z * (x * x - y));
        BOOST_CHECK_EQUAL((2 * x * z * (x - y) * (x * x - y)) / (z * (x - y)), 2 * x * (x * x - y));
        BOOST_CHECK_THROW((2 * x * z * (x - y) * (x * x - y)) / (4 * z * (x - y)), math::inexact_division);
        BOOST_CHECK_EQUAL((2 * x * z * (x - y) * (x * x - y)) / (2 * z * (x - y)), x * (x * x - y));
        BOOST_CHECK_EQUAL((2 * xq * zq * (xq - yq) * (xq * xq - yq)) / (4 * zq * (xq - yq)), xq * (xq * xq - yq) / 2);
        BOOST_CHECK_THROW((2 * x * (x - y)) / z, math::inexact_division);
        // This fails after the mapping back to multivariate.
        BOOST_CHECK_THROW((y * y + x * x * y * y * y) / x, math::inexact_division);
        // Random testing.
        std::uniform_int_distribution<int> dist(0, 9);
        for (int i = 0; i < ntrials; ++i) {
            auto n = rn_poly(x, y, z, dist), m = rn_poly(x, y, z, dist);
            if (m.size() == 0u) {
                BOOST_CHECK_THROW(n / m, zero_division_error);
            } else {
                BOOST_CHECK_EQUAL((n * m) / m, n);
                if (m != 1) {
                    BOOST_CHECK_THROW((n * m + 1) / m, math::inexact_division);
                }
                if (n.size() != 0u) {
                    BOOST_CHECK_EQUAL((n * m * n) / (m * n), n);
                }
                auto tmp = n * m;
                tmp /= m;
                BOOST_CHECK_EQUAL(tmp, n);
                tmp *= m;
                math::divexact(tmp, tmp, m);
                BOOST_CHECK_EQUAL(tmp, n);
            }
        }
        // With rationals as well.
        for (int i = 0; i < ntrials; ++i) {
            auto n = rq_poly(xq, yq, zq, dist), m = rq_poly(xq, yq, zq, dist);
            if (m.size() == 0u) {
                BOOST_CHECK_THROW(n / m, zero_division_error);
            } else {
                BOOST_CHECK_EQUAL((n * m) / m, n);
                if (m != 1) {
                    BOOST_CHECK_THROW((n * m + 1) / m, math::inexact_division);
                }
                if (n.size() != 0u) {
                    BOOST_CHECK_EQUAL((n * m * n) / (m * n), n);
                }
                auto tmp = n * m;
                tmp /= m;
                BOOST_CHECK_EQUAL(tmp, n);
                tmp *= m;
                math::divexact(tmp, tmp, m);
                BOOST_CHECK_EQUAL(tmp, n);
            }
        }
        // Check the type traits.
        BOOST_CHECK(is_divisible<p_type>::value);
        BOOST_CHECK(is_divisible<pq_type>::value);
        BOOST_CHECK(is_divisible_in_place<p_type>::value);
        BOOST_CHECK(is_divisible_in_place<pq_type>::value);
        BOOST_CHECK((!is_divisible_in_place<p_type const, p_type>::value));
        BOOST_CHECK((!is_divisible_in_place<pq_type const, pq_type>::value));
        BOOST_CHECK(has_exact_division<p_type>::value);
        BOOST_CHECK(has_exact_division<pq_type>::value);
        BOOST_CHECK(has_exact_ring_operations<p_type>::value);
        BOOST_CHECK(has_exact_ring_operations<pq_type>::value);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_division_test)
{
    init();
    boost::mpl::for_each<key_types>(division_tester());
    BOOST_CHECK((!has_exact_ring_operations<polynomial<double, k_monomial>>::value));
    BOOST_CHECK((!has_exact_division<polynomial<double, k_monomial>>::value));
    BOOST_CHECK((is_divisible<polynomial<double, k_monomial>>::value));
    BOOST_CHECK((is_divisible_in_place<polynomial<double, k_monomial>>::value));
    BOOST_CHECK((has_exact_ring_operations<polynomial<integer, monomial<rational>>>::value));
    BOOST_CHECK((!has_exact_division<polynomial<integer, monomial<rational>>>::value));
    BOOST_CHECK((is_divisible<polynomial<integer, monomial<rational>>>::value));
    BOOST_CHECK((is_divisible_in_place<polynomial<integer, monomial<rational>>>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_division_recursive_test)
{
    using p_type = polynomial<rational, k_monomial>;
    using pp_type = polynomial<p_type, k_monomial>;
    BOOST_CHECK(has_exact_ring_operations<p_type>::value);
    BOOST_CHECK(has_exact_ring_operations<pp_type>::value);
    BOOST_CHECK(is_divisible<p_type>::value);
    BOOST_CHECK(is_divisible<pp_type>::value);
    BOOST_CHECK(has_exact_division<p_type>::value);
    BOOST_CHECK(has_exact_division<pp_type>::value);
    // A couple of simple tests.
    pp_type x{"x"};
    p_type y{"y"}, z{"z"}, t{"t"};
    BOOST_CHECK_EQUAL((x * x * y * z) / x, x * y * z);
    BOOST_CHECK_THROW((x * y * z) / (x * x), math::inexact_division);
    BOOST_CHECK_EQUAL((x * x * y * z) / y, x * x * z);
    BOOST_CHECK_EQUAL((2 * x * z * (x - y) * (x * x - y)) / (z * (x - y)), 2 * x * (x * x - y));
    BOOST_CHECK_EQUAL(pp_type{} / (x * y * z), 0);
    BOOST_CHECK_EQUAL(pp_type{} / (x.pow(-1) * y * z), 0);
    BOOST_CHECK_EQUAL(pp_type{} / (x * y * z + z.pow(-1)), 0);
    // Random testing.
    std::uniform_int_distribution<int> dist(0, 9);
    for (int i = 0; i < ntrials; ++i) {
        auto n_ = rq_poly(y, z, t, dist), m_ = rq_poly(y, z, t, dist);
        auto d = dist(rng);
        auto n = (n_ * dist(rng) * x.pow(dist(rng))) / (d == 0 ? 1 : d);
        d = dist(rng);
        auto m = (m_ * dist(rng) * x.pow(dist(rng))) / (d == 0 ? 1 : d);
        if (m.size() == 0u) {
            BOOST_CHECK_THROW(n / m, zero_division_error);
        } else {
            BOOST_CHECK_EQUAL((n * m) / m, n);
            BOOST_CHECK_THROW((n * m + 1) / m, math::inexact_division);
            if (n.size() != 0u) {
                BOOST_CHECK_EQUAL((n * m * n) / (m * n), n);
            }
            auto tmp = n * m;
            tmp /= m;
            BOOST_CHECK_EQUAL(tmp, n);
            tmp *= m;
            math::divexact(tmp, tmp, m);
            BOOST_CHECK_EQUAL(tmp, n);
        }
    }
}

struct uprem_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        p_type x{"x"}, y{"y"};
        BOOST_CHECK_THROW(p_type::uprem(x + y, x + y), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::uprem(x, y), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::uprem(x, x - x), zero_division_error);
        BOOST_CHECK_EQUAL(p_type::uprem(x - x, x), 0);
        BOOST_CHECK_THROW(p_type::uprem(x, x.pow(2)), std::invalid_argument);
        BOOST_CHECK_EQUAL(p_type::uprem(x.pow(2), x), 0);
        BOOST_CHECK_EQUAL(p_type::uprem(x, x + 1), -1);
        // Check with negative expos.
        BOOST_CHECK_THROW(p_type::uprem(x, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::uprem(x, x.pow(-2) + x), std::invalid_argument);
        // Random testing.
        std::uniform_int_distribution<int> dist(0, 9);
        for (int i = 0; i < ntrials; ++i) {
            p_type n, d;
            auto nterms = dist(rng);
            for (int j = 0; j < nterms; ++j) {
                auto tmp = dist(rng);
                n += x.pow(dist(rng)) * (tmp % 2 ? tmp : -tmp);
            }
            nterms = dist(rng);
            for (int j = 0; j < nterms; ++j) {
                auto tmp = dist(rng);
                d += x.pow(dist(rng)) * (tmp % 2 ? tmp : -tmp);
            }
            if (n.get_symbol_set() != d.get_symbol_set() || n.get_symbol_set().size() != 1u
                || (n.degree() < d.degree() && n.size() != 0u)) {
                BOOST_CHECK_THROW(p_type::uprem(n, d), std::invalid_argument);
            } else if (d.size() == 0u) {
                BOOST_CHECK_THROW(p_type::uprem(n, d), zero_division_error);
            } else {
                BOOST_CHECK_NO_THROW(p_type::uprem(n, d));
            }
        }
    }
};

BOOST_AUTO_TEST_CASE(polynomial_uprem_test)
{
    boost::mpl::for_each<key_types>(uprem_tester());
}

struct content_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        p_type x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL(p_type{}.content(), 0);
        BOOST_CHECK_EQUAL((x - x).content(), 0);
        BOOST_CHECK_EQUAL(p_type{1}.content(), 1);
        BOOST_CHECK_EQUAL(p_type{2}.content(), 2);
        BOOST_CHECK_EQUAL(x.content(), 1);
        BOOST_CHECK_EQUAL((2 * x).content(), 2);
        BOOST_CHECK_EQUAL((12 * x + 9 * y).content(), 3);
        BOOST_CHECK_EQUAL((12 * x + 8 * y + 6 * x * y).content(), 2);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_content_test)
{
    boost::mpl::for_each<key_types>(content_tester());
}

struct pp_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        p_type x{"x"}, y{"y"};
        BOOST_CHECK_THROW(p_type{}.primitive_part(), zero_division_error);
        BOOST_CHECK_THROW((x - x).primitive_part(), zero_division_error);
        BOOST_CHECK_EQUAL(x.primitive_part(), x);
        BOOST_CHECK_EQUAL(p_type{1}.primitive_part(), 1);
        BOOST_CHECK_EQUAL(p_type{2}.primitive_part(), 1);
        BOOST_CHECK_EQUAL((2 * x).primitive_part(), x);
        BOOST_CHECK_EQUAL((12 * x + 9 * y).primitive_part(), 4 * x + 3 * y);
        BOOST_CHECK_EQUAL((12 * x + 8 * y + 6 * x * y).primitive_part(), 6 * x + 4 * y + 3 * x * y);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_pp_test)
{
    boost::mpl::for_each<key_types>(pp_tester());
}

struct gcd_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        using math::pow;
        BOOST_CHECK(has_gcd<p_type>::value);
        BOOST_CHECK(has_gcd3<p_type>::value);
        // Some zero tests.
        BOOST_CHECK_EQUAL(math::gcd(p_type{}, p_type{}), 0);
        p_type x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL(math::gcd(x, p_type{}), x);
        BOOST_CHECK_EQUAL(math::gcd(p_type{}, x), x);
        BOOST_CHECK_EQUAL(math::gcd(x - x, p_type{}), 0);
        BOOST_CHECK_EQUAL(math::gcd(x - x, y - y), 0);
        BOOST_CHECK_EQUAL(math::gcd(x - x + y - y, y - y), 0);
        // Negative exponents.
        BOOST_CHECK_THROW(math::gcd(p_type{1}, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(x.pow(-1), p_type{1}), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(x, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(x.pow(-1), x), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(x + y, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(x.pow(-1), x + y), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(x + y, y.pow(-1) + x), std::invalid_argument);
        BOOST_CHECK_THROW(math::gcd(y.pow(-1) + x, x + y), std::invalid_argument);
        // Negative exponents will work if one poly is zero.
        BOOST_CHECK_EQUAL(math::gcd(x.pow(-1), p_type{}), x.pow(-1));
        BOOST_CHECK_EQUAL(math::gcd(p_type{}, x.pow(-1)), x.pow(-1));
        BOOST_CHECK_EQUAL(math::gcd(y + x.pow(-1), p_type{}), y + x.pow(-1));
        BOOST_CHECK_EQUAL(math::gcd(p_type{}, y + x.pow(-1)), y + x.pow(-1));
        // Zerovariate tests.
        BOOST_CHECK_EQUAL(math::gcd(p_type{12}, p_type{9}), 3);
        BOOST_CHECK_EQUAL(math::gcd(p_type{0}, p_type{9}), 9);
        BOOST_CHECK_EQUAL(math::gcd(p_type{9}, p_type{0}), 9);
        // The test from the Geddes book.
        auto a = -30 * x.pow(3) * y + 90 * x * x * y * y + 15 * x * x - 60 * x * y + 45 * y * y;
        auto b = 100 * x * x * y - 140 * x * x - 250 * x * y * y + 350 * x * y - 150 * y * y * y + 210 * y * y;
        BOOST_CHECK(math::gcd(a, b) == -15 * y + 5 * x || -math::gcd(a, b) == -15 * y + 5 * x);
        // Random testing.
        std::uniform_int_distribution<int> dist(0, 4);
        for (int i = 0; i < ntrials; ++i) {
            auto n = rn_poly(x, y, z, dist), m = rn_poly(x, y, z, dist), r = rn_poly(x, y, z, dist);
            auto tup = p_type::gcd(n * r, m * n, true);
            const auto &g = std::get<0u>(tup);
            if (math::is_zero(m * n)) {
                BOOST_CHECK_EQUAL(g, n * r);
            } else if (math::is_zero(n * r)) {
                BOOST_CHECK_EQUAL(g, m * n);
            } else {
                BOOST_CHECK_NO_THROW((n * r) / g);
                BOOST_CHECK_NO_THROW((m * n) / g);
                BOOST_CHECK_EQUAL((n * r) / g, std::get<1u>(tup));
                BOOST_CHECK_EQUAL((m * n) / g, std::get<2u>(tup));
                auto inv_g = math::gcd(m * n, n * r);
                if (inv_g != g) {
                    BOOST_CHECK_EQUAL(g, -inv_g);
                }
            }
        }
        // Some explicit tests manually verified via sympy.
        auto explicit_check = [](const p_type &n1, const p_type &n2, const p_type &cmp) -> void {
            auto g = math::gcd(n1, n2);
            BOOST_CHECK(g == cmp || g == -cmp);
        };
        explicit_check(pow(x, 2) * pow(y, 2) - 2 * x * pow(y, 3) * pow(z, 8) + pow(x, 3) * y * y * pow(z, 4)
                           - 2 * pow(y, 3) * pow(z, 4),
                       pow(y, 3) * z - 2 * x * pow(z, 3) + x * pow(y, 3) * pow(z, 5) - 2 * pow(x, 2) * pow(z, 7),
                       1 + x * z * z * z * z);
        explicit_check(4 * pow(x, 3) * pow(y, 5) * pow(z, 5) + 8 * pow(x, 6) * pow(y, 5) * pow(z, 6)
                           - 2 * pow(x, 2) * pow(y, 4) * pow(z, 5) - 4 * pow(x, 5) * pow(y, 4) * pow(z, 6),
                       8 * pow(x, 4) * pow(y, 5) * pow(z, 3) - 2 * pow(x, 2) * pow(y, 5) * pow(z, 5)
                           - 2 * pow(x, 4) * pow(y, 4) * pow(z, 3) - 6 * pow(x, 5) * pow(y, 2) * pow(z, 4)
                           + 16 * pow(x, 7) * pow(y, 5) * pow(z, 4) - 4 * pow(x, 5) * pow(y, 5) * pow(z, 6)
                           - 12 * pow(x, 8) * pow(y, 2) * pow(z, 5) - 4 * pow(x, 7) * pow(y, 4) * pow(z, 4),
                       -4 * pow(x, 5) * pow(y, 2) * pow(z, 4) - 2 * pow(x, 2) * pow(y, 2) * pow(z, 3));
        explicit_check(
            -4 * pow(x, 5) * pow(y, 8) * pow(z, 4) + 3 * pow(x, 3) * pow(y, 6) * pow(z, 3)
                + 9 * pow(x, 2) * pow(y, 5) * pow(z, 3) - 6 * pow(x, 2) * pow(y, 6) * pow(z, 7)
                + 3 * pow(x, 4) * pow(y, 6) * pow(z, 7) - 2 * pow(x, 6) * pow(y, 5) * pow(z, 3)
                - 6 * pow(x, 5) * pow(y, 4) * pow(z, 3) - 4 * pow(x, 4) * pow(y, 8)
                + 4 * pow(x, 5) * pow(y, 5) * pow(z, 7) - 12 * pow(x, 3) * pow(y, 7)
                + 8 * pow(x, 3) * pow(y, 8) * pow(z, 4) - 2 * pow(x, 7) * pow(y, 5) * pow(z, 7),
            -6 * pow(x, 8) * y * pow(z, 3) - 12 * pow(x, 6) * pow(y, 4) + 9 * pow(x, 5) * pow(y, 2) * pow(z, 3),
            2 * pow(x, 5) * y * pow(z, 3) + 4 * pow(x, 3) * pow(y, 4) - 3 * pow(x, 2) * pow(y, 2) * pow(z, 3));
        explicit_check(
            -4 * pow(x, 3) * pow(y, 3) * pow(z, 6) - 12 * pow(x, 3) * pow(z, 3) + 16 * pow(x, 4) * pow(z, 7)
                + 8 * pow(x, 7) * y * pow(z, 4),
            4 * pow(x, 5) * pow(y, 4) * pow(z, 5) + 8 * pow(x, 2) * pow(y, 3) * pow(z, 8)
                - 2 * x * pow(y, 6) * pow(z, 7) + 3 * pow(x, 2) * pow(y, 3) * pow(z, 7) + 9 * pow(x, 2) * pow(z, 4)
                - 6 * pow(x, 6) * y * pow(z, 5) - 12 * pow(x, 3) * pow(z, 8) - 6 * x * pow(y, 3) * pow(z, 4),
            2 * pow(x, 5) * y * pow(z, 4) + 4 * pow(x, 2) * pow(z, 7) - x * pow(y, 3) * pow(z, 6) - 3 * x * pow(z, 3));
    }
};

BOOST_AUTO_TEST_CASE(polynomial_gcd_test)
{
    boost::mpl::for_each<key_types>(gcd_tester());
    // Check the type trait.
    BOOST_CHECK((has_gcd<polynomial<integer, k_monomial>>::value));
    BOOST_CHECK((!has_gcd<polynomial<integer, monomial<rational>>>::value));
    BOOST_CHECK((!has_gcd<polynomial<rational, k_monomial>>::value));
    BOOST_CHECK((!has_gcd<polynomial<double, k_monomial>>::value));
    BOOST_CHECK((has_gcd3<polynomial<integer, k_monomial>>::value));
    BOOST_CHECK((!has_gcd3<polynomial<integer, monomial<rational>>>::value));
    BOOST_CHECK((!has_gcd3<polynomial<rational, k_monomial>>::value));
    BOOST_CHECK((!has_gcd3<polynomial<double, k_monomial>>::value));
}

struct gcd_prs_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        using math::pow;
        // Set the default algorithm to PRS, so that it is used at all levels of the recursion.
        BOOST_CHECK(p_type::get_default_gcd_algorithm() == polynomial_gcd_algorithm::automatic);
        p_type::set_default_gcd_algorithm(polynomial_gcd_algorithm::prs_sr);
        auto gcd_f = [](const p_type &a, const p_type &b) { return std::get<0u>(p_type::gcd(a, b, false)); };
        auto gcd_check = [](const p_type &a, const p_type &b, const p_type &g) {
            try {
                // Here explicitly call the heuristic one, for comparison.
                auto ret = std::get<0u>(p_type::gcd(a, b, false, polynomial_gcd_algorithm::heuristic));
                BOOST_CHECK(ret == g || ret == -g);
            } catch (const detail::gcdheu_failure &) {
            }
        };
        // Some zero tests.
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, p_type{}), 0);
        p_type x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL(gcd_f(x, p_type{}), x);
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, x), x);
        BOOST_CHECK_EQUAL(gcd_f(x - x, p_type{}), 0);
        BOOST_CHECK_EQUAL(gcd_f(x - x, y - y), 0);
        BOOST_CHECK_EQUAL(gcd_f(x - x + y - y, y - y), 0);
        // Negative exponents.
        BOOST_CHECK_THROW(gcd_f(p_type{1}, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x.pow(-1), p_type{1}), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x.pow(-1), x), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x + y, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x.pow(-1), x + y), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x + y, y.pow(-1) + x), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(y.pow(-1) + x, x + y), std::invalid_argument);
        // Negative exponents will work if one poly is zero.
        BOOST_CHECK_EQUAL(gcd_f(x.pow(-1), p_type{}), x.pow(-1));
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, x.pow(-1)), x.pow(-1));
        BOOST_CHECK_EQUAL(gcd_f(y + x.pow(-1), p_type{}), y + x.pow(-1));
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, y + x.pow(-1)), y + x.pow(-1));
        // Zerovariate tests.
        BOOST_CHECK_EQUAL(gcd_f(p_type{12}, p_type{9}), 3);
        BOOST_CHECK_EQUAL(gcd_f(p_type{0}, p_type{9}), 9);
        BOOST_CHECK_EQUAL(gcd_f(p_type{9}, p_type{0}), 9);
        // The test from the Geddes book.
        auto a = -30 * x.pow(3) * y + 90 * x * x * y * y + 15 * x * x - 60 * x * y + 45 * y * y;
        auto b = 100 * x * x * y - 140 * x * x - 250 * x * y * y + 350 * x * y - 150 * y * y * y + 210 * y * y;
        BOOST_CHECK(gcd_f(a, b) == -15 * y + 5 * x || -gcd_f(a, b) == -15 * y + 5 * x);
        // Random testing.
        std::uniform_int_distribution<int> dist(0, 4);
        for (int i = 0; i < ntrials; ++i) {
            auto n = rn_poly(x, y, z, dist), m = rn_poly(x, y, z, dist), r = rn_poly(x, y, z, dist);
            auto tup_res = p_type::gcd(n * r, m * n, true);
            const auto &g = std::get<0u>(tup_res);
            if (!math::is_zero(g)) {
                BOOST_CHECK_EQUAL((n * r) / g, std::get<1u>(tup_res));
                BOOST_CHECK_EQUAL((m * n) / g, std::get<2u>(tup_res));
            }
            gcd_check(n * r, m * n, g);
            if (math::is_zero(m * n)) {
                BOOST_CHECK_EQUAL(g, n * r);
            } else if (math::is_zero(n * r)) {
                BOOST_CHECK_EQUAL(g, m * n);
            } else {
                BOOST_CHECK_NO_THROW((n * r) / g);
                BOOST_CHECK_NO_THROW((m * n) / g);
                auto inv_g = gcd_f(m * n, n * r);
                if (inv_g != g) {
                    BOOST_CHECK_EQUAL(g, -inv_g);
                }
            }
        }
        // Some explicit tests manually verified via sympy.
        auto explicit_check = [gcd_f](const p_type &n1, const p_type &n2, const p_type &cmp) -> void {
            auto g = gcd_f(n1, n2);
            BOOST_CHECK(g == cmp || g == -cmp);
        };
        explicit_check(pow(x, 2) * pow(y, 2) - 2 * x * pow(y, 3) * pow(z, 8) + pow(x, 3) * y * y * pow(z, 4)
                           - 2 * pow(y, 3) * pow(z, 4),
                       pow(y, 3) * z - 2 * x * pow(z, 3) + x * pow(y, 3) * pow(z, 5) - 2 * pow(x, 2) * pow(z, 7),
                       1 + x * z * z * z * z);
        explicit_check(4 * pow(x, 3) * pow(y, 5) * pow(z, 5) + 8 * pow(x, 6) * pow(y, 5) * pow(z, 6)
                           - 2 * pow(x, 2) * pow(y, 4) * pow(z, 5) - 4 * pow(x, 5) * pow(y, 4) * pow(z, 6),
                       8 * pow(x, 4) * pow(y, 5) * pow(z, 3) - 2 * pow(x, 2) * pow(y, 5) * pow(z, 5)
                           - 2 * pow(x, 4) * pow(y, 4) * pow(z, 3) - 6 * pow(x, 5) * pow(y, 2) * pow(z, 4)
                           + 16 * pow(x, 7) * pow(y, 5) * pow(z, 4) - 4 * pow(x, 5) * pow(y, 5) * pow(z, 6)
                           - 12 * pow(x, 8) * pow(y, 2) * pow(z, 5) - 4 * pow(x, 7) * pow(y, 4) * pow(z, 4),
                       -4 * pow(x, 5) * pow(y, 2) * pow(z, 4) - 2 * pow(x, 2) * pow(y, 2) * pow(z, 3));
        explicit_check(
            -4 * pow(x, 5) * pow(y, 8) * pow(z, 4) + 3 * pow(x, 3) * pow(y, 6) * pow(z, 3)
                + 9 * pow(x, 2) * pow(y, 5) * pow(z, 3) - 6 * pow(x, 2) * pow(y, 6) * pow(z, 7)
                + 3 * pow(x, 4) * pow(y, 6) * pow(z, 7) - 2 * pow(x, 6) * pow(y, 5) * pow(z, 3)
                - 6 * pow(x, 5) * pow(y, 4) * pow(z, 3) - 4 * pow(x, 4) * pow(y, 8)
                + 4 * pow(x, 5) * pow(y, 5) * pow(z, 7) - 12 * pow(x, 3) * pow(y, 7)
                + 8 * pow(x, 3) * pow(y, 8) * pow(z, 4) - 2 * pow(x, 7) * pow(y, 5) * pow(z, 7),
            -6 * pow(x, 8) * y * pow(z, 3) - 12 * pow(x, 6) * pow(y, 4) + 9 * pow(x, 5) * pow(y, 2) * pow(z, 3),
            2 * pow(x, 5) * y * pow(z, 3) + 4 * pow(x, 3) * pow(y, 4) - 3 * pow(x, 2) * pow(y, 2) * pow(z, 3));
        explicit_check(
            -4 * pow(x, 3) * pow(y, 3) * pow(z, 6) - 12 * pow(x, 3) * pow(z, 3) + 16 * pow(x, 4) * pow(z, 7)
                + 8 * pow(x, 7) * y * pow(z, 4),
            4 * pow(x, 5) * pow(y, 4) * pow(z, 5) + 8 * pow(x, 2) * pow(y, 3) * pow(z, 8)
                - 2 * x * pow(y, 6) * pow(z, 7) + 3 * pow(x, 2) * pow(y, 3) * pow(z, 7) + 9 * pow(x, 2) * pow(z, 4)
                - 6 * pow(x, 6) * y * pow(z, 5) - 12 * pow(x, 3) * pow(z, 8) - 6 * x * pow(y, 3) * pow(z, 4),
            2 * pow(x, 5) * y * pow(z, 4) + 4 * pow(x, 2) * pow(z, 7) - x * pow(y, 3) * pow(z, 6) - 3 * x * pow(z, 3));
        // Restore the automatic algorithm.
        p_type::reset_default_gcd_algorithm();
        BOOST_CHECK(p_type::get_default_gcd_algorithm() == polynomial_gcd_algorithm::automatic);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_gcd_prs_test)
{
    boost::mpl::for_each<key_types>(gcd_prs_tester());
}

struct gcd_heu_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using p_type = polynomial<integer, Key>;
        using math::pow;
        auto gcd_f = [](const p_type &a, const p_type &b) {
            return std::get<0u>(p_type::gcd(a, b, false, polynomial_gcd_algorithm::heuristic));
        };
        auto gcd_check = [](const p_type &a, const p_type &b, const p_type &g) {
            auto ret = std::get<0u>(p_type::gcd(a, b, false, polynomial_gcd_algorithm::prs_sr));
            BOOST_CHECK(ret == g || ret == -g);
        };
        // Some zero tests.
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, p_type{}), 0);
        p_type x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL(gcd_f(x, p_type{}), x);
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, x), x);
        BOOST_CHECK_EQUAL(gcd_f(x - x, p_type{}), 0);
        BOOST_CHECK_EQUAL(gcd_f(x - x, y - y), 0);
        BOOST_CHECK_EQUAL(gcd_f(x - x + y - y, y - y), 0);
        // Negative exponents.
        BOOST_CHECK_THROW(gcd_f(p_type{1}, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x.pow(-1), p_type{1}), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x.pow(-1), x), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x + y, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x.pow(-1), x + y), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(x + y, y.pow(-1) + x), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_f(y.pow(-1) + x, x + y), std::invalid_argument);
        // Negative exponents will work if one poly is zero.
        BOOST_CHECK_EQUAL(gcd_f(x.pow(-1), p_type{}), x.pow(-1));
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, x.pow(-1)), x.pow(-1));
        BOOST_CHECK_EQUAL(gcd_f(y + x.pow(-1), p_type{}), y + x.pow(-1));
        BOOST_CHECK_EQUAL(gcd_f(p_type{}, y + x.pow(-1)), y + x.pow(-1));
        // Zerovariate tests.
        BOOST_CHECK_EQUAL(gcd_f(p_type{12}, p_type{9}), 3);
        BOOST_CHECK_EQUAL(gcd_f(p_type{0}, p_type{9}), 9);
        BOOST_CHECK_EQUAL(gcd_f(p_type{9}, p_type{0}), 9);
        // The test from the Geddes book.
        auto a = -30 * x.pow(3) * y + 90 * x * x * y * y + 15 * x * x - 60 * x * y + 45 * y * y;
        auto b = 100 * x * x * y - 140 * x * x - 250 * x * y * y + 350 * x * y - 150 * y * y * y + 210 * y * y;
        BOOST_CHECK(gcd_f(a, b) == -15 * y + 5 * x || -gcd_f(a, b) == -15 * y + 5 * x);
        // Random testing.
        std::uniform_int_distribution<int> dist(0, 4);
        for (int i = 0; i < ntrials; ++i) {
            auto n = rn_poly(x, y, z, dist), m = rn_poly(x, y, z, dist), r = rn_poly(x, y, z, dist);
            auto tup_res = p_type::gcd(n * r, m * n, true, polynomial_gcd_algorithm::heuristic);
            const auto &g = std::get<0u>(tup_res);
            gcd_check(n * r, m * n, g);
            if (math::is_zero(m * n)) {
                BOOST_CHECK_EQUAL(g, n * r);
            } else if (math::is_zero(n * r)) {
                BOOST_CHECK_EQUAL(g, m * n);
            } else {
                BOOST_CHECK_EQUAL((n * r) / g, std::get<1u>(tup_res));
                BOOST_CHECK_EQUAL((m * n) / g, std::get<2u>(tup_res));
                auto inv_g = gcd_f(m * n, n * r);
                if (inv_g != g) {
                    BOOST_CHECK_EQUAL(g, -inv_g);
                }
            }
        }
        // Some explicit tests manually verified via sympy.
        auto explicit_check = [gcd_f](const p_type &n1, const p_type &n2, const p_type &cmp) -> void {
            auto g = gcd_f(n1, n2);
            BOOST_CHECK(g == cmp || g == -cmp);
        };
        explicit_check(pow(x, 2) * pow(y, 2) - 2 * x * pow(y, 3) * pow(z, 8) + pow(x, 3) * y * y * pow(z, 4)
                           - 2 * pow(y, 3) * pow(z, 4),
                       pow(y, 3) * z - 2 * x * pow(z, 3) + x * pow(y, 3) * pow(z, 5) - 2 * pow(x, 2) * pow(z, 7),
                       1 + x * z * z * z * z);
        explicit_check(4 * pow(x, 3) * pow(y, 5) * pow(z, 5) + 8 * pow(x, 6) * pow(y, 5) * pow(z, 6)
                           - 2 * pow(x, 2) * pow(y, 4) * pow(z, 5) - 4 * pow(x, 5) * pow(y, 4) * pow(z, 6),
                       8 * pow(x, 4) * pow(y, 5) * pow(z, 3) - 2 * pow(x, 2) * pow(y, 5) * pow(z, 5)
                           - 2 * pow(x, 4) * pow(y, 4) * pow(z, 3) - 6 * pow(x, 5) * pow(y, 2) * pow(z, 4)
                           + 16 * pow(x, 7) * pow(y, 5) * pow(z, 4) - 4 * pow(x, 5) * pow(y, 5) * pow(z, 6)
                           - 12 * pow(x, 8) * pow(y, 2) * pow(z, 5) - 4 * pow(x, 7) * pow(y, 4) * pow(z, 4),
                       -4 * pow(x, 5) * pow(y, 2) * pow(z, 4) - 2 * pow(x, 2) * pow(y, 2) * pow(z, 3));
        explicit_check(
            -4 * pow(x, 5) * pow(y, 8) * pow(z, 4) + 3 * pow(x, 3) * pow(y, 6) * pow(z, 3)
                + 9 * pow(x, 2) * pow(y, 5) * pow(z, 3) - 6 * pow(x, 2) * pow(y, 6) * pow(z, 7)
                + 3 * pow(x, 4) * pow(y, 6) * pow(z, 7) - 2 * pow(x, 6) * pow(y, 5) * pow(z, 3)
                - 6 * pow(x, 5) * pow(y, 4) * pow(z, 3) - 4 * pow(x, 4) * pow(y, 8)
                + 4 * pow(x, 5) * pow(y, 5) * pow(z, 7) - 12 * pow(x, 3) * pow(y, 7)
                + 8 * pow(x, 3) * pow(y, 8) * pow(z, 4) - 2 * pow(x, 7) * pow(y, 5) * pow(z, 7),
            -6 * pow(x, 8) * y * pow(z, 3) - 12 * pow(x, 6) * pow(y, 4) + 9 * pow(x, 5) * pow(y, 2) * pow(z, 3),
            2 * pow(x, 5) * y * pow(z, 3) + 4 * pow(x, 3) * pow(y, 4) - 3 * pow(x, 2) * pow(y, 2) * pow(z, 3));
        explicit_check(
            -4 * pow(x, 3) * pow(y, 3) * pow(z, 6) - 12 * pow(x, 3) * pow(z, 3) + 16 * pow(x, 4) * pow(z, 7)
                + 8 * pow(x, 7) * y * pow(z, 4),
            4 * pow(x, 5) * pow(y, 4) * pow(z, 5) + 8 * pow(x, 2) * pow(y, 3) * pow(z, 8)
                - 2 * x * pow(y, 6) * pow(z, 7) + 3 * pow(x, 2) * pow(y, 3) * pow(z, 7) + 9 * pow(x, 2) * pow(z, 4)
                - 6 * pow(x, 6) * y * pow(z, 5) - 12 * pow(x, 3) * pow(z, 8) - 6 * x * pow(y, 3) * pow(z, 4),
            2 * pow(x, 5) * y * pow(z, 4) + 4 * pow(x, 2) * pow(z, 7) - x * pow(y, 3) * pow(z, 6) - 3 * x * pow(z, 3));
    }
};

BOOST_AUTO_TEST_CASE(polynomial_gcd_heu_test)
{
    boost::mpl::for_each<key_types>(gcd_heu_tester());
    // Some misc tests specific to gcdheu.
    using p_type = polynomial<integer, k_monomial>;
    using math::pow;
    auto gcdheu = [](const p_type &a, const p_type &b) -> std::pair<bool, p_type> {
        try {
            return std::make_pair(false, std::move(std::get<0u>(detail::gcdheu_geddes(a, b).second)));
        } catch (const detail::gcdheu_failure &) {
            return std::make_pair(true, p_type{});
        }
    };
    p_type x{"x"}, y{"y"}, z{"z"};
    // A few simple checks.
    auto g_checker = [gcdheu](const p_type &a, const p_type &b, const p_type &c) {
        auto res = gcdheu(a, b);
        BOOST_CHECK(!res.first);
        BOOST_CHECK(res.second == c || -res.second == c);
    };
    g_checker(x, x, x);
    g_checker(x + y, x + y, x + y);
    g_checker(x + y + z, x + y + z, x + y + z);
    g_checker(2 * x + 4 * y + 6 * z, 3 * x + 6 * y + 9 * z, x + 2 * y + 3 * z);
    g_checker(-2 * x + 4 * y - 6 * z, 3 * x + 6 * y + 9 * z, p_type{1});
    g_checker(-2 * x + 4 * y - 6 * z, 3 * x - 6 * y + 9 * z, x - 2 * y + 3 * z);
    g_checker((x + y) * (x - y) * z, x * (2 * x - 2 * y) * y * z * z, z * (x - y));
    g_checker((x + y) * (x - y) * z, x * (2 * x - 2 * y) * y * z * z, z * (x - y));
    // The test slow with PRS_SR.
    auto n = 9 * x * pow(y, 8) + 5 * x * x * pow(y, 9) * pow(z, 7) + 9 * pow(x, 3) * pow(z, 3)
             + 5 * pow(x, 6) * pow(y, 8) * pow(z, 8) - 8 * pow(x, 8) * y * y * pow(z, 7)
             - 8 * pow(x, 7) * pow(y, 7) * pow(z, 5) - 8 * pow(x, 9) * pow(y, 7) * pow(z, 5);
    auto m = 5 * pow(x, 6) * pow(y, 5) * pow(z, 6) + 9 * pow(x, 4) * pow(y, 3) * pow(z, 8)
             + pow(x, 5) * pow(y, 5) * pow(z, 8) - 8 * pow(x, 9) * y * pow(z, 3) - 2 * pow(x, 5) * pow(y, 8) * pow(z, 5)
             - 2 * pow(x, 7) * pow(y, 9) * pow(z, 2) + 5 * pow(x, 9) * pow(y, 4) * pow(z, 9)
             - 8 * pow(x, 5) * pow(y, 7) * pow(z, 5);
    g_checker(n, m, x);
}

BOOST_AUTO_TEST_CASE(polynomial_height_test)
{
    {
        using p_type = polynomial<integer, k_monomial>;
        p_type x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL((x - y + 3 * z).height(), 3);
        BOOST_CHECK_EQUAL((-4 * x - y + 3 * z).height(), 4);
        BOOST_CHECK_EQUAL(p_type{}.height(), 0);
        BOOST_CHECK_EQUAL(p_type{-100}.height(), 100);
    }
    {
        using p_type = polynomial<rational, monomial<short>>;
        p_type x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK_EQUAL((x / 2 - y / 10 + 3 / 2_q * z).height(), 3 / 2_q);
        BOOST_CHECK_EQUAL((-4 / 5_q * x - y - 3 * z).height(), 3);
        BOOST_CHECK_EQUAL(p_type{}.height(), 0);
        BOOST_CHECK_EQUAL(p_type{-100 / 4_q}.height(), 25);
    }
}

// This was a specific GCD computation that was very slow before changing the heuristic GCD algorithm.
BOOST_AUTO_TEST_CASE(polynomial_gcd_bug_00_test)
{
    using p_type = polynomial<integer, k_monomial>;
    p_type x{"x"}, y{"y"}, z{"z"};
    auto a = -3 * x.pow(2) * y.pow(3) * z.pow(2) - x * z.pow(2);
    auto b = 4 * x.pow(3) * y * z.pow(2) - 3 * y.pow(3) - 3 * x * y * z.pow(2);
    auto c = -4 * z.pow(3) - 2 * x.pow(3) * y.pow(3) - 4 * x.pow(4) * y * z.pow(4);
    auto d = 3 * x.pow(4) * y.pow(3) * z.pow(2) - 2 * x.pow(3) * y.pow(4) * z.pow(3)
             - 4 * x.pow(3) * y.pow(2) * z.pow(2) - 2 * x.pow(3) * y * z.pow(4);
    auto g = math::gcd(a * d + b * c, b * d);
    BOOST_CHECK(g == y || g == -y);
}

// This failed due to a division by zero by the cofactors cf_p/cf_q in gcdheu. The divisibility
// test now also checks that the dividends are not zero.
// NOTE: this does not apply anymore since the most recent implementation of gcdheu, but let's keep it around.
BOOST_AUTO_TEST_CASE(polynomial_gcd_bug_01_test)
{
    using p_type = polynomial<integer, k_monomial>;
    p_type x{"x"}, y{"y"};
    BOOST_CHECK(math::gcd(-x + y, y) == 1 || math::gcd(-x + y, y) == -1);
    BOOST_CHECK(math::gcd(y, -x + y) == 1 || math::gcd(y, -x + y) == -1);
}

// This specific computation resulted in a bug in a previous gcdheu implementation.
BOOST_AUTO_TEST_CASE(polynomial_gcd_bug_02_test)
{
    using p_type = polynomial<integer, k_monomial>;
    p_type x{"x"}, y{"y"}, z{"z"};
    auto num = 12 * x.pow(6) * y.pow(7) * z.pow(3) + 12 * x.pow(3) * y.pow(5) * z.pow(4)
               - 3 * x.pow(4) * y.pow(9) * z.pow(3);
    auto den = 36 * x.pow(4) * y.pow(7) * z - 48 * x.pow(7) * y.pow(8) * z.pow(3) - 48 * x.pow(5) * y.pow(7) * z.pow(2)
               + 36 * x * y.pow(5) * z.pow(2) - 48 * x.pow(2) * y.pow(5) * z.pow(3)
               - 48 * x.pow(4) * y.pow(6) * z.pow(4) - 48 * x.pow(3) * y.pow(4) - 9 * x.pow(2) * y.pow(9) * z
               + 12 * x.pow(3) * y.pow(9) * z.pow(2) + 12 * x.pow(5) * y.pow(10) * z.pow(3) - 48 * y.pow(2) * z
               + 12 * x * y.pow(6);
    auto correct = -12 * y.pow(2) * z - 12 * x.pow(3) * y.pow(4) + 3 * x * y.pow(6);
    p_type::set_default_gcd_algorithm(polynomial_gcd_algorithm::prs_sr);
    auto prs = std::get<0u>(p_type::gcd(num, den, false));
    BOOST_CHECK(prs == correct || prs == -correct);
    p_type::set_default_gcd_algorithm(polynomial_gcd_algorithm::heuristic);
    auto heu = std::get<0u>(p_type::gcd(num, den, false));
    BOOST_CHECK(heu == correct || heu == -correct);
    p_type::set_default_gcd_algorithm(polynomial_gcd_algorithm::automatic);
}
