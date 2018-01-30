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

#include <piranha/series.hpp>

#define BOOST_TEST_MODULE series_05_test
#include <boost/test/included/unit_test.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/math.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

template <typename Cf, typename Expo>
class g_series_type : public series<Cf, monomial<Expo>, g_series_type<Cf, Expo>>
{
public:
    template <typename Cf2>
    using rebind = g_series_type<Cf2, Expo>;
    typedef series<Cf, monomial<Expo>, g_series_type<Cf, Expo>> base;
    g_series_type() = default;
    g_series_type(const g_series_type &) = default;
    g_series_type(g_series_type &&) = default;
    explicit g_series_type(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set = symbol_fset{name};
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{Expo(1)}));
    }
    g_series_type &operator=(const g_series_type &) = default;
    g_series_type &operator=(g_series_type &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type, base)
};

// Mock coefficient.
struct mock_cf {
    mock_cf();
    mock_cf(const int &);
    mock_cf(const mock_cf &);
    mock_cf(mock_cf &&) noexcept;
    mock_cf &operator=(const mock_cf &);
    mock_cf &operator=(mock_cf &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const mock_cf &);
    mock_cf operator-() const;
    bool operator==(const mock_cf &) const;
    bool operator!=(const mock_cf &) const;
    mock_cf &operator+=(const mock_cf &);
    mock_cf &operator-=(const mock_cf &);
    mock_cf operator+(const mock_cf &) const;
    mock_cf operator-(const mock_cf &) const;
    mock_cf &operator*=(const mock_cf &);
    mock_cf operator*(const mock_cf &)const;
};

// A few extra tests for division after the recent changes in implementation.
BOOST_AUTO_TEST_CASE(series_division_test)
{
    {
        // Equal rec index, no type changes.
        using s_type = g_series_type<integer, int>;
        BOOST_CHECK((is_divisible<s_type>::value));
        BOOST_CHECK((is_divisible_in_place<s_type>::value));
        s_type x{"x"}, y{"y"};
        BOOST_CHECK((std::is_same<s_type, decltype(x / y)>::value));
        BOOST_CHECK_EQUAL(s_type{4} / s_type{-3}, -1);
        BOOST_CHECK_THROW(s_type{4} / s_type{}, mppp::zero_division_error);
        BOOST_CHECK_EQUAL(s_type{0} / s_type{-3}, 0);
        BOOST_CHECK_THROW(x / y, std::invalid_argument);
        s_type tmp{4};
        BOOST_CHECK_THROW(tmp /= s_type{}, mppp::zero_division_error);
        tmp /= s_type{-3};
        BOOST_CHECK_EQUAL(tmp, -1);
        BOOST_CHECK_THROW(x /= y, std::invalid_argument);
        BOOST_CHECK((!is_divisible<g_series_type<mock_cf, int>>::value));
        BOOST_CHECK((!is_divisible_in_place<g_series_type<mock_cf, int>>::value));
    }
    {
        // Equal rec index, first coefficient wins.
        using s_type1 = g_series_type<integer, int>;
        using s_type2 = g_series_type<int, int>;
        BOOST_CHECK((is_divisible<s_type1, s_type2>::value));
        BOOST_CHECK((is_divisible_in_place<s_type1, s_type2>::value));
        s_type1 x{"x"};
        s_type2 y{"y"};
        BOOST_CHECK((std::is_same<s_type1, decltype(x / y)>::value));
        BOOST_CHECK_EQUAL(s_type1{4} / s_type2{-3}, -1);
        BOOST_CHECK_THROW(s_type1{4} / s_type2{}, mppp::zero_division_error);
        BOOST_CHECK_EQUAL(s_type1{0} / s_type2{-3}, 0);
        BOOST_CHECK_THROW(x / y, std::invalid_argument);
        s_type1 tmp{4};
        BOOST_CHECK_THROW(tmp /= s_type2{}, mppp::zero_division_error);
        tmp /= s_type2{-3};
        BOOST_CHECK_EQUAL(tmp, -1);
        BOOST_CHECK_THROW(x /= y, std::invalid_argument);
        BOOST_CHECK((!is_divisible<g_series_type<mock_cf, int>, s_type2>::value));
        BOOST_CHECK((!is_divisible_in_place<g_series_type<mock_cf, int>, s_type2>::value));
    }
    {
        // Equal rec index, second coefficient wins.
        using s_type1 = g_series_type<int, int>;
        using s_type2 = g_series_type<integer, int>;
        BOOST_CHECK((is_divisible<s_type1, s_type2>::value));
        BOOST_CHECK((is_divisible_in_place<s_type1, s_type2>::value));
        s_type1 x{"x"};
        s_type2 y{"y"};
        BOOST_CHECK((std::is_same<s_type2, decltype(x / y)>::value));
        BOOST_CHECK_EQUAL(s_type1{4} / s_type2{-3}, -1);
        BOOST_CHECK_THROW(s_type1{4} / s_type2{}, mppp::zero_division_error);
        BOOST_CHECK_EQUAL(s_type1{0} / s_type2{-3}, 0);
        BOOST_CHECK_THROW(x / y, std::invalid_argument);
        s_type1 tmp{4};
        BOOST_CHECK_THROW(tmp /= s_type2{}, mppp::zero_division_error);
        tmp /= s_type2{-3};
        BOOST_CHECK_EQUAL(tmp, -1);
        BOOST_CHECK_THROW(x /= y, std::invalid_argument);
        BOOST_CHECK((!is_divisible<s_type2, g_series_type<mock_cf, int>>::value));
        BOOST_CHECK((!is_divisible_in_place<s_type2, g_series_type<mock_cf, int>>::value));
    }
    {
        // Equal rec index, need a new coefficient.
        using s_type1 = g_series_type<short, int>;
        using s_type2 = g_series_type<signed char, int>;
        using s_type3 = g_series_type<int, int>;
        BOOST_CHECK((is_divisible<s_type1, s_type2>::value));
        BOOST_CHECK((is_divisible_in_place<s_type1, s_type2>::value));
        s_type1 x{"x"};
        s_type2 y{"y"};
        BOOST_CHECK((std::is_same<s_type3, decltype(x / y)>::value));
        BOOST_CHECK_EQUAL(s_type1{4} / s_type2{-3}, -1);
        BOOST_CHECK_EQUAL(s_type1{0} / s_type2{-3}, 0);
        s_type1 tmp{4};
        tmp /= s_type2{-3};
        BOOST_CHECK_EQUAL(tmp, -1);
        BOOST_CHECK_THROW(x /= y, std::invalid_argument);
        BOOST_CHECK_THROW(x / y, std::invalid_argument);
    }
    {
        // Second has higher recursion index, result is second.
        using s_type1 = g_series_type<int, int>;
        using s_type2 = g_series_type<s_type1, int>;
        BOOST_CHECK((is_divisible<s_type1, s_type2>::value));
        BOOST_CHECK((!is_divisible_in_place<s_type1, s_type2>::value));
        s_type1 x{"x"};
        s_type2 y{"y"};
        BOOST_CHECK((std::is_same<s_type2, decltype(x / y)>::value));
        BOOST_CHECK_EQUAL(s_type1{4} / s_type2{-3}, -1);
        BOOST_CHECK_EQUAL(s_type1{0} / s_type2{-3}, 0);
        BOOST_CHECK_THROW(x / y, std::invalid_argument);
        // Try with scalar as well.
        BOOST_CHECK((is_divisible<int, s_type2>::value));
        BOOST_CHECK((std::is_same<s_type2, decltype(1 / y)>::value));
        BOOST_CHECK_EQUAL(4 / s_type2{-3}, -1);
        BOOST_CHECK_EQUAL(0 / s_type2{-3}, 0);
        BOOST_CHECK((!is_divisible<g_series_type<mock_cf, int>, s_type2>::value));
        BOOST_CHECK((!is_divisible_in_place<g_series_type<mock_cf, int>, s_type2>::value));
    }
    {
        // Second has higher recursion index, result is a new coefficient.
        using s_type1 = g_series_type<signed char, int>;
        using s_type2 = g_series_type<g_series_type<short, int>, int>;
        using s_type3 = g_series_type<g_series_type<int, int>, int>;
        BOOST_CHECK((is_divisible<s_type1, s_type2>::value));
        BOOST_CHECK((!is_divisible_in_place<s_type1, s_type2>::value));
        s_type1 x{"x"};
        s_type2 y{"y"};
        BOOST_CHECK((std::is_same<s_type3, decltype(x / y)>::value));
        BOOST_CHECK_EQUAL(s_type1{4} / s_type2{-3}, -1);
        BOOST_CHECK_EQUAL(s_type1{0} / s_type2{-3}, 0);
        BOOST_CHECK_THROW(x / y, std::invalid_argument);
        // Try with scalar as well.
        BOOST_CHECK((is_divisible<short, s_type1>::value));
        BOOST_CHECK((std::is_same<g_series_type<int, int>, decltype(1 / x)>::value));
        BOOST_CHECK_EQUAL(4 / s_type1{-3}, -1);
        BOOST_CHECK_EQUAL(0 / s_type1{-3}, 0);
    }
}

struct nr_00 {
    nr_00(const nr_00 &) = delete;
    nr_00(nr_00 &&) = delete;
};

template <typename Cf, typename Expo>
class g_series_type2 : public series<Cf, monomial<Expo>, g_series_type2<Cf, Expo>>
{
public:
    template <typename Cf2>
    using rebind = g_series_type2<Cf2, Expo>;
    typedef series<Cf, monomial<Expo>, g_series_type2<Cf, Expo>> base;
    g_series_type2() = default;
    g_series_type2(const g_series_type2 &) = default;
    g_series_type2(g_series_type2 &&) = default;
    explicit g_series_type2(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set.add(name);
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{Expo(1)}));
    }
    g_series_type2 &operator=(const g_series_type2 &) = default;
    g_series_type2 &operator=(g_series_type2 &&) = default;
    nr_00 cos() const;
    nr_00 sin() const;
    template <typename T>
    nr_00 evaluate(const std::unordered_map<std::string, T> &) const;
    PIRANHA_FORWARDING_CTOR(g_series_type2, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2, base)
};

// Check that sin/cos methods that return unreturnable types on a series are disabled.
BOOST_AUTO_TEST_CASE(series_sin_cos_test)
{
    BOOST_CHECK_EQUAL(math::sin(g_series_type2<double, int>{}), math::sin(0.));
    BOOST_CHECK_EQUAL(math::cos(g_series_type2<double, int>{}), math::cos(0.));
}

// Some evaluation tests after we added the improved checking + error message logic in series.
BOOST_AUTO_TEST_CASE(series_evaluation_test)
{
    using math::evaluate;
    using p_type = polynomial<integer, monomial<int>>;
    p_type x{"x"}, y{"y"}, z{"z"};
    BOOST_CHECK_EQUAL(math::evaluate<integer>(x + y + z, {{"x", 1_z}, {"y", 2_z}, {"z", 3_z}}), 1 + 2 + 3);
    BOOST_CHECK_EQUAL(math::evaluate<integer>(x + y + z, {{"x", 1_z}, {"y", 2_z}, {"z", 3_z}, {"t", 4_z}}), 1 + 2 + 3);
    BOOST_CHECK_THROW(math::evaluate<integer>(x + y + z, {{"x", 1_z}, {"y", 2_z}}), std::invalid_argument);
    try {
        (void)math::evaluate<integer>(x + y + z, {{"x", 1_z}, {"y", 2_z}});
    } catch (const std::invalid_argument &ia) {
        BOOST_CHECK(std::string(ia.what()).find("the symbol 'z' is missing from the series evaluation dictionary")
                    != std::string::npos);
    }
    try {
        (void)math::evaluate<integer>(x + y + z, {{"x", 1_z}, {"y", 2_z}, {"a", 4_z}});
    } catch (const std::invalid_argument &ia) {
        BOOST_CHECK(std::string(ia.what()).find("the symbol 'z' is missing from the series evaluation dictionary")
                    != std::string::npos);
    }
    try {
        (void)math::evaluate<integer>(x + y + z, {{"y", 2_z}, {"t", 7_z}});
    } catch (const std::invalid_argument &ia) {
        BOOST_CHECK(std::string(ia.what()).find("the symbol 'x' is missing from the series evaluation dictionary")
                    != std::string::npos);
    }
    try {
        (void)math::evaluate<integer>(x + y + z, {{"x", 1_z}, {"z", 2_z}});
    } catch (const std::invalid_argument &ia) {
        BOOST_CHECK(std::string(ia.what()).find("the symbol 'y' is missing from the series evaluation dictionary")
                    != std::string::npos);
    }
    try {
        (void)math::evaluate<integer>(x + y + z, {{"a", 2_z}, {"b", 3_z}});
    } catch (const std::invalid_argument &ia) {
        BOOST_CHECK(std::string(ia.what()).find("the symbol 'x' is missing from the series evaluation dictionary")
                    != std::string::npos);
    }
}
