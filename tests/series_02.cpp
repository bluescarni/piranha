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

#define BOOST_TEST_MODULE series_02_test
#include <boost/test/included/unit_test.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <piranha/base_series_multiplier.hpp>
#include <piranha/debug_access.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/init.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/pow.hpp>
#include <piranha/real.hpp>
#include <piranha/s11n.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/symbol.hpp>
#include <piranha/symbol_set.hpp>
#include <piranha/type_traits.hpp>

static const int ntries = 1000;
static std::mt19937 rng;

using namespace piranha;

using cf_types = std::tuple<double, integer, rational>;
using expo_types = std::tuple<unsigned, integer>;

template <typename Cf, typename Expo>
class g_series_type : public series<Cf, monomial<Expo>, g_series_type<Cf, Expo>>
{
    typedef series<Cf, monomial<Expo>, g_series_type<Cf, Expo>> base;

public:
    template <typename Cf2>
    using rebind = g_series_type<Cf2, Expo>;
    g_series_type() = default;
    g_series_type(const g_series_type &) = default;
    g_series_type(g_series_type &&) = default;
    explicit g_series_type(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set.add(name);
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{Expo(1)}));
    }
    g_series_type &operator=(const g_series_type &) = default;
    g_series_type &operator=(g_series_type &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type, base)
};

// This is essentially the same as above, just a different type.
template <typename Cf, typename Expo>
class g_series_type2 : public series<Cf, monomial<Expo>, g_series_type2<Cf, Expo>>
{
public:
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
    PIRANHA_FORWARDING_CTOR(g_series_type2, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2, base)
    // Provide fake sin/cos methods to test math overloads.
    g_series_type2 sin() const
    {
        return g_series_type2(42);
    }
    g_series_type2 cos() const
    {
        return g_series_type2(-42);
    }
};

template <typename Cf, typename Key>
class g_series_type3 : public series<Cf, Key, g_series_type3<Cf, Key>>
{
    typedef series<Cf, Key, g_series_type3<Cf, Key>> base;

public:
    template <typename Cf2>
    using rebind = g_series_type3<Cf2, Key>;
    g_series_type3() = default;
    g_series_type3(const g_series_type3 &) = default;
    g_series_type3(g_series_type3 &&) = default;
    g_series_type3 &operator=(const g_series_type3 &) = default;
    g_series_type3 &operator=(g_series_type3 &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type3, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type3, base)
};

namespace piranha
{

template <typename Cf, typename Key>
class series_multiplier<g_series_type<Cf, Key>, void> : public base_series_multiplier<g_series_type<Cf, Key>>
{
    using base = base_series_multiplier<g_series_type<Cf, Key>>;
    template <typename T>
    using call_enabler = typename std::enable_if<key_is_multipliable<typename T::term_type::cf_type,
                                                                     typename T::term_type::key_type>::value,
                                                 int>::type;

public:
    using base::base;
    template <typename T = g_series_type<Cf, Key>, call_enabler<T> = 0>
    g_series_type<Cf, Key> operator()() const
    {
        return this->plain_multiplication();
    }
};

template <typename Cf, typename Key>
class series_multiplier<g_series_type2<Cf, Key>, void> : public base_series_multiplier<g_series_type2<Cf, Key>>
{
    using base = base_series_multiplier<g_series_type2<Cf, Key>>;
    template <typename T>
    using call_enabler = typename std::enable_if<key_is_multipliable<typename T::term_type::cf_type,
                                                                     typename T::term_type::key_type>::value,
                                                 int>::type;

public:
    using base::base;
    template <typename T = g_series_type2<Cf, Key>, call_enabler<T> = 0>
    g_series_type2<Cf, Key> operator()() const
    {
        return this->plain_multiplication();
    }
};

template <typename Cf, typename Key>
class series_multiplier<g_series_type3<Cf, Key>, void> : public base_series_multiplier<g_series_type3<Cf, Key>>
{
    using base = base_series_multiplier<g_series_type3<Cf, Key>>;
    template <typename T>
    using call_enabler = typename std::enable_if<key_is_multipliable<typename T::term_type::cf_type,
                                                                     typename T::term_type::key_type>::value,
                                                 int>::type;

public:
    using base::base;
    template <typename T = g_series_type3<Cf, Key>, call_enabler<T> = 0>
    g_series_type3<Cf, Key> operator()() const
    {
        return this->plain_multiplication();
    }
};
}

// Mock coefficient, not differentiable.
struct mock_cf {
    mock_cf();
    explicit mock_cf(const int &);
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

BOOST_AUTO_TEST_CASE(series_partial_test)
{
    init();
    {
        typedef g_series_type<rational, int> p_type1;
        p_type1 x1{"x"};
        BOOST_CHECK(is_differentiable<p_type1>::value);
        BOOST_CHECK((std::is_same<decltype(x1.partial("foo")), p_type1>::value));
        p_type1 x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL(math::partial(x, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x, "y"), 0);
        BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2), "x"), -8 * x);
        BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2) + y * x, "y"), x);
        BOOST_CHECK_EQUAL(math::partial(math::partial(-4 * x.pow(2), "x"), "x"), -8);
        BOOST_CHECK_EQUAL(math::partial(math::partial(math::partial(-4 * x.pow(2), "x"), "x"), "x"), 0);
        BOOST_CHECK_EQUAL(math::partial(-x + 1, "x"), -1);
        BOOST_CHECK_EQUAL(math::partial((1 + 2 * x).pow(10), "x"), 20 * (1 + 2 * x).pow(9));
        BOOST_CHECK_EQUAL(math::partial((1 + 2 * x + y).pow(10), "x"), 20 * (1 + 2 * x + y).pow(9));
        BOOST_CHECK_EQUAL(math::partial(x * (1 + 2 * x + y).pow(10), "x"),
                          20 * x * (1 + 2 * x + y).pow(9) + (1 + 2 * x + y).pow(10));
        BOOST_CHECK(math::partial((1 + 2 * x + y).pow(0), "x").empty());
        // Custom derivatives.
        p_type1::register_custom_derivative("x", [](const p_type1 &) { return p_type1{rational(1, 314)}; });
        BOOST_CHECK_EQUAL(math::partial(x, "x"), rational(1, 314));
        p_type1::register_custom_derivative("x", [](const p_type1 &) { return p_type1{rational(1, 315)}; });
        BOOST_CHECK_EQUAL(math::partial(x, "x"), rational(1, 315));
        p_type1::unregister_custom_derivative("x");
        p_type1::unregister_custom_derivative("x");
        BOOST_CHECK_EQUAL(math::partial(x, "x"), 1);
        // y as implicit function of x: y = x**2.
        p_type1::register_custom_derivative(
            "x", [x](const p_type1 &p) -> p_type1 { return p.partial("x") + math::partial(p, "y") * 2 * x; });
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1 + 2 * x);
        p_type1::unregister_custom_derivative("y");
        p_type1::unregister_custom_derivative("x");
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x + 2 * y, "y"), 2);
        p_type1::register_custom_derivative("x", [](const p_type1 &p) { return p.partial("x"); });
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x + y * x, "x"), y + 1);
        p_type1::register_custom_derivative(
            "x", [x](const p_type1 &p) -> p_type1 { return p.partial("x") + math::partial(p, "y") * 2 * x; });
        p_type1::register_custom_derivative("y", [](const p_type1 &p) -> p_type1 { return 2 * p; });
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1 + 4 * x * (x + y));
        BOOST_CHECK_EQUAL(math::partial(x + y, "y"), 2 * (x + y));
        p_type1::unregister_all_custom_derivatives();
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x + 3 * y, "y"), 3);
    }
    {
        typedef g_series_type<integer, rational> p_type2;
        using p_type2_diff = g_series_type<rational, rational>;
        p_type2 x2{"x"};
        BOOST_CHECK(is_differentiable<p_type2>::value);
        BOOST_CHECK((std::is_same<decltype(x2.partial("foo")), p_type2_diff>::value));
        p_type2 x{"x"}, y{"y"};
        BOOST_CHECK_EQUAL(math::partial(x, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x, "y"), 0);
        BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2), "x"), -8 * x);
        BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2) + y * x, "y"), x);
        BOOST_CHECK_EQUAL(math::partial(math::partial(-4 * x.pow(2), "x"), "x"), -8);
        BOOST_CHECK_EQUAL(math::partial(math::partial(math::partial(-4 * x.pow(2), "x"), "x"), "x"), 0);
        BOOST_CHECK_EQUAL(math::partial(-x + 1, "x"), -1);
        BOOST_CHECK_EQUAL(math::partial((1 + 2 * x).pow(10), "x"), 20 * (1 + 2 * x).pow(9));
        BOOST_CHECK_EQUAL(math::partial((1 + 2 * x + y).pow(10), "x"), 20 * (1 + 2 * x + y).pow(9));
        BOOST_CHECK_EQUAL(math::partial(x * (1 + 2 * x + y).pow(10), "x"),
                          20 * x * (1 + 2 * x + y).pow(9) + (1 + 2 * x + y).pow(10));
        BOOST_CHECK(math::partial((1 + 2 * x + y).pow(0), "x").empty());
        // Custom derivatives.
        p_type2::register_custom_derivative("x", [](const p_type2 &) { return p_type2_diff{rational(1, 314)}; });
        BOOST_CHECK_EQUAL(math::partial(x, "x"), rational(1, 314));
        p_type2::register_custom_derivative("x", [](const p_type2 &) { return p_type2_diff{rational(1, 315)}; });
        BOOST_CHECK_EQUAL(math::partial(x, "x"), rational(1, 315));
        p_type2::unregister_custom_derivative("x");
        BOOST_CHECK_EQUAL(math::partial(x, "x"), 1);
        // y as implicit function of x: y = x**2.
        p_type2::register_custom_derivative(
            "x", [x](const p_type2 &p) { return p.partial("x") + math::partial(p, "y") * 2 * x; });
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1 + 2 * x);
        p_type2::unregister_custom_derivative("y");
        p_type2::unregister_custom_derivative("x");
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x + 2 * y, "y"), 2);
        p_type2::register_custom_derivative("x", [](const p_type2 &p) { return p.partial("x"); });
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x + y * x, "x"), y + 1);
        p_type2::register_custom_derivative(
            "x", [x](const p_type2 &p) { return p.partial("x") + math::partial(p, "y") * 2 * x; });
        p_type2::register_custom_derivative("y", [](const p_type2 &p) { return 2 * p_type2_diff{p}; });
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1 + 4 * x * (x + y));
        BOOST_CHECK_EQUAL(math::partial(x + y, "y"), 2 * (x + y));
        p_type2::unregister_all_custom_derivatives();
        BOOST_CHECK_EQUAL(math::partial(x + y, "x"), 1);
        BOOST_CHECK_EQUAL(math::partial(x + 3 * y, "y"), 3);
    }
    // Check with mock_cf.
    BOOST_CHECK((!is_differentiable<g_series_type<mock_cf, rational>>::value));
    {
        using s0 = g_series_type<double, rational>;
        using ss0 = g_series_type<s0, rational>;
        // Series as coefficient.
        BOOST_CHECK((is_differentiable<ss0>::value));
        BOOST_CHECK_EQUAL(math::partial(s0{"y"} * ss0{"x"}, "y"), ss0{"x"});
        BOOST_CHECK_EQUAL(math::partial(s0{"y"} * ss0{"x"}, "x"), s0{"y"});
        BOOST_CHECK_EQUAL(math::partial(s0{"y"} * math::pow(ss0{"x"}, 5), "x"), 5 * s0{"y"} * math::pow(ss0{"x"}, 4));
    }
}

BOOST_AUTO_TEST_CASE(series_serialization_test)
{
    // Serialization test done with a randomly-generated series.
    typedef g_series_type<rational, int> p_type1;
    auto x = p_type1{"x"}, y = p_type1{"y"}, z = p_type1{"z"};
    std::uniform_int_distribution<int> int_dist(0, 5);
    std::uniform_int_distribution<unsigned> size_dist(0u, 10u);
    p_type1 tmp;
    for (int i = 0; i < ntries; ++i) {
        p_type1 p;
        const unsigned size = size_dist(rng);
        for (unsigned j = 0u; j < size; ++j) {
            p += math::pow(x, int_dist(rng)) * math::pow(y, int_dist(rng)) * math::pow(z, int_dist(rng));
        }
        p *= int_dist(rng);
        const auto div = int_dist(rng);
        if (div) {
            p /= div;
        }
        std::stringstream ss;
        {
            boost::archive::text_oarchive oa(ss);
            oa << p;
        }
        {
            boost::archive::text_iarchive ia(ss);
            ia >> tmp;
        }
        BOOST_CHECK_EQUAL(tmp, p);
    }
}

struct mock_key {
    mock_key() = default;
    mock_key(const mock_key &) = default;
    mock_key(mock_key &&) noexcept;
    mock_key &operator=(const mock_key &) = default;
    mock_key &operator=(mock_key &&) noexcept;
    mock_key(const symbol_set &);
    bool operator==(const mock_key &) const;
    bool operator!=(const mock_key &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    mock_key merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    mock_key trim(const symbol_set &, const symbol_set &) const;
};

namespace std
{

template <>
struct hash<mock_key> {
    std::size_t operator()(const mock_key &) const;
};
}

BOOST_AUTO_TEST_CASE(series_evaluate_test)
{
    typedef g_series_type<rational, int> p_type1;
    typedef std::unordered_map<std::string, rational> dict_type;
    typedef std::unordered_map<std::string, int> dict_type_int;
    typedef std::unordered_map<std::string, long> dict_type_long;
    BOOST_CHECK((is_evaluable<p_type1, rational>::value));
    BOOST_CHECK((is_evaluable<p_type1, integer>::value));
    BOOST_CHECK((is_evaluable<p_type1, int>::value));
    BOOST_CHECK((is_evaluable<p_type1, long>::value));
    BOOST_CHECK((std::is_same<rational, decltype(p_type1{}.evaluate(dict_type_int{}))>::value));
    BOOST_CHECK((std::is_same<rational, decltype(p_type1{}.evaluate(dict_type_long{}))>::value));
    BOOST_CHECK_EQUAL(p_type1{}.evaluate(dict_type{}), 0);
    p_type1 x{"x"}, y{"y"};
    BOOST_CHECK_THROW(x.evaluate(dict_type{}), std::invalid_argument);
    BOOST_CHECK_EQUAL(x.evaluate(dict_type{{"x", rational(1)}}), 1);
    BOOST_CHECK_THROW((x + (2 * y).pow(3)).evaluate(dict_type{{"x", rational(1)}}), std::invalid_argument);
    BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type{{"x", rational(1)}, {"y", rational(2, 3)}}),
                      rational(1) + (2 * rational(2, 3)).pow(3));
    BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type{{"x", rational(1)}, {"y", rational(2, 3)}}),
                      math::evaluate(x + (2 * y).pow(3), dict_type{{"x", rational(1)}, {"y", rational(2, 3)}}));
    BOOST_CHECK((std::is_same<decltype(p_type1{}.evaluate(dict_type{})), rational>::value));
    typedef std::unordered_map<std::string, real> dict_type2;
    BOOST_CHECK((is_evaluable<p_type1, real>::value));
    BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type2{{"x", real(1.234)}, {"y", real(-5.678)}, {"z", real()}}),
                      real(1.234) + math::pow(2 * real(-5.678), 3));
    BOOST_CHECK_EQUAL(
        (x + (2 * y).pow(3)).evaluate(dict_type2{{"x", real(1.234)}, {"y", real(-5.678)}, {"z", real()}}),
        math::evaluate(x + math::pow(2 * y, 3), dict_type2{{"x", real(1.234)}, {"y", real(-5.678)}, {"z", real()}}));
    BOOST_CHECK((std::is_same<decltype(p_type1{}.evaluate(dict_type2{})), real>::value));
    typedef std::unordered_map<std::string, double> dict_type3;
    BOOST_CHECK((is_evaluable<p_type1, double>::value));
    BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type3{{"x", 1.234}, {"y", -5.678}, {"z", 0.0001}}),
                      1.234 + math::pow(2 * -5.678, 3));
    BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type3{{"x", 1.234}, {"y", -5.678}, {"z", 0.0001}}),
                      math::evaluate(x + math::pow(2 * y, 3), dict_type3{{"x", 1.234}, {"y", -5.678}, {"z", 0.0001}}));
    BOOST_CHECK((std::is_same<decltype(p_type1{}.evaluate(dict_type3{})), double>::value));
    BOOST_CHECK((!is_evaluable<g_series_type3<double, mock_key>, double>::value));
    // NOTE: this used to be true before we changed the ctor from int of mock_cf to explicit.
    BOOST_CHECK((!is_evaluable<g_series_type3<mock_cf, monomial<int>>, double>::value));
    BOOST_CHECK((!is_evaluable<g_series_type3<mock_cf, mock_key>, double>::value));
    BOOST_CHECK((is_evaluable<g_series_type3<double, monomial<int>>, double>::value));
    // Check the syntax from initializer list with explicit template parameter.
    BOOST_CHECK_EQUAL(p_type1{}.evaluate<int>({{"foo", 4.}}), 0);
    BOOST_CHECK_EQUAL(p_type1{}.evaluate<double>({{"foo", 4.}, {"bar", 7}}), 0);
    BOOST_CHECK_EQUAL(math::evaluate<int>(p_type1{}, {{"foo", 4.}}), 0);
    BOOST_CHECK_EQUAL(math::evaluate<double>(p_type1{}, {{"foo", 4.}, {"bar", 7}}), 0);
}

template <typename Expo>
class g_series_type_nr : public series<float, monomial<Expo>, g_series_type_nr<Expo>>
{
    typedef series<float, monomial<Expo>, g_series_type_nr<Expo>> base;

public:
    g_series_type_nr() = default;
    g_series_type_nr(const g_series_type_nr &) = default;
    g_series_type_nr(g_series_type_nr &&) = default;
    g_series_type_nr &operator=(const g_series_type_nr &) = default;
    g_series_type_nr &operator=(g_series_type_nr &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type_nr, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nr, base)
};

template <typename Expo>
class g_series_type_nr2 : public series<short, monomial<Expo>, g_series_type_nr2<Expo>>
{
    typedef series<short, monomial<Expo>, g_series_type_nr2<Expo>> base;

public:
    template <typename Expo2>
    using rebind = g_series_type_nr2<Expo2>;
    g_series_type_nr2() = default;
    g_series_type_nr2(const g_series_type_nr2 &) = default;
    g_series_type_nr2(g_series_type_nr2 &&) = default;
    g_series_type_nr2 &operator=(const g_series_type_nr2 &) = default;
    g_series_type_nr2 &operator=(g_series_type_nr2 &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type_nr2, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nr2, base)
};

template <typename Expo>
class g_series_type_nr3 : public series<float, monomial<Expo>, g_series_type_nr3<Expo>>
{
    typedef series<float, monomial<Expo>, g_series_type_nr3<Expo>> base;

public:
    template <typename Expo2>
    using rebind = void;
    g_series_type_nr3() = default;
    g_series_type_nr3(const g_series_type_nr3 &) = default;
    g_series_type_nr3(g_series_type_nr3 &&) = default;
    g_series_type_nr3 &operator=(const g_series_type_nr3 &) = default;
    g_series_type_nr3 &operator=(g_series_type_nr3 &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type_nr3, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nr3, base)
};

BOOST_AUTO_TEST_CASE(series_series_is_rebindable_test)
{
    typedef g_series_type<rational, int> p_type1;
    BOOST_CHECK((series_is_rebindable<p_type1, int>::value));
    BOOST_CHECK((std::is_same<series_rebind<p_type1, int>, g_series_type<int, int>>::value));
    BOOST_CHECK((series_is_rebindable<p_type1, rational>::value));
    BOOST_CHECK((std::is_same<series_rebind<p_type1, rational>, p_type1>::value));
    BOOST_CHECK((std::is_same<series_rebind<p_type1 &, rational const>, p_type1>::value));
    BOOST_CHECK((series_is_rebindable<p_type1, p_type1>::value));
    BOOST_CHECK((series_is_rebindable<p_type1 &, p_type1>::value));
    BOOST_CHECK((series_is_rebindable<p_type1 &, const p_type1>::value));
    BOOST_CHECK((std::is_same<series_rebind<p_type1, p_type1>, g_series_type<p_type1, int>>::value));
    typedef g_series_type_nr<int> p_type_nr;
    BOOST_CHECK((!series_is_rebindable<p_type_nr, unsigned>::value));
    BOOST_CHECK((!series_is_rebindable<p_type_nr, integer>::value));
    BOOST_CHECK((!series_is_rebindable<p_type_nr &, unsigned const>::value));
    BOOST_CHECK((!series_is_rebindable<p_type_nr &&, const integer &>::value));
    typedef g_series_type_nr2<int> p_type_nr2;
    BOOST_CHECK((!series_is_rebindable<p_type_nr2, unsigned>::value));
    BOOST_CHECK((!series_is_rebindable<p_type_nr2, integer>::value));
    typedef g_series_type_nr3<int> p_type_nr3;
    BOOST_CHECK((!series_is_rebindable<p_type_nr3, unsigned>::value));
    BOOST_CHECK((!series_is_rebindable<p_type_nr3, integer>::value));
    // Check when the requirements on the input types are not satisfied.
    BOOST_CHECK((!series_is_rebindable<p_type1, std::string>::value));
    BOOST_CHECK((!series_is_rebindable<p_type1, std::vector<std::string>>::value));
    BOOST_CHECK((!series_is_rebindable<p_type1, std::vector<std::string> &>::value));
    BOOST_CHECK((!series_is_rebindable<p_type1, const std::vector<std::string> &>::value));
    BOOST_CHECK((!series_is_rebindable<p_type1, std::vector<std::string> &&>::value));
    BOOST_CHECK((!series_is_rebindable<std::string, std::vector<std::string>>::value));
    BOOST_CHECK((!series_is_rebindable<const std::string &, std::vector<std::string>>::value));
    BOOST_CHECK((!series_is_rebindable<const std::string &, std::vector<std::string> &&>::value));
}

BOOST_AUTO_TEST_CASE(series_series_recursion_index_test)
{
    BOOST_CHECK_EQUAL(series_recursion_index<int>::value, 0u);
    BOOST_CHECK_EQUAL(series_recursion_index<double>::value, 0u);
    BOOST_CHECK_EQUAL(series_recursion_index<float>::value, 0u);
    BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<rational, int>>::value), 1u);
    BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<float, int>>::value), 1u);
    BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<double, int>>::value), 1u);
    BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<double, int>, int>>::value), 2u);
    BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<double, int>, long>>::value), 2u);
    BOOST_CHECK_EQUAL(
        (series_recursion_index<g_series_type<g_series_type<g_series_type<double, int>, int>, long>>::value), 3u);
    BOOST_CHECK_EQUAL(
        (series_recursion_index<g_series_type<g_series_type<g_series_type<rational, int>, int>, long>>::value), 3u);
    BOOST_CHECK_EQUAL(
        (series_recursion_index<g_series_type<g_series_type<g_series_type<rational, int>, int>, long> &>::value), 3u);
    BOOST_CHECK_EQUAL(
        (series_recursion_index<g_series_type<g_series_type<g_series_type<rational, int>, int>, long> const>::value),
        3u);
    BOOST_CHECK_EQUAL(
        (series_recursion_index<g_series_type<g_series_type<g_series_type<rational, int>, int>, long> const &>::value),
        3u);
}

PIRANHA_DECLARE_HAS_TYPEDEF(type);

template <typename T, typename U>
using binary_series_op_return_type = detail::binary_series_op_return_type<T, U, 0>;

BOOST_AUTO_TEST_CASE(series_binary_series_op_return_type_test)
{
    // Check missing type in case both operands are not series.
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<int, int>>::value));
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<int, double>>::value));
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<float, double>>::value));
    // Case 0.
    // NOTE: this cannot fail in any way as we require coefficients to be addable in is_cf.
    typedef g_series_type<rational, int> p_type1;
    BOOST_CHECK((std::is_same<p_type1, binary_series_op_return_type<p_type1, p_type1>::type>::value));
    // Case 1 and 2.
    typedef g_series_type<double, int> p_type2;
    BOOST_CHECK((std::is_same<p_type2, binary_series_op_return_type<p_type2, p_type1>::type>::value));
    BOOST_CHECK((std::is_same<p_type2, binary_series_op_return_type<p_type1, p_type2>::type>::value));
    // mock_cf supports only multiplication vs mock_cf.
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<double, int>,
                                                                g_series_type<mock_cf, int>>>::value));
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<mock_cf, int>,
                                                                g_series_type<double, int>>>::value));
    // Case 3.
    typedef g_series_type<short, int> p_type3;
    BOOST_CHECK((std::is_same<g_series_type<int, int>, binary_series_op_return_type<p_type3, p_type3>::type>::value));
    typedef g_series_type<char, int> p_type4;
    BOOST_CHECK((std::is_same<g_series_type<int, int>, binary_series_op_return_type<p_type3, p_type4>::type>::value));
    BOOST_CHECK((std::is_same<g_series_type<int, int>, binary_series_op_return_type<p_type4, p_type3>::type>::value));
    // Wrong rebind implementations.
    BOOST_CHECK(
        (!has_typedef_type<binary_series_op_return_type<g_series_type_nr2<int>, g_series_type<char, int>>>::value));
    BOOST_CHECK(
        (!has_typedef_type<binary_series_op_return_type<g_series_type<char, int>, g_series_type_nr2<int>>>::value));
    // Case 4 and 6.
    BOOST_CHECK((std::is_same<p_type2, binary_series_op_return_type<p_type2, int>::type>::value));
    BOOST_CHECK((std::is_same<p_type2, binary_series_op_return_type<int, p_type2>::type>::value));
    // mock_cf does not support multiplication with int.
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<mock_cf, int>, int>>::value));
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<int, g_series_type<mock_cf, int>>>::value));
    // Case 5 and 7.
    BOOST_CHECK((std::is_same<p_type2, binary_series_op_return_type<p_type3, double>::type>::value));
    BOOST_CHECK((std::is_same<p_type2, binary_series_op_return_type<double, p_type3>::type>::value));
    BOOST_CHECK((std::is_same<g_series_type<int, int>, binary_series_op_return_type<p_type4, short>::type>::value));
    BOOST_CHECK((std::is_same<g_series_type<int, int>, binary_series_op_return_type<short, p_type4>::type>::value));
    // These need rebinding, but rebind is not supported.
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type_nr<int>, double>>::value));
    BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<double, g_series_type_nr<int>>>::value));
    // Wrong implementation of rebind.
    BOOST_CHECK(
        (!has_typedef_type<binary_series_op_return_type<g_series_type_nr2<char>, g_series_type<char, char>>>::value));
    BOOST_CHECK(
        (!has_typedef_type<binary_series_op_return_type<g_series_type<char, char>, g_series_type_nr2<char>>>::value));
    // Same coefficients, amibguity in series type.
    BOOST_CHECK(
        (!has_typedef_type<binary_series_op_return_type<g_series_type_nr<int>, g_series_type<float, int>>>::value));
}

struct arithmetics_add_tag {
};

namespace piranha
{
template <>
class debug_access<arithmetics_add_tag>
{
public:
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            typedef g_series_type2<Cf, Expo> p_type2;
            typedef g_series_type<int, Expo> p_type3;
            // Binary add first.
            // Some type checks - these are not addable as they result in an ambiguity
            // between two series with same coefficient but different series types.
            BOOST_CHECK((!is_addable<p_type1, p_type2>::value));
            BOOST_CHECK((!is_addable<p_type2, p_type1>::value));
            BOOST_CHECK((!is_addable_in_place<p_type1, p_type2>::value));
            BOOST_CHECK((!is_addable_in_place<p_type2, p_type1>::value));
            // Various subcases of case 0.
            p_type1 x{"x"}, y{"y"};
            // No need to merge args.
            auto tmp = x + x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Try with moves on both sides.
            tmp = p_type1{x} + x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            tmp = x + p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            tmp = p_type1{x} + p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Check that move erases.
            auto x_copy(x);
            tmp = std::move(x) + x_copy;
            BOOST_CHECK_EQUAL(x.size(), 0u);
            x = x_copy;
            tmp = x_copy + std::move(x);
            BOOST_CHECK_EQUAL(x.size(), 0u);
            x = x_copy;
            // A few self move tests.
            tmp = std::move(x) + std::move(x);
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            x = p_type1{"x"};
            tmp = x + std::move(x);
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            x = p_type1{"x"};
            tmp = std::move(x) + x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            x = p_type1{"x"};
            // Now with merging.
            tmp = x + y;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            auto it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // With moves.
            tmp = p_type1{x} + y;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x + p_type1{y};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Test the swapping of operands when one series is larger than the other.
            tmp = (x + y) + x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x + (y + x);
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Some tests for case 1/4.
            tmp = x + p_type3{"y"};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x + (p_type3{"y"} + p_type3{"x"});
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x + 1;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 1u);
            ++it;
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Symmetric of the previous case.
            tmp = p_type3{"y"} + x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = (p_type3{"y"} + p_type3{"x"}) + x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = 1 + x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 1u);
            ++it;
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Case 3/5 and symmetric.
            using p_type4 = g_series_type<g_series_type<int, Expo>, Expo>;
            using p_type5 = g_series_type<double, Expo>;
            auto tmp2 = p_type4{"x"} + p_type5{"y"};
            BOOST_CHECK_EQUAL(tmp2.size(), 2u);
            BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<g_series_type<double, Expo>, Expo>>::value));
            auto it2 = tmp2.m_container.begin();
            BOOST_CHECK((it2->m_cf == g_series_type<double, Expo>{"y"} || it2->m_cf == 1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            ++it2;
            BOOST_CHECK((it2->m_cf == g_series_type<double, Expo>{"y"} || it2->m_cf == 1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            tmp2 = p_type5{"y"} + p_type4{"x"};
            BOOST_CHECK_EQUAL(tmp2.size(), 2u);
            BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<g_series_type<double, Expo>, Expo>>::value));
            it2 = tmp2.m_container.begin();
            BOOST_CHECK((it2->m_cf == g_series_type<double, Expo>{"y"} || it2->m_cf == 1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            ++it2;
            BOOST_CHECK((it2->m_cf == g_series_type<double, Expo>{"y"} || it2->m_cf == 1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            // Now in-place.
            // Case 0.
            tmp = x;
            tmp += x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Move.
            tmp = x;
            tmp += p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Check that a move really happens.
            tmp = x;
            tmp += std::move(x);
            // NOTE: here the symbol set has still size 1 as it does not get moved
            // (it gets moved only when operands are swapped because of difference
            // in sizes or because it's a sub operation).
            BOOST_CHECK_EQUAL(x.size(), 0u);
            x = p_type1{"x"};
            // Move self.
            tmp += std::move(tmp);
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1) + Cf(1) + Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Now with merging.
            tmp = x;
            tmp += y;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // With moves.
            tmp = x;
            tmp += p_type1{y};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Test the swapping of operands when one series is larger than the other.
            tmp = x + y;
            tmp += x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp += y + x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Some tests for case 1/4.
            tmp = x;
            tmp += p_type3{"y"};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp += p_type3{"y"} + p_type3{"x"};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp += 1;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 1u);
            ++it;
            BOOST_CHECK(it->m_cf == 1);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Symmetric of the previous case.
            p_type3 tmp3{"y"};
            tmp3 += x;
            BOOST_CHECK_EQUAL(tmp3.size(), 2u);
            auto it3 = tmp3.m_container.begin();
            BOOST_CHECK(it3->m_cf == 1);
            BOOST_CHECK(it3->m_key.size() == 2u);
            ++it3;
            BOOST_CHECK(it3->m_cf == 1);
            BOOST_CHECK(it3->m_key.size() == 2u);
            BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp3 += p_type3{"y"} + p_type3{"x"};
            tmp3 += x;
            BOOST_CHECK_EQUAL(tmp3.size(), 2u);
            it3 = tmp3.m_container.begin();
            BOOST_CHECK(it3->m_cf == 2 || it3->m_cf == 3);
            BOOST_CHECK(it3->m_key.size() == 2u);
            ++it3;
            BOOST_CHECK(it3->m_cf == 2 || it3->m_cf == 3);
            BOOST_CHECK(it3->m_key.size() == 2u);
            BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Case 3/5.
            auto tmp4 = p_type4{"x"};
            tmp4 += p_type5{"y"};
            BOOST_CHECK_EQUAL(tmp4.size(), 2u);
            auto it4 = tmp4.m_container.begin();
            BOOST_CHECK((std::is_same<decltype(it4->m_cf), g_series_type<int, Expo>>::value));
            BOOST_CHECK((it4->m_cf == g_series_type<int, Expo>{"y"} || it4->m_cf == 1));
            BOOST_CHECK(it4->m_key.size() == 1u);
            ++it4;
            BOOST_CHECK((it4->m_cf == g_series_type<int, Expo>{"y"} || it4->m_cf == 1));
            BOOST_CHECK(it4->m_key.size() == 1u);
            BOOST_CHECK((tmp4.m_symbol_set == symbol_set{symbol{"x"}}));
            // Check with scalar on the left.
            BOOST_CHECK((!is_addable_in_place<int, p_type1>::value));
            BOOST_CHECK((!is_addable_in_place<int, p_type2>::value));
            BOOST_CHECK((!is_addable_in_place<int, p_type3>::value));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};
}

typedef debug_access<arithmetics_add_tag> arithmetics_add_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_add_test)
{
    // Functional testing.
    tuple_for_each(cf_types{}, arithmetics_add_tester());
    // Type testing for binary addition.
    typedef g_series_type<rational, int> p_type1;
    typedef g_series_type<int, rational> p_type2;
    typedef g_series_type<short, rational> p_type3;
    typedef g_series_type<char, rational> p_type4;
    // First let's check the output type.
    // Case 0.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} + p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} + p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type2{} + p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type3{} + p_type4{})>::value));
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type4{} + p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} + 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type3{} + 0)>::value));
    // Case 6.
    BOOST_CHECK((std::is_same<p_type1, decltype(0 + p_type1{})>::value));
    // Case 7.
    BOOST_CHECK((std::is_same<p_type2, decltype(0 + p_type3{})>::value));
    // Check non-addable series.
    typedef g_series_type2<rational, int> p_type5;
    BOOST_CHECK((!is_addable<p_type1, p_type5>::value));
    BOOST_CHECK((!is_addable<p_type5, p_type1>::value));
    // Check coefficient series.
    typedef g_series_type<p_type1, int> p_type11;
    typedef g_series_type<p_type2, rational> p_type22;
    typedef g_series_type<p_type1, rational> p_type21;
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type1{} + p_type11{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type11{} + p_type1{})>::value));
    BOOST_CHECK((std::is_same<p_type21, decltype(p_type1{} + p_type22{})>::value));
    BOOST_CHECK((std::is_same<p_type21, decltype(p_type22{} + p_type1{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type11{} + p_type22{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type22{} + p_type11{})>::value));
    // Type testing for in-place addition.
    // Case 0.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() += p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() += p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<p_type2 &, decltype(std::declval<p_type2 &>() += p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<p_type3 &, decltype(std::declval<p_type3 &>() += p_type4{})>::value));
    BOOST_CHECK((std::is_same<p_type4 &, decltype(std::declval<p_type4 &>() += p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() += 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type3 &, decltype(std::declval<p_type3 &>() += 0)>::value));
    // Cases 6 and 7 do not make sense at the moment.
    BOOST_CHECK((!is_addable_in_place<int, p_type3>::value));
    BOOST_CHECK((!is_addable_in_place<p_type1, p_type11>::value));
    // Checks for coefficient series.
    p_type11 tmp;
    BOOST_CHECK((std::is_same<p_type11 &, decltype(tmp += p_type1{})>::value));
    p_type22 tmp2;
    BOOST_CHECK((std::is_same<p_type22 &, decltype(tmp2 += p_type1{})>::value));
}

struct arithmetics_sub_tag {
};

namespace piranha
{
template <>
class debug_access<arithmetics_sub_tag>
{
public:
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            typedef g_series_type2<Cf, Expo> p_type2;
            typedef g_series_type<int, Expo> p_type3;
            // Binary sub first.
            // Some type checks - these are not subtractable as they result in an ambiguity
            // between two series with same coefficient but different series types.
            BOOST_CHECK((!is_subtractable<p_type1, p_type2>::value));
            BOOST_CHECK((!is_subtractable<p_type2, p_type1>::value));
            BOOST_CHECK((!is_subtractable_in_place<p_type1, p_type2>::value));
            BOOST_CHECK((!is_subtractable_in_place<p_type2, p_type1>::value));
            // Various subcases of case 0.
            p_type1 x{"x"}, y{"y"}, x2 = x + x;
            // No need to merge args.
            auto tmp = x2 - x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Check going to zero.
            tmp = x - x;
            BOOST_CHECK(tmp.size() == 0u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Try with moves on both sides.
            tmp = p_type1{x} - x2;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(-1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            tmp = x2 - p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            tmp = p_type1{x2} - p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Check that move erases.
            auto x_copy(x);
            tmp = std::move(x) - x_copy;
            BOOST_CHECK_EQUAL(x.size(), 0u);
            x = x_copy;
            tmp = x_copy - std::move(x);
            BOOST_CHECK_EQUAL(x.size(), 0u);
            x = x_copy;
            // Self move tests.
            tmp = std::move(x) - std::move(x);
            BOOST_CHECK_EQUAL(tmp.size(), 0u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            x = p_type1{"x"};
            tmp = x - std::move(x);
            BOOST_CHECK_EQUAL(tmp.size(), 0u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            x = p_type1{"x"};
            tmp = std::move(x) - x;
            BOOST_CHECK_EQUAL(tmp.size(), 0u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            x = p_type1{"x"};
            // Now with merging.
            tmp = x - y;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            auto it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // With moves.
            tmp = p_type1{x} - y;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x - p_type1{y};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Test the swapping of operands when one series is larger than the other.
            tmp = (x2 - y) - x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x2 - (y - x);
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Some tests for case 1/4.
            tmp = x - p_type3{"y"};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x2 - (p_type3{"y"} - p_type3{"x"});
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x - 1;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 1u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Symmetric of the previous case.
            tmp = p_type3{"y"} - x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = (p_type3{"y"} - p_type3{"x"}) - x2;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -3);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -3);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = 1 - x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 1u);
            ++it;
            BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Case 3/5 and symmetric.
            using p_type4 = g_series_type<g_series_type<int, Expo>, Expo>;
            using p_type5 = g_series_type<double, Expo>;
            auto tmp2 = p_type4{"x"} - p_type5{"y"};
            BOOST_CHECK_EQUAL(tmp2.size(), 2u);
            BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<g_series_type<double, Expo>, Expo>>::value));
            auto it2 = tmp2.m_container.begin();
            BOOST_CHECK((it2->m_cf == -g_series_type<double, Expo>{"y"} || it2->m_cf == 1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            ++it2;
            BOOST_CHECK((it2->m_cf == -g_series_type<double, Expo>{"y"} || it2->m_cf == 1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            tmp2 = p_type5{"y"} - p_type4{"x"};
            BOOST_CHECK_EQUAL(tmp2.size(), 2u);
            BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<g_series_type<double, Expo>, Expo>>::value));
            it2 = tmp2.m_container.begin();
            BOOST_CHECK((it2->m_cf == g_series_type<double, Expo>{"y"} || it2->m_cf == -1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            ++it2;
            BOOST_CHECK((it2->m_cf == g_series_type<double, Expo>{"y"} || it2->m_cf == -1));
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            // Now in-place.
            // Case 0.
            tmp = x2;
            tmp -= x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Check that a move really happens.
            tmp = x;
            tmp -= std::move(x);
            BOOST_CHECK_EQUAL(x.size(), 0u);
            x = p_type1{"x"};
            // Move.
            tmp = x2;
            tmp -= p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Now with merging.
            tmp = x;
            tmp -= y;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // With moves.
            tmp = x;
            tmp -= p_type1{y};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Move self.
            tmp -= std::move(tmp);
            BOOST_CHECK_EQUAL(tmp.size(), 0u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Test the swapping of operands when one series is larger than the other.
            tmp = x2 - y;
            tmp -= x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp -= y - x2;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Some tests for case 1/4.
            tmp = x;
            tmp -= p_type3{"y"};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x2;
            tmp -= p_type3{"y"} - p_type3{"x"};
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp -= 1;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 1u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Symmetric of the previous case.
            p_type3 tmp3{"y"};
            tmp3 -= x;
            BOOST_CHECK_EQUAL(tmp3.size(), 2u);
            auto it3 = tmp3.m_container.begin();
            BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
            BOOST_CHECK(it3->m_key.size() == 2u);
            ++it3;
            BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
            BOOST_CHECK(it3->m_key.size() == 2u);
            BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp3 = p_type3{"x"};
            tmp3 -= p_type3{"y"} - p_type3{"x"};
            tmp3 -= x;
            BOOST_CHECK_EQUAL(tmp3.size(), 2u);
            it3 = tmp3.m_container.begin();
            BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
            BOOST_CHECK(it3->m_key.size() == 2u);
            ++it3;
            BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
            BOOST_CHECK(it3->m_key.size() == 2u);
            BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Case 3/5.
            auto tmp4 = p_type4{"x"};
            tmp4 -= p_type5{"y"};
            BOOST_CHECK_EQUAL(tmp4.size(), 2u);
            auto it4 = tmp4.m_container.begin();
            BOOST_CHECK((std::is_same<decltype(it4->m_cf), g_series_type<int, Expo>>::value));
            BOOST_CHECK((it4->m_cf == -g_series_type<int, Expo>{"y"} || it4->m_cf == 1));
            BOOST_CHECK(it4->m_key.size() == 1u);
            ++it4;
            BOOST_CHECK((it4->m_cf == -g_series_type<int, Expo>{"y"} || it4->m_cf == 1));
            BOOST_CHECK(it4->m_key.size() == 1u);
            BOOST_CHECK((tmp4.m_symbol_set == symbol_set{symbol{"x"}}));
            // Check with scalar on the left.
            BOOST_CHECK((!is_subtractable_in_place<int, p_type1>::value));
            BOOST_CHECK((!is_subtractable_in_place<int, p_type2>::value));
            BOOST_CHECK((!is_subtractable_in_place<int, p_type3>::value));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};
}

typedef debug_access<arithmetics_sub_tag> arithmetics_sub_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_sub_test)
{
    // Functional testing.
    tuple_for_each(cf_types{}, arithmetics_sub_tester());
    // Type testing for binary subtraction.
    typedef g_series_type<rational, int> p_type1;
    typedef g_series_type<int, rational> p_type2;
    typedef g_series_type<short, rational> p_type3;
    typedef g_series_type<char, rational> p_type4;
    // First let's check the output type.
    // Case 0.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} - p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} - p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type2{} - p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type3{} - p_type4{})>::value));
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type4{} - p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} - 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type3{} - 0)>::value));
    // Case 6.
    BOOST_CHECK((std::is_same<p_type1, decltype(0 - p_type1{})>::value));
    // Case 7.
    BOOST_CHECK((std::is_same<p_type2, decltype(0 - p_type3{})>::value));
    // Check non-subtractable series.
    typedef g_series_type2<rational, int> p_type5;
    BOOST_CHECK((!is_subtractable<p_type1, p_type5>::value));
    BOOST_CHECK((!is_subtractable<p_type5, p_type1>::value));
    // Check coefficient series.
    typedef g_series_type<p_type1, int> p_type11;
    typedef g_series_type<p_type2, rational> p_type22;
    typedef g_series_type<p_type1, rational> p_type21;
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type1{} - p_type11{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type11{} - p_type1{})>::value));
    BOOST_CHECK((std::is_same<p_type21, decltype(p_type1{} - p_type22{})>::value));
    BOOST_CHECK((std::is_same<p_type21, decltype(p_type22{} - p_type1{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type11{} - p_type22{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type22{} - p_type11{})>::value));
    // Type testing for in-place subtraction.
    // Case 0.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() -= p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() -= p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<p_type2 &, decltype(std::declval<p_type2 &>() -= p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<p_type3 &, decltype(std::declval<p_type3 &>() -= p_type4{})>::value));
    BOOST_CHECK((std::is_same<p_type4 &, decltype(std::declval<p_type4 &>() -= p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() -= 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type3 &, decltype(std::declval<p_type3 &>() -= 0)>::value));
    // Cases 6 and 7 do not make sense at the moment.
    BOOST_CHECK((!is_subtractable_in_place<int, p_type3>::value));
    BOOST_CHECK((!is_subtractable_in_place<p_type1, p_type11>::value));
    // Checks for coefficient series.
    p_type11 tmp;
    BOOST_CHECK((std::is_same<p_type11 &, decltype(tmp -= p_type1{})>::value));
    p_type22 tmp2;
    BOOST_CHECK((std::is_same<p_type22 &, decltype(tmp2 -= p_type1{})>::value));
}

struct arithmetics_mul_tag {
};

namespace piranha
{
template <>
class debug_access<arithmetics_mul_tag>
{
public:
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            typedef g_series_type2<Cf, Expo> p_type2;
            typedef g_series_type<int, Expo> p_type3;
            // Binary mul first.
            // Some type checks - these are not multipliable as they result in an ambiguity
            // between two series with same coefficient but different series types.
            BOOST_CHECK((!is_multipliable<p_type1, p_type2>::value));
            BOOST_CHECK((!is_multipliable<p_type2, p_type1>::value));
            BOOST_CHECK((!is_multipliable_in_place<p_type1, p_type2>::value));
            BOOST_CHECK((!is_multipliable_in_place<p_type2, p_type1>::value));
            // Various subcases of case 0.
            p_type1 x{"x"}, y{"y"};
            // No need to merge args.
            auto tmp = 2 * x * x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2) * Cf(1));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Try with moves on both sides.
            tmp = 3 * p_type1{x} * 2 * x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(3) * Cf(2));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            tmp = 2 * x * 3 * p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2) * Cf(3));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Now with merging.
            tmp = x * y;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            auto it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // With moves.
            tmp = p_type1{x} * y;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x * p_type1{y};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(1) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Test the swapping of operands when one series is larger than the other.
            tmp = (x + y) * 2 * x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x * (2 * y + 2 * x);
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Some tests for case 1/4.
            tmp = 3 * x * p_type3{"y"};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = 3 * x * (p_type3{"y"} + p_type3{"x"});
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x * 2;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 2);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Symmetric of the previous case.
            tmp = p_type3{"y"} * x * 3;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = (p_type3{"y"} + p_type3{"x"}) * 4 * x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 4);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 4);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = -2 * x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == -2);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Case 3/5 and symmetric.
            using p_type4 = g_series_type<g_series_type<int, Expo>, Expo>;
            using p_type5 = g_series_type<double, Expo>;
            auto tmp2 = p_type4{"x"} * p_type5{"y"} * -1;
            BOOST_CHECK_EQUAL(tmp2.size(), 1u);
            BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<g_series_type<double, Expo>, Expo>>::value));
            auto it2 = tmp2.m_container.begin();
            BOOST_CHECK((it2->m_cf == -g_series_type<double, Expo>{"y"}));
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            tmp2 = p_type5{"y"} * p_type4{"x"} * 2;
            BOOST_CHECK_EQUAL(tmp2.size(), 1u);
            BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<g_series_type<double, Expo>, Expo>>::value));
            it2 = tmp2.m_container.begin();
            BOOST_CHECK((it2->m_cf == 2 * g_series_type<double, Expo>{"y"}));
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            // Now in-place.
            // Case 0.
            tmp = 2 * x;
            tmp *= x;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Move.
            tmp = 2 * x;
            tmp *= p_type1{x};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2));
            BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
            BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
            // Now with merging.
            tmp = -3 * x;
            tmp *= y;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(-3));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // With moves.
            tmp = 4 * x;
            tmp *= p_type1{y};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(4));
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Test the swapping of operands when one series is larger than the other.
            tmp = 4 * (x + y);
            tmp *= x;
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 4);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 4);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp *= 3 * (y + x);
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Some tests for case 1/4.
            tmp = 4 * x;
            tmp *= p_type3{"y"};
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 4);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp *= -4 * (p_type3{"y"} + p_type3{"x"});
            BOOST_CHECK_EQUAL(tmp.size(), 2u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == -4);
            BOOST_CHECK(it->m_key.size() == 2u);
            ++it;
            BOOST_CHECK(it->m_cf == -4);
            BOOST_CHECK(it->m_key.size() == 2u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp = x;
            tmp *= 3;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == 3);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Symmetric of the previous case.
            p_type3 tmp3{"y"};
            tmp3 *= -4 * x;
            BOOST_CHECK_EQUAL(tmp3.size(), 1u);
            auto it3 = tmp3.m_container.begin();
            BOOST_CHECK(it3->m_cf == -4);
            BOOST_CHECK(it3->m_key.size() == 2u);
            BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            tmp3 *= p_type3{"y"} + p_type3{"x"};
            tmp3 *= -x;
            BOOST_CHECK_EQUAL(tmp3.size(), 2u);
            it3 = tmp3.m_container.begin();
            BOOST_CHECK(it3->m_cf == 4);
            BOOST_CHECK(it3->m_key.size() == 2u);
            BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"}, symbol{"y"}}));
            // Case 3/5.
            auto tmp4 = p_type4{"x"};
            tmp4 *= p_type5{"y"} * 3;
            BOOST_CHECK_EQUAL(tmp4.size(), 1u);
            auto it4 = tmp4.m_container.begin();
            BOOST_CHECK((std::is_same<decltype(it4->m_cf), g_series_type<int, Expo>>::value));
            BOOST_CHECK((it4->m_cf == 3 * g_series_type<int, Expo>{"y"}));
            BOOST_CHECK(it4->m_key.size() == 1u);
            BOOST_CHECK((tmp4.m_symbol_set == symbol_set{symbol{"x"}}));
            // Check with scalar on the left.
            BOOST_CHECK((!is_multipliable_in_place<int, p_type1>::value));
            BOOST_CHECK((!is_multipliable_in_place<int, p_type2>::value));
            BOOST_CHECK((!is_multipliable_in_place<int, p_type3>::value));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};
}

typedef debug_access<arithmetics_mul_tag> arithmetics_mul_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_mul_test)
{
    // Functional testing.
    tuple_for_each(cf_types{}, arithmetics_mul_tester());
    // Type testing for binary multiplication.
    typedef g_series_type<rational, int> p_type1;
    typedef g_series_type<int, rational> p_type2;
    typedef g_series_type<short, rational> p_type3;
    typedef g_series_type<char, rational> p_type4;
    // First let's check the output type.
    // Case 0.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} * p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} * p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type2{} * p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type3{} * p_type4{})>::value));
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type4{} * p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} * 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type2, decltype(p_type3{} * 0)>::value));
    // Case 6.
    BOOST_CHECK((std::is_same<p_type1, decltype(0 * p_type1{})>::value));
    // Case 7.
    BOOST_CHECK((std::is_same<p_type2, decltype(0 * p_type3{})>::value));
    // Check non-multipliable series.
    typedef g_series_type2<rational, int> p_type5;
    BOOST_CHECK((!is_multipliable<p_type1, p_type5>::value));
    BOOST_CHECK((!is_multipliable<p_type5, p_type1>::value));
    // Check coefficient series.
    typedef g_series_type<p_type1, int> p_type11;
    typedef g_series_type<p_type2, rational> p_type22;
    typedef g_series_type<p_type1, rational> p_type21;
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type1{} * p_type11{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type11{} * p_type1{})>::value));
    BOOST_CHECK((std::is_same<p_type21, decltype(p_type1{} * p_type22{})>::value));
    BOOST_CHECK((std::is_same<p_type21, decltype(p_type22{} * p_type1{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type11{} * p_type22{})>::value));
    BOOST_CHECK((std::is_same<p_type11, decltype(p_type22{} * p_type11{})>::value));
    // Type testing for in-place multiplication.
    // Case 0.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() *= p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() *= p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<p_type2 &, decltype(std::declval<p_type2 &>() *= p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<p_type3 &, decltype(std::declval<p_type3 &>() *= p_type4{})>::value));
    BOOST_CHECK((std::is_same<p_type4 &, decltype(std::declval<p_type4 &>() *= p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() *= 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type3 &, decltype(std::declval<p_type3 &>() *= 0)>::value));
    // Cases 6 and 7 do not make sense at the moment.
    BOOST_CHECK((!is_multipliable_in_place<int, p_type3>::value));
    BOOST_CHECK((!is_multipliable_in_place<p_type1, p_type11>::value));
    // Checks for coefficient series.
    p_type11 tmp;
    BOOST_CHECK((std::is_same<p_type11 &, decltype(tmp *= p_type1{})>::value));
    p_type22 tmp2;
    BOOST_CHECK((std::is_same<p_type22 &, decltype(tmp2 *= p_type1{})>::value));
}

struct arithmetics_div_tag {
};

namespace piranha
{
template <>
class debug_access<arithmetics_div_tag>
{
public:
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            p_type1 x{"x"};
            // Some tests for case 4.
            auto tmp = 3 * x / 2;
            BOOST_CHECK_EQUAL(tmp.size(), 1u);
            auto it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(3) / 2);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Case 5.
            auto tmp2 = 3 * x / 2.;
            auto it2 = tmp2.m_container.begin();
            BOOST_CHECK(it2->m_cf == Cf(3) / 2.);
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            // In-place.
            // Case 4.
            tmp = 3 * x;
            tmp /= 2;
            it = tmp.m_container.begin();
            BOOST_CHECK(it->m_cf == Cf(3) / 2);
            BOOST_CHECK(it->m_key.size() == 1u);
            BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
            // Case 5.
            tmp2 = 3 * x;
            tmp2 /= 2.;
            it2 = tmp2.m_container.begin();
            BOOST_CHECK(it2->m_cf == Cf(3) / 2.);
            BOOST_CHECK(it2->m_key.size() == 1u);
            BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
            // Test division by zero of empty series.
            if (std::is_same<integer, Cf>::value) {
                BOOST_CHECK_THROW(p_type1{} / 0, mppp::zero_division_error);
                p_type1 zero;
                BOOST_CHECK_THROW(zero /= 0, mppp::zero_division_error);
            }
            if (std::is_same<rational, Cf>::value) {
                BOOST_CHECK_THROW(p_type1{} / 0, zero_division_error);
                p_type1 zero;
                BOOST_CHECK_THROW(zero /= 0, zero_division_error);
            }
            // Check with scalar on the left.
            BOOST_CHECK((!is_divisible_in_place<int, p_type1>::value));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};
}

typedef debug_access<arithmetics_div_tag> arithmetics_div_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_div_test)
{
    // Functional testing.
    tuple_for_each(cf_types{}, arithmetics_div_tester());
    // Type testing for binary division.
    typedef g_series_type<rational, int> p_type1;
    typedef g_series_type<p_type1, int> p_type11;
    typedef g_series_type<double, int> p_type1d;
    typedef g_series_type<float, int> p_type1f;
    // First let's check the output type.
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} / 0)>::value));
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} / integer{})>::value));
    BOOST_CHECK((std::is_same<p_type1, decltype(p_type1{} / rational{})>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type1d, decltype(p_type1{} / 0.)>::value));
    BOOST_CHECK((std::is_same<p_type1f, decltype(p_type1{} / 0.f)>::value));
    // Some scalars on the first argument.
    BOOST_CHECK((is_divisible<double, p_type1>::value));
    BOOST_CHECK((std::is_same<decltype(3. / p_type1{}), g_series_type<double, int>>::value));
    BOOST_CHECK((is_divisible<int, p_type1>::value));
    BOOST_CHECK((std::is_same<decltype(3 / p_type1{}), p_type1>::value));
    BOOST_CHECK((is_divisible<integer, p_type1>::value));
    BOOST_CHECK((std::is_same<decltype(3_z / p_type1{}), p_type1>::value));
    // Type testing for in-place division.
    // Case 4.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() /= 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<p_type1 &, decltype(std::declval<p_type1 &>() /= 0.)>::value));
    // Not divisible in-place.
    BOOST_CHECK((!is_divisible_in_place<int, p_type1>::value));
    // Divisible in-place after recent changes.
    BOOST_CHECK((is_divisible_in_place<p_type11, p_type1>::value));
    // Special cases to test the erasing of terms.
    using pint = g_series_type<integer, int>;
    pint x{"x"}, y{"y"};
    auto tmp = 2 * x + y;
    tmp /= 2;
    BOOST_CHECK_EQUAL(tmp, x);
    tmp = 2 * x + 2 * y;
    tmp /= 3;
    BOOST_CHECK(tmp.empty());
    // Check zero division error.
    tmp = 2 * x + y;
    BOOST_CHECK_THROW(tmp /= 0, mppp::zero_division_error);
    BOOST_CHECK(tmp.empty());
}

struct eq_tag {
};

namespace piranha
{
template <>
class debug_access<eq_tag>
{
public:
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            typedef g_series_type2<Cf, Expo> p_type2;
            typedef g_series_type<int, Expo> p_type3;
            // Some type checks - these are not comparable as they result in an ambiguity
            // between two series with same coefficient but different series types.
            BOOST_CHECK((!is_equality_comparable<p_type1, p_type2>::value));
            BOOST_CHECK((!is_equality_comparable<p_type2, p_type1>::value));
            BOOST_CHECK((!is_equality_comparable<p_type1, p_type2>::value));
            BOOST_CHECK((!is_equality_comparable<p_type2, p_type1>::value));
            // Various subcases of case 0.
            p_type1 x{"x"}, y{"y"};
            BOOST_CHECK_EQUAL(x, x);
            BOOST_CHECK_EQUAL(y, y);
            BOOST_CHECK_EQUAL(x, x + y - y);
            BOOST_CHECK_EQUAL(y, y + x - x);
            // Arguments merging on both sides.
            BOOST_CHECK(x != y);
            // Check with series of different size.
            BOOST_CHECK(x != y + x);
            // Arguments merging on the other side.
            BOOST_CHECK(y + x != y);
            // Some tests for case 1/4.
            BOOST_CHECK(x != p_type3{"y"});
            BOOST_CHECK(y != p_type3{"x"});
            BOOST_CHECK(x != p_type3{"y"} + p_type3{"x"});
            BOOST_CHECK(y != p_type3{"x"} + p_type3{"y"});
            BOOST_CHECK_EQUAL(x, p_type3{"x"});
            BOOST_CHECK_EQUAL(x, p_type3{"x"} + p_type3{"y"} - p_type3{"y"});
            BOOST_CHECK(x != 0);
            BOOST_CHECK(y != 0);
            BOOST_CHECK_EQUAL(x - x, 0);
            BOOST_CHECK_EQUAL(p_type1{1}, 1);
            BOOST_CHECK_EQUAL(p_type1{-1}, -1);
            // Symmetric of above.
            BOOST_CHECK(p_type3{"y"} != x);
            BOOST_CHECK(p_type3{"x"} != y);
            BOOST_CHECK(p_type3{"y"} + p_type3{"x"} != x);
            BOOST_CHECK(p_type3{"x"} + p_type3{"y"} != y);
            BOOST_CHECK_EQUAL(p_type3{"x"}, x);
            BOOST_CHECK_EQUAL(p_type3{"x"} + p_type3{"y"} - p_type3{"y"}, x);
            BOOST_CHECK(0 != x);
            BOOST_CHECK(0 != y);
            BOOST_CHECK_EQUAL(0, x - x);
            BOOST_CHECK_EQUAL(1, p_type1{1});
            BOOST_CHECK_EQUAL(-1, p_type1{-1});
            // Case 3/5 and symmetric.
            using p_type4 = g_series_type<g_series_type<int, Expo>, Expo>;
            using p_type5 = g_series_type<double, Expo>;
            BOOST_CHECK_EQUAL((p_type4{g_series_type<int, Expo>{"x"}}), p_type5{"x"});
            BOOST_CHECK_EQUAL(p_type5{"x"}, (p_type4{g_series_type<int, Expo>{"x"}}));
            BOOST_CHECK((p_type4{g_series_type<int, Expo>{"x"}} != p_type5{"y"}));
            BOOST_CHECK((p_type5{"y"} != p_type4{g_series_type<int, Expo>{"x"}}));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};
}

typedef debug_access<eq_tag> eq_tester;

BOOST_AUTO_TEST_CASE(series_eq_test)
{
    // Functional testing.
    tuple_for_each(cf_types{}, eq_tester());
    // Type testing for binary addition.
    typedef g_series_type<rational, int> p_type1;
    typedef g_series_type<int, rational> p_type2;
    typedef g_series_type<short, rational> p_type3;
    typedef g_series_type<char, rational> p_type4;
    // First let's check the output type.
    // Case 0.
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} == p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} != p_type1{})>::value));
    // Case 1.
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} == p_type2{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} != p_type2{})>::value));
    // Case 2.
    BOOST_CHECK((std::is_same<bool, decltype(p_type2{} == p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type2{} != p_type1{})>::value));
    // Case 3, symmetric.
    BOOST_CHECK((std::is_same<bool, decltype(p_type3{} == p_type4{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type3{} != p_type4{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type4{} == p_type3{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type4{} != p_type3{})>::value));
    // Case 4.
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} == 0)>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} != 0)>::value));
    // Case 5.
    BOOST_CHECK((std::is_same<bool, decltype(p_type3{} == 0)>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type3{} != 0)>::value));
    // Case 6.
    BOOST_CHECK((std::is_same<bool, decltype(0 == p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(0 != p_type1{})>::value));
    // Case 7.
    BOOST_CHECK((std::is_same<bool, decltype(0 == p_type3{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(0 != p_type3{})>::value));
    // Check non-addable series.
    typedef g_series_type2<rational, int> p_type5;
    BOOST_CHECK((!is_equality_comparable<p_type1, p_type5>::value));
    BOOST_CHECK((!is_equality_comparable<p_type5, p_type1>::value));
    // Check coefficient series.
    typedef g_series_type<p_type1, int> p_type11;
    typedef g_series_type<p_type2, rational> p_type22;
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} == p_type11{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} != p_type11{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type11{} == p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type11{} != p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} == p_type22{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type1{} != p_type22{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type22{} == p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type22{} != p_type1{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type11{} == p_type22{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type11{} != p_type22{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type22{} == p_type11{})>::value));
    BOOST_CHECK((std::is_same<bool, decltype(p_type22{} != p_type11{})>::value));
}

BOOST_AUTO_TEST_CASE(series_hash_test)
{
    typedef g_series_type<rational, int> p_type1;
    typedef g_series_type<integer, int> p_type2;
    BOOST_CHECK_EQUAL(p_type1{}.hash(), 0u);
    BOOST_CHECK_EQUAL(p_type2{}.hash(), 0u);
    // Check that only the key is used to compute the hash.
    BOOST_CHECK_EQUAL(p_type1{"x"}.hash(), p_type2{"x"}.hash());
    auto x = p_type1{"x"}, y = p_type1{"y"}, x2 = (x + y) - y;
    // NOTE: this is not 100% sure as the hash mixing in the monomial could actually lead to identical hashes.
    // But the probability should be rather low.
    BOOST_CHECK(x.hash() != x2.hash());
    // This shows we cannot use standard equality operator in hash tables.
    BOOST_CHECK_EQUAL(x, x2);
    // A bit more testing.
    BOOST_CHECK_EQUAL((x + 2 * y).hash(), (x + y + y).hash());
    BOOST_CHECK_EQUAL((x + 2 * y - y).hash(), (x + y).hash());
}

BOOST_AUTO_TEST_CASE(series_is_identical_test)
{
    typedef g_series_type<rational, int> p_type1;
    BOOST_CHECK(p_type1{}.is_identical(p_type1{}));
    auto x = p_type1{"x"}, y = p_type1{"y"}, x2 = (x + y) - y;
    BOOST_CHECK(x.is_identical(x));
    BOOST_CHECK(x.is_identical(p_type1{"x"}));
    BOOST_CHECK(!x.is_identical(y));
    BOOST_CHECK(!y.is_identical(x));
    BOOST_CHECK_EQUAL(x2, x);
    BOOST_CHECK(!x2.is_identical(x));
    BOOST_CHECK(!x.is_identical(x2));
    BOOST_CHECK(x.is_identical(x2.trim()));
    BOOST_CHECK(x2.trim().is_identical(x));
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

BOOST_AUTO_TEST_CASE(series_has_series_multiplier_test)
{
    typedef g_series_type<rational, int> p_type1;
    BOOST_CHECK(series_has_multiplier<p_type1>::value);
    BOOST_CHECK(series_has_multiplier<p_type1 &>::value);
    BOOST_CHECK(series_has_multiplier<const p_type1 &>::value);
    typedef g_series_type<mock_cf3, int> p_type2;
    BOOST_CHECK(!series_has_multiplier<p_type2>::value);
    BOOST_CHECK(!series_has_multiplier<p_type2 const>::value);
    BOOST_CHECK(!series_has_multiplier<p_type2 const &>::value);
    typedef g_series_type3<double, mock_key> p_type3;
    BOOST_CHECK(!series_has_multiplier<p_type3>::value);
    BOOST_CHECK(!series_has_multiplier<p_type3 &>::value);
    BOOST_CHECK(!series_has_multiplier<p_type3 &&>::value);
}

// A non-multipliable series, missing a suitable series_multiplier specialisation.
template <typename Cf, typename Expo>
class g_series_type_nm : public series<Cf, monomial<Expo>, g_series_type_nm<Cf, Expo>>
{
    typedef series<Cf, monomial<Expo>, g_series_type_nm<Cf, Expo>> base;

public:
    template <typename Cf2>
    using rebind = g_series_type_nm<Cf2, Expo>;
    g_series_type_nm() = default;
    g_series_type_nm(const g_series_type_nm &) = default;
    g_series_type_nm(g_series_type_nm &&) = default;
    explicit g_series_type_nm(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set.add(name);
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{Expo(1)}));
    }
    g_series_type_nm &operator=(const g_series_type_nm &) = default;
    g_series_type_nm &operator=(g_series_type_nm &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type_nm, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nm, base)
};

namespace piranha
{

template <typename Cf, typename Expo>
class series_multiplier<g_series_type_nm<Cf, Expo>, void>
{
};
}

BOOST_AUTO_TEST_CASE(series_no_series_multiplier_test)
{
    typedef g_series_type_nm<rational, int> p_type1;
    BOOST_CHECK(!is_multipliable<p_type1>::value);
}

// Mock coefficient, with weird semantics for operator+(integer): the output is not a coefficient type.
struct mock_cf2 {
    mock_cf2();
    explicit mock_cf2(const int &);
    mock_cf2(const mock_cf2 &);
    mock_cf2(mock_cf2 &&) noexcept;
    mock_cf2 &operator=(const mock_cf2 &);
    mock_cf2 &operator=(mock_cf2 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const mock_cf2 &);
    mock_cf2 operator-() const;
    bool operator==(const mock_cf2 &) const;
    bool operator!=(const mock_cf2 &) const;
    mock_cf2 &operator+=(const mock_cf2 &);
    mock_cf2 &operator-=(const mock_cf2 &);
    mock_cf2 operator+(const mock_cf2 &) const;
    mock_cf2 operator-(const mock_cf2 &) const;
    mock_cf2 &operator*=(const mock_cf2 &);
    mock_cf2 operator*(const mock_cf2 &)const;
    std::string operator+(const integer &) const;
    std::vector<std::string> operator*(const integer &)const;
    std::vector<std::string> operator-(const integer &) const;
};

// Check that attempting to rebind to an invalid coefficient disables the operator, rather
// than resulting in a static assertion firing (as it was the case in the past).
BOOST_AUTO_TEST_CASE(series_rebind_failure_test)
{
    BOOST_CHECK(is_cf<mock_cf2>::value);
    BOOST_CHECK((!is_addable<g_series_type<integer, int>, g_series_type<mock_cf2, int>>::value));
    BOOST_CHECK((!is_addable<g_series_type<mock_cf2, int>, g_series_type<integer, int>>::value));
    BOOST_CHECK((is_addable<g_series_type<mock_cf2, int>, g_series_type<mock_cf2, int>>::value));
    BOOST_CHECK((!is_subtractable<g_series_type<integer, int>, g_series_type<mock_cf2, int>>::value));
    BOOST_CHECK((!is_subtractable<g_series_type<mock_cf2, int>, g_series_type<integer, int>>::value));
    BOOST_CHECK((is_subtractable<g_series_type<mock_cf2, int>, g_series_type<mock_cf2, int>>::value));
    BOOST_CHECK((!is_multipliable<g_series_type<integer, int>, g_series_type<mock_cf2, int>>::value));
    BOOST_CHECK((!is_multipliable<g_series_type<mock_cf2, int>, g_series_type<integer, int>>::value));
    BOOST_CHECK((is_multipliable<g_series_type<mock_cf2, int>, g_series_type<mock_cf2, int>>::value));
}
