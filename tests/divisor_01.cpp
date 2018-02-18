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

#include <piranha/divisor.hpp>

#define BOOST_TEST_MODULE divisor_01_test
#include <boost/test/included/unit_test.hpp>

#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mp++/config.hpp>
#include <mp++/exceptions.hpp>

#include <piranha/detail/vector_hasher.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/integer.hpp>
#include <piranha/key/key_is_one.hpp>
#include <piranha/key/key_is_zero.hpp>
#include <piranha/key_is_convertible.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/symbol_utils.hpp>
#include <piranha/term.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

#if defined(MPPP_WITH_MPFR)

static inline real operator"" _r(const char *s)
{
    return real(s, 100);
}

#endif

using value_types = std::tuple<signed char, short, int, long, long long, integer>;

struct ctor_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        // Default constructor.
        d_type d0;
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        T e(1);
        std::vector<T> tmp{T(1), T(-3)};
        d0.insert(tmp.begin(), tmp.end(), e);
        tmp[0u] = 4;
        tmp[1u] = -5;
        d0.insert(tmp.begin(), tmp.end(), e);
        BOOST_CHECK_EQUAL(d0.size(), 2u);
        // Copy constructor.
        d_type d1{d0};
        BOOST_CHECK_EQUAL(d1.size(), 2u);
        BOOST_CHECK(d1 == d0);
        // Move constructor.
        d_type d2{std::move(d1)};
        BOOST_CHECK_EQUAL(d2.size(), 2u);
        BOOST_CHECK(d2 == d0);
        BOOST_CHECK_EQUAL(d1.size(), 0u);
        // Copy assignment.
        d_type d3;
        d3 = d0;
        BOOST_CHECK_EQUAL(d2.size(), 2u);
        BOOST_CHECK(d2 == d0);
        // Move assignment.
        d_type d4;
        d4 = std::move(d3);
        BOOST_CHECK_EQUAL(d4.size(), 2u);
        BOOST_CHECK(d4 == d0);
        BOOST_CHECK_EQUAL(d3.size(), 0u);
        // Test clear().
        d4.clear();
        BOOST_CHECK_EQUAL(d4.size(), 0u);
        // Constructor from arguments set.
        d_type d5(symbol_fset{});
        BOOST_CHECK_EQUAL(d5.size(), 0u);
        d_type d6(symbol_fset{"foo"});
        BOOST_CHECK_EQUAL(d6.size(), 0u);
        // Converting constructor.
        d_type d7(d6, symbol_fset{"foo"});
        BOOST_CHECK_EQUAL(d7.size(), 0u);
        d7.insert(tmp.begin(), tmp.end(), e);
        d_type d8(d7, symbol_fset{"foo", "bar"});
        BOOST_CHECK_EQUAL(d8.size(), 1u);
        BOOST_CHECK_THROW((d_type{d7, symbol_fset{"foo", "bar", "frob"}}), std::invalid_argument);
        // Check the type trait.
        BOOST_CHECK((key_is_convertible<d_type, d_type>::value));
        BOOST_CHECK((!key_is_convertible<d_type, monomial<int>>::value));
        BOOST_CHECK((!key_is_convertible<monomial<int>, d_type>::value));
    }
};

BOOST_AUTO_TEST_CASE(divisor_ctor_test)
{
    tuple_for_each(value_types{}, ctor_tester{});
}

struct insert_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d0;
        // Insertion with non-positive exponent must fail.
        std::vector<T> tmp;
        T exponent(0);
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        exponent = -1;
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        // Various canonical checks.
        exponent = 1;
        // Empty vector must fail.
        tmp = {};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        // Vectors of zeroes must fail.
        tmp = {T(0)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        tmp = {T(0), T(0)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        // First nonzero element negative must fail.
        tmp = {T(-1), T(2)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        tmp = {T(0), T(-1), T(2)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        tmp = {T(0), T(-2), T(0), T(3), T(0)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        tmp = {T(-7), T(0), T(-2), T(0), T(3), T(0)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        // Non-coprimes must fail.
        tmp = {T(8)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        tmp = {T(8), T(0), T(-2), T(0), T(6), T(0)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        tmp = {T(0), T(8), T(0), T(-2), T(0), T(6), T(0)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        tmp = {T(8), T(-2), T(6)};
        BOOST_CHECK_THROW(d0.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d0.size(), 0u);
        // Some successful insertions.
        tmp = {T(1)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.size(), 1u);
        d0.clear();
        tmp = {T(8), T(-3), T(6)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(8), T(-3), T(7)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.size(), 2u);
        // Update an exponent.
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.size(), 2u);
        // Insert another new term.
        tmp = {T(8), T(-3), T(35)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.size(), 3u);
        // Various range checks dependent on T being an integral type.
        range_checks<T>();
    }
    template <typename T, typename U = T, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
    static void range_checks()
    {
        divisor<T> d;
        std::vector<T> tmp;
        T exponent(1);
        if (std::numeric_limits<T>::min() < -detail::safe_abs_sint<T>::value
            && std::numeric_limits<T>::max() > detail::safe_abs_sint<T>::value) {
            tmp = {std::numeric_limits<T>::min()};
            BOOST_CHECK_THROW(d.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
            BOOST_CHECK_EQUAL(d.size(), 0u);
            tmp = {std::numeric_limits<T>::max()};
            BOOST_CHECK_THROW(d.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
            BOOST_CHECK_EQUAL(d.size(), 0u);
        }
        // Check potential failure in safe_cast for the exponent and the values.
        const long long long_e = std::numeric_limits<long long>::max();
        if (std::numeric_limits<T>::max() < long_e) {
            tmp = {T(1)};
            BOOST_CHECK_THROW(d.insert(tmp.begin(), tmp.end(), long_e), std::invalid_argument);
            BOOST_CHECK_EQUAL(d.size(), 0u);
            std::vector<long long> tmp2 = {long_e, long_e};
            BOOST_CHECK_THROW(d.insert(tmp2.begin(), tmp2.end(), exponent), std::invalid_argument);
            BOOST_CHECK_EQUAL(d.size(), 0u);
        }
        // Check failure in updating the exponent.
        tmp = {T(1)};
        exponent = std::numeric_limits<T>::max();
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = T(1);
        BOOST_CHECK_THROW(d.insert(tmp.begin(), tmp.end(), exponent), std::invalid_argument);
        BOOST_CHECK_EQUAL(d.size(), 1u);
    }
    template <typename T, typename U = T, typename std::enable_if<!std::is_integral<U>::value, int>::type = 0>
    static void range_checks()
    {
    }
};

BOOST_AUTO_TEST_CASE(divisor_insert_test)
{
    tuple_for_each(value_types{}, insert_tester{});
}

struct equality_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        std::vector<T> tmp;
        T exponent(1);
        d_type d0;
        BOOST_CHECK(d0 == d0);
        d_type d1;
        BOOST_CHECK(d0 == d1);
        tmp = {T(1), T(2)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(!(d0 == d1));
        BOOST_CHECK(d0 != d1);
        d1.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(d0 == d1);
        tmp = {T(1), T(-2)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(!(d0 == d1));
        BOOST_CHECK(d0 != d1);
        exponent = T(2);
        d1.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(!(d0 == d1));
        BOOST_CHECK(d0 != d1);
        exponent = T(1);
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(d0 == d1);
    }
};

BOOST_AUTO_TEST_CASE(divisor_equality_test)
{
    tuple_for_each(value_types{}, equality_tester{});
}

struct hash_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        std::vector<T> tmp;
        T exponent(1);
        d_type d0;
        std::hash<d_type> hasher;
        BOOST_CHECK_EQUAL(d0.hash(), 0u);
        BOOST_CHECK_EQUAL(d0.hash(), hasher(d0));
        tmp = {T(1), T(2)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.hash(), detail::vector_hasher(tmp));
        BOOST_CHECK_EQUAL(d0.hash(), hasher(d0));
        tmp = {T(2), T(1)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.hash(), detail::vector_hasher(tmp) + detail::vector_hasher(std::vector<T>{T(1), T(2)}));
        BOOST_CHECK_EQUAL(d0.hash(), hasher(d0));
        // Check that the exponent does not matter.
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.hash(), detail::vector_hasher(tmp) + detail::vector_hasher(std::vector<T>{T(1), T(2)}));
        BOOST_CHECK_EQUAL(d0.hash(), hasher(d0));
        tmp = {T(1), T(2)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_EQUAL(d0.hash(), detail::vector_hasher(tmp) + detail::vector_hasher(std::vector<T>{T(2), T(1)}));
        BOOST_CHECK_EQUAL(d0.hash(), hasher(d0));
        BOOST_CHECK_EQUAL(d0.size(), 2u);
    }
};

BOOST_AUTO_TEST_CASE(divisor_hash_test)
{
    tuple_for_each(value_types{}, hash_tester{});
}

struct tt_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        BOOST_CHECK(is_container_element<d_type>::value);
    }
};

BOOST_AUTO_TEST_CASE(divisor_tt_test)
{
    tuple_for_each(value_types{}, tt_tester{});
}

struct ci_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d0;
        BOOST_CHECK(d0.is_compatible(symbol_fset{}));
        BOOST_CHECK(!piranha::key_is_zero(d0, symbol_fset{}));
        BOOST_CHECK(d0.is_compatible(symbol_fset{"foo", "bar"}));
        BOOST_CHECK(!piranha::key_is_zero(d0, symbol_fset{"foo", "bar"}));
        std::vector<T> tmp;
        T exponent(1);
        tmp = {T(1)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(!d0.is_compatible(symbol_fset{"foo", "bar"}));
        BOOST_CHECK(!piranha::key_is_zero(d0, symbol_fset{"foo", "bar"}));
        BOOST_CHECK(!d0.is_compatible(symbol_fset{}));
        BOOST_CHECK(!piranha::key_is_zero(d0, symbol_fset{}));
    }
};

BOOST_AUTO_TEST_CASE(divisor_ci_test)
{
    tuple_for_each(value_types{}, ci_tester{});
}

struct key_is_one_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d0;
        BOOST_CHECK(piranha::key_is_one(d0, symbol_fset{}));
        BOOST_CHECK(piranha::key_is_one(d0, symbol_fset{"foo"}));
        std::vector<T> tmp;
        T exponent(1);
        tmp = {T(1)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(!piranha::key_is_one(d0, symbol_fset{"foo"}));
    }
};

BOOST_AUTO_TEST_CASE(divisor_key_is_one_test)
{
    tuple_for_each(value_types{}, key_is_one_tester{});
}

struct merge_symbols_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d;
        T exponent(1);
        std::vector<T> tmp;
        tmp = {T(1), T(2)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        auto out = d.merge_symbols({{0, {"a"}}, {1, {"c"}}}, symbol_fset{"b", "d"});
        BOOST_CHECK_EQUAL(out.size(), 1u);
        BOOST_CHECK_EQUAL(out._container().begin()->v.size(), 4u);
        BOOST_CHECK((out._container().begin()->v == small_vector<T>{T(0), T(1), T(0), T(2)}));
        tmp = {T(3), T(-2)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        out = d.merge_symbols({{0, {"a"}}, {1, {"c"}}}, symbol_fset{"b", "d"});
        BOOST_CHECK_EQUAL(out.size(), 2u);
        auto it = out._container().begin();
        BOOST_CHECK((it->v == small_vector<T>{T(0), T(1), T(0), T(2)})
                    || (it->v == small_vector<T>{T(0), T(3), T(0), T(-2)}));
        ++it;
        BOOST_CHECK((it->v == small_vector<T>{T(0), T(1), T(0), T(2)})
                    || (it->v == small_vector<T>{T(0), T(3), T(0), T(-2)}));
        d.clear();
        tmp = {T(3), T(-2), T(0), T(1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(1), T(-2), T(0), T(7)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        out = d.merge_symbols({{0, {"a"}}, {1, {"c"}}, {3, {"f"}}, {4, {"h"}}}, symbol_fset{"b", "d", "e", "g"});
        BOOST_CHECK_EQUAL(out.size(), 2u);
        it = out._container().begin();
        BOOST_CHECK((it->v == small_vector<T>{T(0), T(3), T(0), T(-2), T(0), T(0), T(1), T(0)})
                    || (it->v == small_vector<T>{T(0), T(1), T(0), T(-2), T(0), T(0), T(7), T(0)}));
        ++it;
        BOOST_CHECK((it->v == small_vector<T>{T(0), T(3), T(0), T(-2), T(0), T(0), T(1), T(0)})
                    || (it->v == small_vector<T>{T(0), T(1), T(0), T(-2), T(0), T(0), T(7), T(0)}));
        // Check the throwing conditions.
        BOOST_CHECK_THROW(d.merge_symbols({{0, {"a"}}, {1, {"c"}}, {3, {"f"}}, {4, {"h"}}}, symbol_fset{"b", "d"}),
                          std::invalid_argument);
        BOOST_CHECK_THROW(d.merge_symbols({}, symbol_fset{"b", "d", "e", "g"}), std::invalid_argument);
        BOOST_CHECK_THROW(
            d.merge_symbols({{0, {"a"}}, {1, {"c"}}, {3, {"f"}}, {40, {"h"}}}, symbol_fset{"b", "d", "e", "g"}),
            std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(divisor_merge_symbols_test)
{
    tuple_for_each(value_types{}, merge_symbols_tester{});
}
struct print_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d;
        std::ostringstream oss;
        d.print(oss, symbol_fset{});
        BOOST_CHECK(oss.str().empty());
        T exponent(1);
        std::vector<T> tmp;
        tmp = {T(1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print(oss, symbol_fset{"x"});
        BOOST_CHECK_EQUAL(oss.str(), "1/[(x)]");
        exponent = 2;
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print(oss, symbol_fset{"x"});
        BOOST_CHECK_EQUAL(oss.str(), "1/[(x)**2]");
        tmp = {T(1), T(-2)};
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 1;
        tmp = {T(3), T(4)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print(oss, symbol_fset{"x", "y"});
        BOOST_CHECK(oss.str() == "1/[(x-2*y)**2*(3*x+4*y)]" || oss.str() == "1/[(3*x+4*y)*(x-2*y)**2]");
        tmp = {T(1), T(0), T(-1)};
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 3;
        tmp = {T(0), T(4), T(-1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print(oss, symbol_fset{"x", "y", "z"});
        BOOST_CHECK(oss.str() == "1/[(x-z)*(4*y-z)**3]" || oss.str() == "1/[(4*y-z)**3*(x-z)]");
        tmp = {T(1), T(0), T(0)};
        exponent = 1;
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 3;
        tmp = {T(0), T(4), T(-1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print(oss, symbol_fset{"x", "y", "z"});
        BOOST_CHECK(oss.str() == "1/[(x)*(4*y-z)**3]" || oss.str() == "1/[(4*y-z)**3*(x)]");
        // Check throwing.
        BOOST_CHECK_THROW(d.print(oss, symbol_fset{"x", "y", "z", "t"}), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(divisor_print_test)
{
    tuple_for_each(value_types{}, print_tester{});
}

struct print_tex_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d;
        std::ostringstream oss;
        d.print_tex(oss, symbol_fset{});
        BOOST_CHECK(oss.str().empty());
        T exponent(1);
        std::vector<T> tmp;
        tmp = {T(1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print_tex(oss, symbol_fset{"x"});
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{\\left(x\\right)}");
        exponent = 2;
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print_tex(oss, symbol_fset{"x"});
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{\\left(x\\right)^{2}}");
        tmp = {T(1), T(-2)};
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 1;
        tmp = {T(3), T(4)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print_tex(oss, symbol_fset{"x", "y"});
        BOOST_CHECK(oss.str() == "\\frac{1}{\\left(x-2y\\right)^{2}\\left(3x+4y\\right)}"
                    || oss.str() == "\\frac{1}{\\left(3x+4y\\right)\\left(x-2y\\right)^{2}}");
        tmp = {T(1), T(0), T(-1)};
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 3;
        tmp = {T(0), T(4), T(-1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print_tex(oss, symbol_fset{"x", "y", "z"});
        BOOST_CHECK(oss.str() == "\\frac{1}{\\left(x-z\\right)\\left(4y-z\\right)^{3}}"
                    || oss.str() == "\\frac{1}{\\left(4y-z\\right)^{3}\\left(x-z\\right)}");
        tmp = {T(1), T(0), T(0)};
        exponent = 1;
        d.clear();
        oss.str("");
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 3;
        tmp = {T(0), T(4), T(-1)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        d.print_tex(oss, symbol_fset{"x", "y", "z"});
        BOOST_CHECK(oss.str() == "\\frac{1}{\\left(x\\right)\\left(4y-z\\right)^{3}}"
                    || oss.str() == "\\frac{1}{\\left(4y-z\\right)^{3}\\left(x\\right)}");
        // Check throwing.
        BOOST_CHECK_THROW(d.print_tex(oss, symbol_fset{"x", "y", "z", "t"}), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(divisor_print_tex_test)
{
    tuple_for_each(value_types{}, print_tex_tester{});
}

struct evaluate_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d;
        // Test the type trait first.
        BOOST_CHECK((key_is_evaluable<d_type, rational>::value));
        BOOST_CHECK((key_is_evaluable<d_type, double>::value));
#if defined(MPPP_WITH_MPFR)
        BOOST_CHECK((key_is_evaluable<d_type, long double>::value));
        BOOST_CHECK((key_is_evaluable<d_type, real>::value));
#endif
        BOOST_CHECK((!key_is_evaluable<d_type, std::string>::value));
        BOOST_CHECK((!key_is_evaluable<d_type, void>::value));
        // Empty divisor.
        BOOST_CHECK_EQUAL(d.template evaluate<rational>({}, symbol_fset{}), 1_q);
        BOOST_CHECK((std::is_same<rational, decltype(d.template evaluate<rational>({}, symbol_fset{}))>::value));
        BOOST_CHECK((std::is_same<double, decltype(d.template evaluate<double>({}, symbol_fset{}))>::value));
#if defined(MPPP_WITH_MPFR)
        BOOST_CHECK((std::is_same<real, decltype(d.template evaluate<real>({}, symbol_fset{}))>::value));
#endif
        T exponent(2);
        std::vector<T> tmp;
        tmp = {T(1), T(-2)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 3;
        tmp = {T(2), T(7)};
        d.insert(tmp.begin(), tmp.end(), exponent);
        // Error checking.
        BOOST_CHECK_EXCEPTION(d.template evaluate<rational>({}, symbol_fset{"x"}), std::invalid_argument,
                              [](const std::invalid_argument &e) {
                                  return boost::contains(e.what(), "cannot evaluate divisor: the size of the symbol "
                                                                   "set (1) differs from the size of the vector of "
                                                                   "values (0)");
                              });
        BOOST_CHECK_EXCEPTION(d.template evaluate<rational>({1_q}, symbol_fset{"x"}), std::invalid_argument,
                              [](const std::invalid_argument &e) {
                                  return boost::contains(e.what(), "cannot evaluate divisor: the size of the symbol "
                                                                   "set (1) differs from the number of symbols in the "
                                                                   "divisor (2)");
                              });
        // Some numerical checks.
        BOOST_CHECK_EQUAL(d.template evaluate<rational>({-1_q, 2_q}, symbol_fset{"x", "y"}), 1 / 43200_q);
        BOOST_CHECK_EQUAL(d.template evaluate<rational>({2 / 3_q, -4 / 5_q}, symbol_fset{"x", "y"}),
                          -759375_z / 303038464_q);
        BOOST_CHECK_THROW(d.template evaluate<rational>({2_q, 1_q}, symbol_fset{"x", "y"}), mppp::zero_division_error);
#if defined(MPPP_WITH_MPFR)
        // A simple test with real.
        BOOST_CHECK_EQUAL(d.template evaluate<real>({-1.5_r, 2.5_r}, symbol_fset{"x", "y"}),
                          1 / (piranha::pow(-1.5_r - 2.5_r * 2, 2) * piranha::pow(-1.5_r * 2 + 7 * 2.5_r, 3)));
#endif
    }
};

BOOST_AUTO_TEST_CASE(divisor_evaluate_test)
{
    tuple_for_each(value_types{}, evaluate_tester{});
}
// Mock cf with wrong specialisation of mul3.
struct mock_cf3 {
    mock_cf3();
    explicit mock_cf3(const int &);
    mock_cf3(const mock_cf3 &);
    mock_cf3(mock_cf3 &&) noexcept;
    mock_cf3 &operator=(const mock_cf3 &);
    mock_cf3 &operator=(mock_cf3 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const mock_cf3 &);
    mock_cf3 operator-() const;
    bool operator==(const mock_cf3 &) const;
    bool operator!=(const mock_cf3 &) const;
    mock_cf3 &operator+=(const mock_cf3 &);
    mock_cf3 &operator-=(const mock_cf3 &);
    mock_cf3 operator+(const mock_cf3 &) const;
    mock_cf3 operator-(const mock_cf3 &) const;
    mock_cf3 &operator*=(const mock_cf3 &);
    mock_cf3 operator*(const mock_cf3 &)const;
};

namespace piranha
{
namespace math
{

template <typename T>
struct mul3_impl<T, typename std::enable_if<std::is_same<T, mock_cf3>::value>::type> {
};
}
}

struct multiply_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        // Test the type trait first.
        BOOST_CHECK((key_is_multipliable<double, d_type>::value));
        BOOST_CHECK((key_is_multipliable<integer, d_type>::value));
#if defined(MPPP_WITH_MPFR)
        BOOST_CHECK((key_is_multipliable<real, d_type>::value));
#endif
        BOOST_CHECK((key_is_multipliable<rational, d_type>::value));
        BOOST_CHECK((!key_is_multipliable<mock_cf3, d_type>::value));
        std::array<term<integer, d_type>, 1u> res;
        term<integer, d_type> t1, t2;
        t1.m_cf = 2;
        t2.m_cf = -3;
        // Try with empty divisors first.
        d_type::multiply(res, t1, t2, symbol_fset{});
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0u].m_key.size(), 0u);
        // 1 - 0.
        std::ostringstream ss;
        T exponent(2);
        std::vector<T> tmp;
        tmp = {T(1), T(-2)};
        t1.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(res, t1, t2, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0u].m_key.size(), 1u);
        res[0u].m_key.print(ss, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(ss.str(), "1/[(x-2*y)**2]");
        // 0 - 1.
        ss.str("");
        t1.m_key.clear();
        t2.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(res, t1, t2, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0u].m_key.size(), 1u);
        res[0u].m_key.print(ss, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(ss.str(), "1/[(x-2*y)**2]");
        // 1 - 1.
        ss.str("");
        tmp = {T(4), T(-3)};
        exponent = 3;
        t1.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(res, t1, t2, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0u].m_key.size(), 2u);
        res[0u].m_key.print(ss, symbol_fset{"x", "y"});
        BOOST_CHECK(ss.str() == "1/[(x-2*y)**2*(4*x-3*y)**3]" || ss.str() == "1/[(4*x-3*y)**3*(x-2*y)**2]");
        // 1 - 1 with simplification.
        ss.str("");
        tmp = {T(1), T(-2)};
        t1.m_key.clear();
        t1.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(res, t1, t2, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0u].m_key.size(), 1u);
        res[0u].m_key.print(ss, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(ss.str(), "1/[(x-2*y)**5]");
        // A 2 - 3 test with simplification.
        t1.m_key.clear();
        t2.m_key.clear();
        // (x - 2y).
        tmp = {T(1), T(-2)};
        exponent = 1;
        t1.m_key.insert(tmp.begin(), tmp.end(), exponent);
        // (8x + 3y)**2.
        tmp = {T(8), T(3)};
        exponent = 2;
        t1.m_key.insert(tmp.begin(), tmp.end(), exponent);
        // (x + y)**4.
        tmp = {T(1), T(1)};
        exponent = 4;
        t2.m_key.insert(tmp.begin(), tmp.end(), exponent);
        // (8x + 3y)**3.
        tmp = {T(8), T(3)};
        exponent = 3;
        t2.m_key.insert(tmp.begin(), tmp.end(), exponent);
        // (x - y)**4.
        tmp = {T(1), T(-1)};
        exponent = 4;
        t2.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(res, t1, t2, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0u].m_key.size(), 4u);
        // Check correct handling of rationals.
        std::array<term<rational, d_type>, 1u> resq;
        term<rational, d_type> ta, tb;
        ta.m_cf = 2 / 3_q;
        tb.m_cf = -3 / 5_q;
        tmp = {T(1), T(-1)};
        exponent = 4;
        ta.m_key.insert(tmp.begin(), tmp.end(), exponent);
        tb.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(resq, ta, tb, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(resq[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(resq[0u].m_key.size(), 1u);
        // Coefficient series test.
        std::array<term<polynomial<integer, monomial<int>>, d_type>, 1u> res2;
        term<polynomial<integer, monomial<int>>, d_type> t1a, t2a;
        ss.str("");
        t1a.m_cf = -2;
        t2a.m_cf = 3;
        tmp = {T(1), T(-2)};
        exponent = 3;
        t1a.m_key.insert(tmp.begin(), tmp.end(), exponent);
        exponent = 1;
        t2a.m_key.insert(tmp.begin(), tmp.end(), exponent);
        d_type::multiply(res2, t1a, t2a, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res2[0u].m_cf, -6);
        BOOST_CHECK_EQUAL(res2[0u].m_key.size(), 1u);
        res2[0u].m_key.print(ss, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(ss.str(), "1/[(x-2*y)**4]");
        // Test incompatible symbol set.
        BOOST_CHECK_THROW(d_type::multiply(res, t1, t2, symbol_fset{"x", "y", "z"}), std::invalid_argument);
        t1.m_key.clear();
        BOOST_CHECK_THROW(d_type::multiply(res, t1, t2, symbol_fset{"x", "y", "z"}), std::invalid_argument);
        // Exponent range overflow check.
        range_checks<T>();
    }
    template <typename T, typename U = T, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
    static void range_checks()
    {
        using d_type = divisor<T>;
        // Test the type trait first.
        std::array<term<integer, d_type>, 1u> res;
        term<integer, d_type> t1, t2;
        t1.m_cf = 2;
        t2.m_cf = -3;
        T exponent(1);
        std::vector<T> tmp;
        tmp = {T(1), T(-2)};
        t1.m_key.insert(tmp.begin(), tmp.end(), exponent);
        exponent = std::numeric_limits<T>::max();
        t2.m_key.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK_THROW(d_type::multiply(res, t1, t2, symbol_fset{"x", "y"}), std::invalid_argument);
        // Basic exception safety.
        BOOST_CHECK_EQUAL(res[0u].m_cf, -6);
    }
    template <typename T, typename U = T, typename std::enable_if<!std::is_integral<U>::value, int>::type = 0>
    static void range_checks()
    {
    }
};

BOOST_AUTO_TEST_CASE(divisor_multiply_test)
{
    tuple_for_each(value_types{}, multiply_tester{});
}

struct trim_identify_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type d0;
        T exponent(2);
        std::vector<T> tmp;
        tmp = {T(1), T(-2)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(3), T(-4)};
        exponent = 1;
        std::vector<char> mask{1, 1};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        d0.trim_identify(mask, symbol_fset{"x", "y"});
        BOOST_CHECK((mask == std::vector<char>{0, 0}));
        d0.clear();
        mask = std::vector<char>{1, 1};
        tmp = {T(1), T(0)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(3), T(-4)};
        exponent = 3;
        d0.insert(tmp.begin(), tmp.end(), exponent);
        d0.trim_identify(mask, symbol_fset{"x", "y"});
        BOOST_CHECK((mask == std::vector<char>{0, 0}));
        d0.clear();
        mask = std::vector<char>{1, 1, 1};
        tmp = {T(1), T(0), T(3)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(0), T(3), T(-4)};
        exponent = 2;
        d0.insert(tmp.begin(), tmp.end(), exponent);
        d0.trim_identify(mask, symbol_fset{"x", "y", "z"});
        BOOST_CHECK((mask == std::vector<char>{0, 0, 0}));
        d0.clear();
        mask = std::vector<char>{1, 1, 1};
        tmp = {T(1), T(0), T(3)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(1), T(0), T(-4)};
        exponent = 1;
        d0.insert(tmp.begin(), tmp.end(), exponent);
        d0.trim_identify(mask, symbol_fset{"x", "y", "z"});
        BOOST_CHECK((mask == std::vector<char>{0, 1, 0}));
        // Error handling.
        BOOST_CHECK_EXCEPTION(d0.trim_identify(mask, symbol_fset{"x", "y"}), std::invalid_argument,
                              [](const std::invalid_argument &e) {
                                  return boost::contains(e.what(), "invalid arguments set for trim_identify()");
                              });
        mask = std::vector<char>{1, 1, 1, 1};
        BOOST_CHECK_EXCEPTION(d0.trim_identify(mask, symbol_fset{"x", "y", "z"}), std::invalid_argument,
                              [](const std::invalid_argument &e) {
                                  return boost::contains(e.what(), "invalid symbol_set for trim_identify() in a "
                                                                   "divisor: the size of the symbol set (3) differs "
                                                                   "from the size of the trim mask (4)");
                              });
    }
};

BOOST_AUTO_TEST_CASE(divisor_trim_identify_test)
{
    tuple_for_each(value_types{}, trim_identify_tester{});
}

struct trim_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        std::ostringstream ss;
        d_type d0;
        T exponent(2);
        std::vector<T> tmp;
        tmp = {T(1), T(0), T(-1)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(3), T(0), T(-5)};
        exponent = 1;
        d0.insert(tmp.begin(), tmp.end(), exponent);
        std::vector<char> mask{0, 1, 0};
        auto d1 = d0.trim(mask, symbol_fset{"x", "y", "z"});
        d1.print(ss, symbol_fset{"x", "z"});
        BOOST_CHECK_EQUAL(d1.size(), 2u);
        BOOST_CHECK(ss.str() == "1/[(x-z)**2*(3*x-5*z)]" || ss.str() == "1/[(3*x-5*z)*(x-z)**2]");
        // Check a case that does not trim anything.
        ss.str("");
        d0.clear();
        tmp = {T(1), T(0), T(-1)};
        d0.insert(tmp.begin(), tmp.end(), exponent);
        tmp = {T(3), T(0), T(-5)};
        exponent = 4;
        d0.insert(tmp.begin(), tmp.end(), exponent);
        mask = {0, 0, 0};
        d1 = d0.trim(mask, symbol_fset{"x", "y", "z"});
        d1.print(ss, symbol_fset{"x", "y", "z"});
        BOOST_CHECK_EQUAL(d1.size(), 2u);
        BOOST_CHECK(ss.str() == "1/[(x-z)*(3*x-5*z)**4]" || ss.str() == "1/[(3*x-5*z)**4*(x-z)]");
        // Failure modes.
        BOOST_CHECK_EXCEPTION(d0.trim(mask, symbol_fset{"x", "y"}), std::invalid_argument,
                              [](const std::invalid_argument &e) {
                                  return boost::contains(e.what(), "invalid arguments set for trim()");
                              });
        mask = {0, 0};
        BOOST_CHECK_EXCEPTION(
            d0.trim(mask, symbol_fset{"x", "y", "z"}), std::invalid_argument, [](const std::invalid_argument &e) {
                return boost::contains(e.what(), "invalid symbol_set for trim() in a divisor: the size of the symbol "
                                                 "set (3) differs from the size of the trim mask (2)");
            });
    }
};

BOOST_AUTO_TEST_CASE(divisor_trim_test)
{
    tuple_for_each(value_types{}, trim_tester{});
}

struct split_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using d_type = divisor<T>;
        d_type k1;
        auto s1 = k1.split(0, symbol_fset{"x"});
        BOOST_CHECK_EQUAL(s1.first.size(), 0u);
        BOOST_CHECK_EQUAL(s1.second.size(), 0u);
        BOOST_CHECK_EXCEPTION(k1.split(1, symbol_fset{"x"}), std::invalid_argument, [](const std::invalid_argument &e) {
            return boost::contains(e.what(), "invalid index for the splitting of a divisor: the value of the index (1) "
                                             "is not less than the number of symbols in the divisor (1)");
        });
        T exponent(1);
        std::vector<T> tmp{T(1)};
        k1.insert(tmp.begin(), tmp.end(), exponent);
        s1 = k1.split(0, symbol_fset{"x"});
        BOOST_CHECK_EQUAL(s1.first.size(), 1u);
        BOOST_CHECK_EQUAL(s1.second.size(), 0u);
        BOOST_CHECK(s1.first == k1);
        BOOST_CHECK_EXCEPTION(k1.split(0, symbol_fset{}), std::invalid_argument, [](const std::invalid_argument &e) {
            return boost::contains(e.what(), "invalid size of arguments set");
        });
        k1 = d_type{};
        tmp = std::vector<T>{T(1), T(0)};
        k1.insert(tmp.begin(), tmp.end(), exponent);
        tmp = std::vector<T>{T(0), T(1)};
        k1.insert(tmp.begin(), tmp.end(), exponent);
        s1 = k1.split(0, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(s1.first.size(), 1u);
        BOOST_CHECK_EQUAL(s1.second.size(), 1u);
        auto k2 = d_type{};
        tmp = std::vector<T>{T(1), T(0)};
        k2.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(s1.first == k2);
        k2 = d_type{};
        tmp = std::vector<T>{T(0), T(1)};
        k2.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(s1.second == k2);
        s1 = k1.split(1, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(s1.first.size(), 1u);
        BOOST_CHECK_EQUAL(s1.second.size(), 1u);
        k2 = d_type{};
        tmp = std::vector<T>{T(0), T(1)};
        k2.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(s1.first == k2);
        k2 = d_type{};
        tmp = std::vector<T>{T(1), T(0)};
        k2.insert(tmp.begin(), tmp.end(), exponent);
        BOOST_CHECK(s1.second == k2);
    }
};

BOOST_AUTO_TEST_CASE(divisor_split_test)
{
    tuple_for_each(value_types{}, split_tester{});
}
