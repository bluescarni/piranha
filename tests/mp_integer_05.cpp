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

#include <piranha/mp_integer.hpp>

#define BOOST_TEST_MODULE mp_integer_05_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <limits>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/init.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

// NOTE: check floating-point support in the new type.
using fp_types = std::tuple<float, double, long double>;

// NOTE: this should be ok in the new type.
using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long>;

using size_types = std::tuple<std::integral_constant<int, 0>, std::integral_constant<int, 8>,
                              std::integral_constant<int, 16>, std::integral_constant<int, 32>
#if defined(PIRANHA_UINT128_T)
                              ,
                              std::integral_constant<int, 64>
#endif
                              >;

struct safe_cast_float_tester {
    template <typename S>
    struct runner {
        template <typename T>
        void operator()(const T &) const
        {
            using int_type = mp_integer<S::value>;
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

BOOST_AUTO_TEST_CASE(mp_integer_safe_cast_float_test)
{
    init();
    tuple_for_each(size_types{}, safe_cast_float_tester());
}

struct safe_cast_int_tester {
    template <typename S>
    struct runner {
        template <typename T>
        void operator()(const T &) const
        {
            using int_type = mp_integer<S::value>;
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
            BOOST_CHECK_EQUAL(safe_cast<int_type>(T(0)), int_type{0});
            BOOST_CHECK_EQUAL(safe_cast<int_type>(T(1)), int_type{1});
            BOOST_CHECK_EQUAL(safe_cast<int_type>(T(12)), int_type{12});
            BOOST_CHECK_EQUAL(safe_cast<T>(int_type(0)), T{0});
            BOOST_CHECK_EQUAL(safe_cast<T>(int_type(1)), T{1});
            BOOST_CHECK_EQUAL(safe_cast<T>(int_type{12}), T{12});

            // Failures.
            using lim = std::numeric_limits<T>;
            BOOST_CHECK_EXCEPTION(
                safe_cast<T>(int_type(lim::max()) + 1), safe_cast_failure, [](const safe_cast_failure &e) {
                    return boost::contains(e.what(), "the conversion cannot preserve the original value");
                });
            BOOST_CHECK_EXCEPTION(
                safe_cast<T>(int_type(lim::min()) - 1), safe_cast_failure, [](const safe_cast_failure &e) {
                    return boost::contains(e.what(), "the conversion cannot preserve the original value");
                });
        }
    };
    template <typename S>
    void operator()(const S &) const
    {
        tuple_for_each(int_types{}, runner<S>{});
    }
};

BOOST_AUTO_TEST_CASE(mp_integer_safe_cast_int_test)
{
    tuple_for_each(size_types{}, safe_cast_int_tester());
}
