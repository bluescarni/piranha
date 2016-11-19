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

#define BOOST_TEST_MODULE polynomial_multiplier_01_test
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "../src/init.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

using cf_types = boost::mpl::vector<double, rational>;
using k_types = boost::mpl::vector<monomial<int>, monomial<rational>,
                                   // This should ensure in the overflow tests below we have enough bit width (we
                                   // are testing with 3 variables).
                                   kronecker_monomial<std::int_least64_t>>;

struct bounds_tester {
    template <typename Cf>
    struct runner {
        template <typename Key, typename std::enable_if<detail::is_monomial<Key>::value
                                                            && !std::is_integral<typename Key::value_type>::value,
                                                        int>::type
                                = 0>
        void operator()(const Key &)
        {
        }
        template <typename Key, typename std::enable_if<detail::is_monomial<Key>::value
                                                            && std::is_integral<typename Key::value_type>::value,
                                                        int>::type
                                = 0>
        void operator()(const Key &)
        {
            settings::set_min_work_per_thread(1u);
            for (unsigned nt = 1u; nt <= 20u; ++nt) {
                using pt = polynomial<Cf, Key>;
                using int_type = typename Key::value_type;
                settings::set_n_threads(nt);
                auto x = pt{"x"}, y = pt{"y"};
                BOOST_CHECK_THROW(math::pow(x, std::numeric_limits<int_type>::max()) * x, std::overflow_error);
                BOOST_CHECK_THROW((math::pow(x, std::numeric_limits<int_type>::max()) + 1) * (x + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(math::pow(x, std::numeric_limits<int_type>::min()) * x.pow(-1), std::overflow_error);
                BOOST_CHECK_THROW((math::pow(x, std::numeric_limits<int_type>::min()) + 1) * (x.pow(-1) + 1),
                                  std::overflow_error);
                BOOST_CHECK_EQUAL(math::pow(x, std::numeric_limits<int_type>::max() - 1) * x,
                                  math::pow(x, std::numeric_limits<int_type>::max()));
                BOOST_CHECK_EQUAL(math::pow(x, std::numeric_limits<int_type>::min() + 1) * x.pow(-1),
                                  math::pow(x, std::numeric_limits<int_type>::min()));
                // Try also with more than one variable.
                BOOST_CHECK_THROW(x * math::pow(y, std::numeric_limits<int_type>::max()) * y, std::overflow_error);
                BOOST_CHECK_THROW((x + 1) * (math::pow(y, std::numeric_limits<int_type>::max()) * y + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(math::pow(x, std::numeric_limits<int_type>::max())
                                      * math::pow(y, std::numeric_limits<int_type>::min()) * y.pow(-1),
                                  std::overflow_error);
                BOOST_CHECK_THROW((math::pow(x, std::numeric_limits<int_type>::max()) + 1)
                                      * (math::pow(y, std::numeric_limits<int_type>::min()) * y.pow(-1) + 1),
                                  std::overflow_error);
                BOOST_CHECK_EQUAL(math::pow(y, std::numeric_limits<int_type>::max())
                                      * math::pow(x, std::numeric_limits<int_type>::max() - 1) * x,
                                  math::pow(y, std::numeric_limits<int_type>::max())
                                      * math::pow(x, std::numeric_limits<int_type>::max()));
                BOOST_CHECK_EQUAL(math::pow(y, std::numeric_limits<int_type>::min())
                                      * math::pow(x, std::numeric_limits<int_type>::min() + 1) * x.pow(-1),
                                  math::pow(y, std::numeric_limits<int_type>::min())
                                      * math::pow(x, std::numeric_limits<int_type>::min()));
                // Check with empty series.
                BOOST_CHECK_EQUAL(math::pow(y, std::numeric_limits<int_type>::max()) * 0, 0);
                BOOST_CHECK_EQUAL(math::pow(y, std::numeric_limits<int_type>::min()) * 0, 0);
                BOOST_CHECK_EQUAL(pt(0) * pt(0), 0);
                // Check with constant polys.
                BOOST_CHECK_EQUAL(pt{2} * pt{3}, 6);
            }
            settings::reset_min_work_per_thread();
            settings::reset_n_threads();
        }
        template <typename Key, typename std::enable_if<detail::is_kronecker_monomial<Key>::value, int>::type = 0>
        void operator()(const Key &)
        {
            settings::set_min_work_per_thread(1u);
            for (unsigned nt = 1u; nt <= 20u; ++nt) {
                using pt = polynomial<Cf, Key>;
                using value_type = typename Key::value_type;
                using ka = kronecker_array<value_type>;
                settings::set_n_threads(nt);
                // Use polynomials with 3 variables for testing.
                const auto &limits = std::get<0u>(ka::get_limits()[3u]);
                auto x = pt{"x"}, y = pt{"y"}, z = pt{"z"};
                BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * x, std::overflow_error);
                BOOST_CHECK_THROW((x.pow(limits[0u]) + 1) * (y.pow(limits[1u]) + 1) * (z.pow(limits[2u]) + 1) * (x + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * y, std::overflow_error);
                BOOST_CHECK_THROW((x.pow(limits[0u]) + 1) * (y.pow(limits[1u]) + 1) * (z.pow(limits[2u]) + 1) * (y + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * z, std::overflow_error);
                BOOST_CHECK_THROW((x.pow(limits[0u]) + 1) * (y.pow(limits[1u]) + 1) * (z.pow(limits[2u]) + 1) * (z + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(x.pow(-limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * x.pow(-1),
                                  std::overflow_error);
                BOOST_CHECK_THROW((x.pow(-limits[0u]) + 1) * (y.pow(limits[1u]) + 1) * (z.pow(limits[2u]) + 1)
                                      * (x.pow(-1) + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(-limits[1u]) * z.pow(limits[2u]) * y.pow(-1),
                                  std::overflow_error);
                BOOST_CHECK_THROW((x.pow(limits[0u]) + 1) * (y.pow(-limits[1u]) + 1) * (z.pow(limits[2u]) + 1)
                                      * (y.pow(-1) + 1),
                                  std::overflow_error);
                BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(-limits[2u]) * z.pow(-1),
                                  std::overflow_error);
                BOOST_CHECK_THROW((x.pow(limits[0u]) + 1) * (y.pow(limits[1u]) + 1) * (z.pow(-limits[2u]) + 1)
                                      * (z.pow(-1) + 1),
                                  std::overflow_error);
                BOOST_CHECK_EQUAL(x.pow(limits[0u] - 1) * y.pow(limits[1u]) * z.pow(limits[2u]) * x,
                                  x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]));
                BOOST_CHECK_EQUAL(x.pow(limits[0u]) * y.pow(limits[1u] - 1) * z.pow(limits[2u]) * y,
                                  x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]));
                BOOST_CHECK_EQUAL(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u] - 1) * z,
                                  x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]));
                BOOST_CHECK_EQUAL(x.pow(-limits[0u] + 1) * y.pow(-limits[1u]) * z.pow(-limits[2u]) * x.pow(-1),
                                  x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u]));
                BOOST_CHECK_EQUAL(x.pow(-limits[0u]) * y.pow(-limits[1u] + 1) * z.pow(-limits[2u]) * y.pow(-1),
                                  x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u]));
                BOOST_CHECK_EQUAL(x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u] + 1) * z.pow(-1),
                                  x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u]));
                // Check with constant polys.
                BOOST_CHECK_EQUAL(pt{2} * pt{3}, 6);
            }
            settings::reset_min_work_per_thread();
            settings::reset_n_threads();
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<k_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_bounds_test)
{
    init();
    boost::mpl::for_each<cf_types>(bounds_tester());
}

struct multiplication_tester {
    template <typename Cf>
    struct runner {
        template <typename Key>
        void operator()(const Key &)
        {
            using pt = polynomial<Cf, Key>;
            // First a test with empty series.
            pt e1, e2;
            BOOST_CHECK_EQUAL(e1 * e2, 0);
            BOOST_CHECK_EQUAL((e1 * e2).get_symbol_set().size(), 0u);
            pt x{"x"};
            BOOST_CHECK_EQUAL(e1 * x, 0);
            BOOST_CHECK_EQUAL(x * e1, 0);
            BOOST_CHECK((x * e1).get_symbol_set() == symbol_set{symbol{"x"}});
            BOOST_CHECK((e1 * x).get_symbol_set() == symbol_set{symbol{"x"}});
            // A reduced fateman benchmark.
            pt y{"y"}, z{"z"}, t{"t"};
            auto f = 1 + x + y + z + t;
            auto tmp2 = f;
            for (int i = 1; i < 10; ++i) {
                f *= tmp2;
            }
            auto g = f + 1;
            auto retval = f * g;
            BOOST_CHECK_EQUAL(retval.size(), 10626u);
            // NOTE: this test is going to be exact in case of coefficients cancellations with double
            // precision coefficients only if the platform has ieee 754 format (integer exactly representable
            // as doubles up to 2 ** 53).
            // NOTE: no check on radix needed, 2 is the minimum value.
            if (std::is_same<Cf, double>::value
                && (!std::numeric_limits<double>::is_iec559 || std::numeric_limits<double>::digits < 53)) {
                return;
            }
            // With cancellations, default setup.
            auto h = 1 - x + y + z + t;
            f = 1 + x + y + z + t;
            tmp2 = h;
            auto tmp3 = f;
            for (int i = 1; i < 10; ++i) {
                h *= tmp2;
                f *= tmp3;
            }
            retval = f * h;
            BOOST_CHECK_EQUAL(retval.size(), 5786u);
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<k_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_multiplication_test)
{
    boost::mpl::for_each<cf_types>(multiplication_tester());
    for (unsigned i = 1u; i <= 4u; ++i) {
        settings::set_n_threads(i);
        boost::mpl::for_each<cf_types>(multiplication_tester());
    }
    settings::reset_n_threads();
}
