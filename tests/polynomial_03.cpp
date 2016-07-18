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

#define BOOST_TEST_MODULE polynomial_03_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>

#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

static std::mt19937 rng;
static const int ntrials = 300;

using cf_types = boost::mpl::vector<integer, rational>;
using key_types = boost::mpl::vector<monomial<short>, monomial<integer>, k_monomial>;

struct split_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            using p_type = polynomial<Cf, Key>;
            using pp_type = polynomial<p_type, Key>;
            p_type x{"x"}, y{"y"}, z{"z"};
            pp_type xx{"x"};
            BOOST_CHECK_THROW(p_type{}.split(), std::invalid_argument);
            BOOST_CHECK_THROW(x.split(), std::invalid_argument);
            BOOST_CHECK_EQUAL((2 * x + 3 * y).split(), 2 * xx + 3 * y);
            BOOST_CHECK_EQUAL((2 * x + 3 * y).split().join(), 2 * x + 3 * y);
            BOOST_CHECK((2 * x + 3 * y).split().get_symbol_set() == symbol_set{symbol{"x"}});
            BOOST_CHECK((2 * x + 3 * y).split().join() == 2 * x + 3 * y);
            BOOST_CHECK(
                ((2 * x + 3 * y).split()._container().begin()->m_cf.get_symbol_set() == symbol_set{symbol{"y"}}));
            BOOST_CHECK_EQUAL((2 * x * z + 3 * x * x * y - 6 * x * y * z).split(),
                              xx * (2 * z - 6 * y * z) + 3 * xx * xx * y);
            BOOST_CHECK_EQUAL((2 * x * z + 3 * x * x * y - 6 * x * y * z).split().join(),
                              2 * x * z + 3 * x * x * y - 6 * x * y * z);
            BOOST_CHECK((2 * x * z + 3 * x * x * y - 6 * x * y * z).split().get_symbol_set()
                        == symbol_set{symbol{"x"}});
            BOOST_CHECK(((2 * x * z + 3 * x * x * y - 6 * x * y * z).split()._container().begin()->m_cf.get_symbol_set()
                         == symbol_set{symbol{"y"}, symbol{"z"}}));
            BOOST_CHECK((std::is_same<decltype(x.split()), pp_type>::value));
            BOOST_CHECK((std::is_same<decltype(x.split().join()), p_type>::value));
            p_type null;
            null.set_symbol_set((x + y).get_symbol_set());
            BOOST_CHECK_EQUAL(null.split(), pp_type{});
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<key_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_split_join_test)
{
    init();
    boost::mpl::for_each<cf_types>(split_tester());
}

struct udivrem_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using piranha::math::pow;
        using p_type = polynomial<rational, Key>;
        p_type x{"x"}, y{"y"};
        // A few initial tests that can be checked manually.
        auto res = p_type::udivrem(x, x);
        BOOST_CHECK_EQUAL(res.first, 1);
        BOOST_CHECK_EQUAL(res.second, 0);
        res = p_type::udivrem(2 * x, x);
        BOOST_CHECK_EQUAL(res.first, 2);
        BOOST_CHECK_EQUAL(res.second, 0);
        res = p_type::udivrem(x, 2 * x);
        BOOST_CHECK_EQUAL(res.first, 1 / 2_q);
        BOOST_CHECK_EQUAL(res.second, 0);
        res = p_type::udivrem(pow(x, 3) - 2 * pow(x, 2) - 4, x - 3);
        BOOST_CHECK_EQUAL(res.first, pow(x, 2) + x + 3);
        BOOST_CHECK_EQUAL(res.second, 5);
        res = p_type::udivrem(pow(x, 8) + pow(x, 6) - 3 * pow(x, 4) - 3 * pow(x, 3) + 8 * pow(x, 2) + 2 * x - 5,
                              3 * pow(x, 6) + 5 * pow(x, 4) - 4 * x * x - 9 * x + 21);
        BOOST_CHECK_EQUAL(res.first, x * x / 3 - 2 / 9_q);
        BOOST_CHECK_EQUAL(res.second, -5 * x.pow(4) / 9 + x * x / 9 - 1 / 3_q);
        res = p_type::udivrem(x * x + 4 * x + 1, x + 2);
        BOOST_CHECK_EQUAL(res.first, 2 + x);
        BOOST_CHECK_EQUAL(res.second, -3);
        // With zero numerator.
        res = p_type::udivrem(x - x, x + 2);
        BOOST_CHECK_EQUAL(res.first, 0);
        BOOST_CHECK_EQUAL(res.second, 0);
        // With plain numbers
        res = p_type::udivrem(x - x - 3, x - x + 2);
        BOOST_CHECK_EQUAL(res.first, -3 / 2_q);
        BOOST_CHECK_EQUAL(res.second, 0);
        // Multivariate error.
        BOOST_CHECK_THROW(p_type::udivrem(x + y, x), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::udivrem(x + y - y, x), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::udivrem(x, x + y), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::udivrem(x + y, x + y), std::invalid_argument);
        // Zero division error.
        BOOST_CHECK_THROW(p_type::udivrem(x, x - x), zero_division_error);
        // Different univariate.
        BOOST_CHECK_THROW(p_type::udivrem(x, y), std::invalid_argument);
        // Negative exponents.
        BOOST_CHECK_THROW(p_type::udivrem(x, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::udivrem(x.pow(-1), x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(p_type::udivrem(x.pow(-1), x), std::invalid_argument);
        // Negative powers are allowed if the numerator is zero.
        res = p_type::udivrem((x - x), x.pow(-1));
        BOOST_CHECK_EQUAL(res.first.size(), 0u);
        BOOST_CHECK_EQUAL(res.second.size(), 0u);
        // Randomised testing.
        std::uniform_int_distribution<int> ud(0, 9);
        for (auto i = 0; i < ntrials; ++i) {
            // Little hack to make sure that the symbol set contains always x. It could be
            // that in the random loop num/den get inited to a constant.
            p_type num = x - x, den = num;
            const int nterms = ud(rng);
            for (int j = 0; j < nterms; ++j) {
                num += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
                den += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
            }
            if (den.size() == 0u) {
                den = x - x + 1;
            }
            auto tmp = p_type::udivrem(num, den);
            BOOST_CHECK_EQUAL(num, den * tmp.first + tmp.second);
        }
        // A recursive test.
        using pp_type = polynomial<p_type, Key>;
        pp_type xx{"x"};
        auto res2 = pp_type::udivrem(x * xx, xx);
        BOOST_CHECK_EQUAL(res2.first, x);
        BOOST_CHECK_EQUAL(res2.second.size(), 0u);
        BOOST_CHECK_THROW(pp_type::udivrem(x.pow(-1) * xx, xx), std::invalid_argument);
        res2 = pp_type::udivrem(xx - xx, x.pow(-1) * xx);
        BOOST_CHECK_EQUAL(res2.first.size(), 0u);
        BOOST_CHECK_EQUAL(res2.second.size(), 0u);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_udivrem_test)
{
    boost::mpl::for_each<key_types>(udivrem_tester());
}

struct gcd_prs_sr_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using namespace piranha::detail;
        using piranha::math::pow;
        using p_type = polynomial<integer, Key>;
        using pq_type = polynomial<rational, Key>;
        // Set the default algorithm to PRS, so it is used at all levels of the recursion.
        BOOST_CHECK(p_type::get_default_gcd_algorithm() == polynomial_gcd_algorithm::automatic);
        p_type::set_default_gcd_algorithm(polynomial_gcd_algorithm::prs_sr);
        p_type x{"x"};
        // Some known tests.
        BOOST_CHECK_EQUAL(x, gcd_prs_sr(x, x));
        BOOST_CHECK_EQUAL(1 + x, gcd_prs_sr(pow(x, 2) + 7 * x + 6, pow(x, 2) - 5 * x - 6));
        BOOST_CHECK_EQUAL(gcd_prs_sr(pow(x, 8) + pow(x, 6) - 3 * pow(x, 4) - 3 * pow(x, 3) + 8 * pow(x, 2) + 2 * x - 5,
                                     3 * pow(x, 6) + 5 * pow(x, 4) - 4 * pow(x, 2) - 9 * x + 21),
                          1);
        BOOST_CHECK_EQUAL(gcd_prs_sr(pow(x, 4) - 9 * pow(x, 2) - 4 * x + 12, pow(x, 3) + 5 * pow(x, 2) + 2 * x - 8),
                          -2 + x * x + x);
        BOOST_CHECK_EQUAL(1, gcd_prs_sr(pow(x, 4) + pow(x, 2) + 1, pow(x, 2) + 1));
        BOOST_CHECK_EQUAL(gcd_prs_sr(x * x + 1, pow(x, 5) + pow(x, 4) + x + 1), 1);
        BOOST_CHECK_EQUAL(gcd_prs_sr(pow(x, 6) + pow(x, 5) + pow(x, 3) + x, pow(x, 4) + pow(x, 2) + 1), 1);
        // With zeroes.
        BOOST_CHECK_EQUAL(x + 1, gcd_prs_sr(x - x, x + 1));
        BOOST_CHECK_EQUAL(x + 1, gcd_prs_sr(x + 1, x - x));
        BOOST_CHECK_EQUAL(0, gcd_prs_sr(x - x, x - x));
        // With negative exponents.
        BOOST_CHECK_THROW(gcd_prs_sr(x.pow(-1), x), std::invalid_argument);
        BOOST_CHECK_THROW(gcd_prs_sr(x, x.pow(-1)), std::invalid_argument);
        // Random testing.
        std::uniform_int_distribution<int> ud(0, 9);
        for (auto i = 0; i < ntrials; ++i) {
            p_type a = x - x, b = a;
            const int nterms = ud(rng);
            for (int j = 0; j < nterms; ++j) {
                a += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
                b += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
            }
            auto tmp = gcd_prs_sr(a, b);
            if (a.size() != 0u || b.size() != 0u) {
                BOOST_CHECK_EQUAL(pq_type::udivrem(pq_type(a), pq_type(tmp)).second.size(), 0u);
                BOOST_CHECK_EQUAL(pq_type::udivrem(pq_type(b), pq_type(tmp)).second.size(), 0u);
            }
            BOOST_CHECK(tmp == gcd_prs_sr(b, a) || tmp == -gcd_prs_sr(b, a));
        }
        for (auto i = 0; i < ntrials; ++i) {
            p_type a = x - x, b = a;
            const int nterms = ud(rng);
            for (int j = 0; j < nterms; ++j) {
                a += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
                b += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
            }
            auto tmp = gcd_prs_sr(a * b, b);
            if ((a * b).size() != 0u || b.size() != 0u) {
                BOOST_CHECK_EQUAL(pq_type::udivrem(pq_type(a * b), pq_type(tmp)).second.size(), 0u);
                BOOST_CHECK_EQUAL(pq_type::udivrem(pq_type(b), pq_type(tmp)).second.size(), 0u);
            }
            BOOST_CHECK(tmp == gcd_prs_sr(b, a * b) || tmp == -gcd_prs_sr(b, a * b));
            tmp = gcd_prs_sr(a * b * b, b * a);
            if ((a * b * b).size() != 0u || (b * a).size() != 0u) {
                BOOST_CHECK_EQUAL(pq_type::udivrem(pq_type(a * b * b), pq_type(tmp)).second.size(), 0u);
                BOOST_CHECK_EQUAL(pq_type::udivrem(pq_type(b * a), pq_type(tmp)).second.size(), 0u);
            }
            BOOST_CHECK(tmp == gcd_prs_sr(a * b, a * b * b) || tmp == -gcd_prs_sr(a * b, a * b * b));
        }
        // Restore.
        p_type::reset_default_gcd_algorithm();
        BOOST_CHECK(p_type::get_default_gcd_algorithm() == polynomial_gcd_algorithm::automatic);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_gcd_prs_sr_test)
{
    boost::mpl::for_each<key_types>(gcd_prs_sr_tester());
}

struct establish_limits_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using namespace piranha::detail;
        using p_type = polynomial<integer, Key>;
        p_type x{"x"}, y{"y"};
        auto lims = poly_establish_limits(x + y, x + y);
        BOOST_CHECK_EQUAL(lims.size(), 2u);
        BOOST_CHECK_EQUAL(lims[0].first, 0);
        BOOST_CHECK_EQUAL(lims[0].second, 1);
        BOOST_CHECK_EQUAL(lims[1].first, 0);
        BOOST_CHECK_EQUAL(lims[1].second, 1);
        lims = poly_establish_limits(x + y - x, x + y - x + y * y);
        BOOST_CHECK_EQUAL(lims.size(), 2u);
        BOOST_CHECK_EQUAL(lims[0].first, 0);
        BOOST_CHECK_EQUAL(lims[0].second, 0);
        BOOST_CHECK_EQUAL(lims[1].first, 1);
        BOOST_CHECK_EQUAL(lims[1].second, 2);
        lims = poly_establish_limits(p_type{1}, p_type{2});
        BOOST_CHECK_EQUAL(lims.size(), 0u);
        lims = poly_establish_limits(x + 1 - x, x + 2 - x);
        BOOST_CHECK_EQUAL(lims.size(), 1u);
        BOOST_CHECK_EQUAL(lims[0].first, 0);
        BOOST_CHECK_EQUAL(lims[0].second, 0);
        lims = poly_establish_limits(x + y - y, y + x.pow(4));
        BOOST_CHECK_EQUAL(lims.size(), 2u);
        BOOST_CHECK_EQUAL(lims[0].first, 0);
        BOOST_CHECK_EQUAL(lims[0].second, 4);
        BOOST_CHECK_EQUAL(lims[1].first, 0);
        BOOST_CHECK_EQUAL(lims[1].second, 1);
        BOOST_CHECK_THROW(poly_establish_limits(x + y.pow(-1), y + x.pow(4)), std::invalid_argument);
        // Try with zero variables.
        p_type a{1}, b{2};
        lims = poly_establish_limits(a, b);
        BOOST_CHECK_EQUAL(lims.size(), 0u);
    }
};

BOOST_AUTO_TEST_CASE(polynomial_establish_limits_test)
{
    boost::mpl::for_each<key_types>(establish_limits_tester());
}

struct mapping_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using namespace piranha::detail;
        using p_type = polynomial<integer, Key>;
        using kp_type = polynomial<integer, k_monomial>;
        // Start with some basic checks.
        p_type x{"x"}, y{"y"}, z{"z"};
        kp_type kx{"x"};
        auto res = poly_to_univariate(2 * x, x * x);
        BOOST_CHECK_EQUAL(std::get<0u>(res).size(), 1u);
        BOOST_CHECK(std::get<0u>(res).get_symbol_set() == x.get_symbol_set());
        BOOST_CHECK_EQUAL(std::get<0u>(res)._container().begin()->m_cf, 2);
        BOOST_CHECK_EQUAL(std::get<0u>(res)._container().begin()->m_key.get_int(), 1);
        BOOST_CHECK_EQUAL(std::get<1u>(res).size(), 1u);
        BOOST_CHECK(std::get<1u>(res).get_symbol_set() == x.get_symbol_set());
        BOOST_CHECK_EQUAL(std::get<1u>(res)._container().begin()->m_cf, 1);
        BOOST_CHECK_EQUAL(std::get<1u>(res)._container().begin()->m_key.get_int(), 2);
        BOOST_CHECK_EQUAL(std::get<2u>(res).size(), 2u);
        BOOST_CHECK_EQUAL(std::get<2u>(res)[0u], 1);
        BOOST_CHECK_EQUAL(std::get<2u>(res)[1u], 3);
        // Decodification.
        auto dres = poly_from_univariate<Key>(std::get<0u>(res), std::get<2u>(res), symbol_set({symbol{"x"}}));
        BOOST_CHECK_EQUAL(dres, 2 * x);
        // Check the throwing with negative powers.
        BOOST_CHECK_THROW(poly_to_univariate(y, y.pow(-1)), std::invalid_argument);
        // Another simple example to check by hand.
        res = poly_to_univariate(2 * x * x + y * y, x * x * x - y);
        BOOST_CHECK_EQUAL(std::get<2u>(res).size(), 3u);
        BOOST_CHECK_EQUAL(std::get<2u>(res)[0u], 1);
        BOOST_CHECK_EQUAL(std::get<2u>(res)[1u], 4);
        BOOST_CHECK_EQUAL(std::get<2u>(res)[2u], 12);
        BOOST_CHECK_EQUAL(std::get<0u>(res).size(), 2u);
        BOOST_CHECK_EQUAL(std::get<0u>(res).get_symbol_set().size(), 1u);
        BOOST_CHECK_EQUAL(*std::get<0u>(res).get_symbol_set().begin(), symbol("x"));
        BOOST_CHECK_EQUAL(std::get<0u>(res), 2 * kx * kx + kx.pow(8));
        BOOST_CHECK_EQUAL(std::get<1u>(res).size(), 2u);
        BOOST_CHECK_EQUAL(std::get<1u>(res).get_symbol_set().size(), 1u);
        BOOST_CHECK_EQUAL(*std::get<1u>(res).get_symbol_set().begin(), symbol("x"));
        BOOST_CHECK_EQUAL(std::get<1u>(res), kx * kx * kx - kx.pow(4));
        overflow_check(x, y);
        dres = poly_from_univariate<Key>(std::get<0u>(res), std::get<2u>(res), symbol_set({symbol{"x"}, symbol{"y"}}));
        BOOST_CHECK_EQUAL(dres, 2 * x * x + y * y);
        // Random checking.
        std::uniform_int_distribution<int> ud(0, 9);
        for (int i = 0; i < ntrials; ++i) {
            symbol_set ss({symbol{"x"}, symbol{"y"}, symbol{"z"}});
            // Generate two random polys.
            p_type n, d;
            for (int j = 0; j < 10; ++j) {
                n += x.pow(ud(rng)) * y.pow(ud(rng)) * z.pow(ud(rng));
                d += x.pow(ud(rng)) * y.pow(ud(rng)) * z.pow(ud(rng));
            }
            // In these unlikely cases, skip the iteration.
            if (n.get_symbol_set().size() != 3u || n.get_symbol_set() != d.get_symbol_set() || n.size() == 0u
                || d.size() == 0u) {
                continue;
            }
            res = poly_to_univariate(n, d);
            BOOST_CHECK_EQUAL(std::get<0u>(res).size(), n.size());
            BOOST_CHECK(std::get<0u>(res).get_symbol_set() == symbol_set({symbol{"x"}}));
            BOOST_CHECK_EQUAL(std::get<1u>(res).size(), d.size());
            BOOST_CHECK(std::get<1u>(res).get_symbol_set() == symbol_set({symbol{"x"}}));
            BOOST_CHECK_EQUAL(std::get<2u>(res).size(), 4u);
            dres = poly_from_univariate<Key>(std::get<0u>(res), std::get<2u>(res), ss);
            BOOST_CHECK_EQUAL(dres, n);
        }
    }
    template <
        typename T,
        typename std::enable_if<detail::is_mp_integer<typename T::term_type::key_type::value_type>::value, int>::type
        = 0>
    void overflow_check(const T &x, const T &y) const
    {
        BOOST_CHECK_THROW(poly_to_univariate(x.pow(std::numeric_limits<k_monomial::value_type>::max()), x),
                          std::overflow_error);
        BOOST_CHECK_THROW(poly_to_univariate(x.pow(std::numeric_limits<k_monomial::value_type>::max() / 2)
                                                 + y.pow(std::numeric_limits<k_monomial::value_type>::max() / 2),
                                             x + y),
                          std::overflow_error);
    }
    template <
        typename T,
        typename std::enable_if<!detail::is_mp_integer<typename T::term_type::key_type::value_type>::value, int>::type
        = 0>
    void overflow_check(const T &, const T &) const
    {
    }
};

BOOST_AUTO_TEST_CASE(polynomial_mapping_test)
{
    boost::mpl::for_each<key_types>(mapping_tester());
}

struct univariate_gcdheu_tester {
    template <typename Key>
    void operator()(const Key &)
    {
        using namespace piranha::detail;
        using piranha::math::pow;
        using p_type = polynomial<integer, Key>;
        p_type x{"x"};
        // The idea here is that instead of calling the detail:: implementation we go through
        // the gcd() static function in poly and force the algorithm. This is in order to test
        // the exception throwing paths in the static method.
        auto heu_wrapper = [](const p_type &a, const p_type &b) -> std::pair<bool, p_type> {
            try {
                auto res_heu = p_type::gcd(a, b, true, polynomial_gcd_algorithm::heuristic);
                if (!math::is_zero(std::get<0u>(res_heu))) {
                    BOOST_CHECK_EQUAL(a / std::get<0u>(res_heu), std::get<1u>(res_heu));
                    BOOST_CHECK_EQUAL(b / std::get<0u>(res_heu), std::get<2u>(res_heu));
                }
                return std::make_pair(false, std::move(std::get<0u>(res_heu)));
            } catch (const std::runtime_error &) {
                return std::make_pair(true, p_type{});
            }
        };
        auto gcd_check = [heu_wrapper](const p_type &a, const p_type &b, const p_type &g) {
            auto res_heu = heu_wrapper(a, b);
            BOOST_CHECK(!res_heu.first);
            BOOST_CHECK(res_heu.second == g || res_heu.second == -g);
        };
        // Some known tests.
        gcd_check(x, x, x);
        gcd_check(pow(x, 2) + 7 * x + 6, pow(x, 2) - 5 * x - 6, 1 + x);
        gcd_check(pow(x, 8) + pow(x, 6) - 3 * pow(x, 4) - 3 * pow(x, 3) + 8 * pow(x, 2) + 2 * x - 5,
                  3 * pow(x, 6) + 5 * pow(x, 4) - 4 * pow(x, 2) - 9 * x + 21, p_type{1});
        gcd_check(pow(x, 4) - 9 * pow(x, 2) - 4 * x + 12, pow(x, 3) + 5 * pow(x, 2) + 2 * x - 8, -2 + x * x + x);
        gcd_check(pow(x, 4) + pow(x, 2) + 1, pow(x, 2) + 1, p_type{1});
        gcd_check(x * x + 1, pow(x, 5) + pow(x, 4) + x + 1, p_type{1});
        gcd_check(pow(x, 6) + pow(x, 5) + pow(x, 3) + x, pow(x, 4) + pow(x, 2) + 1, p_type{1});
        // With zeroes.
        gcd_check(x - x, x + 1, x + 1);
        gcd_check(x + 1, x - x, x + 1);
        gcd_check(x - x, x - x, p_type{});
        // With constants.
        gcd_check(p_type{}, p_type{}, p_type{});
        gcd_check(p_type{}, p_type{3}, p_type{3});
        gcd_check(p_type{3}, p_type{}, p_type{3});
        gcd_check(p_type{9}, p_type{12}, p_type{3});
        gcd_check(p_type{-24}, p_type{30}, p_type{-6});
        // With negative exponents.
        BOOST_CHECK_THROW(heu_wrapper(x.pow(-1), x), std::invalid_argument);
        BOOST_CHECK_THROW(heu_wrapper(x, x.pow(-1)), std::invalid_argument);
        BOOST_CHECK_THROW(heu_wrapper(x.pow(-1), x.pow(-1)), std::invalid_argument);
        // Random testing.
        std::uniform_int_distribution<int> ud(0, 9);
        for (auto i = 0; i < ntrials; ++i) {
            p_type a = x - x, b = a;
            const int nterms = ud(rng);
            for (int j = 0; j < nterms; ++j) {
                a += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
                b += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
            }
            auto res_heu = heu_wrapper(a, b);
            if (res_heu.first) {
                continue;
            }
            if (a.size() != 0u || b.size() != 0u) {
                BOOST_CHECK_EQUAL(p_type::udivrem(a, res_heu.second).second.size(), 0u);
                BOOST_CHECK_EQUAL(p_type::udivrem(b, res_heu.second).second.size(), 0u);
            }
            auto res_heu2 = heu_wrapper(b, a);
            BOOST_CHECK(!res_heu2.first);
            BOOST_CHECK(res_heu.second == res_heu2.second || res_heu.second == -res_heu2.second);
            auto res_prs = gcd_prs_sr(a, b);
            BOOST_CHECK(res_heu.second == res_prs || res_heu.second == -res_prs);
        }
        for (auto i = 0; i < ntrials; ++i) {
            p_type a = x - x, b = a;
            const int nterms = ud(rng);
            for (int j = 0; j < nterms; ++j) {
                a += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
                b += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
            }
            auto res_heu = heu_wrapper(a * b, b);
            if (res_heu.first) {
                continue;
            }
            if ((a * b).size() != 0u || b.size() != 0u) {
                BOOST_CHECK_EQUAL(p_type::udivrem(a * b, res_heu.second).second.size(), 0u);
                BOOST_CHECK_EQUAL(p_type::udivrem(b, res_heu.second).second.size(), 0u);
            }
            auto res_heu2 = heu_wrapper(b, a * b);
            BOOST_CHECK(!res_heu2.first);
            BOOST_CHECK(res_heu.second == res_heu2.second || res_heu.second == -res_heu2.second);
            auto res_prs = gcd_prs_sr(a * b, b);
            BOOST_CHECK(res_heu.second == res_prs || res_heu.second == -res_prs);
            res_heu = heu_wrapper(a * b * b, b * a);
            if (res_heu.first) {
                continue;
            }
            if ((a * b * b).size() != 0u || (b * a).size() != 0u) {
                BOOST_CHECK_EQUAL(p_type::udivrem(a * b * b, res_heu.second).second.size(), 0u);
                BOOST_CHECK_EQUAL(p_type::udivrem(b * a, res_heu.second).second.size(), 0u);
            }
            res_heu2 = heu_wrapper(b * a, a * b * b);
            BOOST_CHECK(!res_heu2.first);
            BOOST_CHECK(res_heu.second == res_heu2.second || res_heu.second == -res_heu2.second);
            res_prs = gcd_prs_sr(a * b * b, b * a);
            BOOST_CHECK(res_heu.second == res_prs || res_heu.second == -res_prs);
        }
    }
};

BOOST_AUTO_TEST_CASE(polynomial_univariate_gcdheu_test)
{
    boost::mpl::for_each<key_types>(univariate_gcdheu_tester());
}
