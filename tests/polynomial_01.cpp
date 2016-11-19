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

#define BOOST_TEST_MODULE polynomial_01_test
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/base_series_multiplier.hpp"
#include "../src/debug_access.hpp"
#include "../src/forwarding.hpp"
#include "../src/init.hpp"
#include "../src/key_is_multipliable.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/real.hpp"
#include "../src/series.hpp"
#include "../src/series_multiplier.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<double, rational> cf_types;
typedef boost::mpl::vector<int, integer> expo_types;

template <typename Cf, typename Expo>
class polynomial_alt : public series<Cf, monomial<Expo>, polynomial_alt<Cf, Expo>>
{
    typedef series<Cf, monomial<Expo>, polynomial_alt<Cf, Expo>> base;

public:
    polynomial_alt() = default;
    polynomial_alt(const polynomial_alt &) = default;
    polynomial_alt(polynomial_alt &&) = default;
    explicit polynomial_alt(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set.add(symbol(name));
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{Expo(1)}));
    }
    PIRANHA_FORWARDING_CTOR(polynomial_alt, base)
    ~polynomial_alt() = default;
    polynomial_alt &operator=(const polynomial_alt &) = default;
    polynomial_alt &operator=(polynomial_alt &&) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(polynomial_alt, base)
};

namespace piranha
{

template <typename Cf, typename Expo>
class series_multiplier<polynomial_alt<Cf, Expo>, void> : public base_series_multiplier<polynomial_alt<Cf, Expo>>
{
    using base = base_series_multiplier<polynomial_alt<Cf, Expo>>;
    template <typename T>
    using call_enabler = typename std::enable_if<key_is_multipliable<typename T::term_type::cf_type,
                                                                     typename T::term_type::key_type>::value,
                                                 int>::type;

public:
    using base::base;
    template <typename T = polynomial_alt<Cf, Expo>, call_enabler<T> = 0>
    polynomial_alt<Cf, Expo> operator()() const
    {
        return this->plain_multiplication();
    }
};
}

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

struct constructor_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &)
        {
            typedef polynomial<Cf, monomial<Expo>> p_type;
            // Default construction.
            p_type p1;
            BOOST_CHECK(p1 == 0);
            BOOST_CHECK(p1.empty());
            // Construction from symbol name.
            p_type p2{"x"};
            BOOST_CHECK(p2.size() == 1u);
            BOOST_CHECK(p2 == p_type{"x"});
            BOOST_CHECK(p2 != p_type{std::string("y")});
            BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
            // Construction from number-like entities.
            p_type p3{3};
            BOOST_CHECK(p3.size() == 1u);
            BOOST_CHECK(p3 == 3);
            BOOST_CHECK(3 == p3);
            BOOST_CHECK(p3 != p2);
            p_type p3a{integer(3)};
            BOOST_CHECK(p3a == p3);
            BOOST_CHECK(p3 == p3a);
            // Construction from polynomial of different type.
            typedef polynomial<long, monomial<int>> p_type1;
            typedef polynomial<int, monomial<short>> p_type2;
            p_type1 p4(1);
            p_type2 p5(p4);
            BOOST_CHECK(p4 == p5);
            BOOST_CHECK(p5 == p4);
            p_type1 p6("x");
            p_type2 p7(std::string("x"));
            p_type2 p8("y");
            BOOST_CHECK(p6 == p7);
            BOOST_CHECK(p7 == p6);
            BOOST_CHECK(p6 != p8);
            BOOST_CHECK(p8 != p6);
            // Type traits checks.
            BOOST_CHECK((std::is_constructible<p_type, Cf>::value));
            BOOST_CHECK((std::is_constructible<p_type, std::string>::value));
            BOOST_CHECK((std::is_constructible<p_type2, p_type1>::value));
            BOOST_CHECK((!std::is_constructible<p_type, symbol>::value));
            // A check on the linarg detector.
            BOOST_CHECK(detail::key_has_linarg<monomial<Expo>>::value);
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<expo_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_constructors_test)
{
    init();
    boost::mpl::for_each<cf_types>(constructor_tester());
}

struct is_evaluable_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &)
        {
            typedef polynomial<Cf, monomial<Expo>> p_type;
            BOOST_CHECK((is_evaluable<p_type, double>::value));
            BOOST_CHECK((is_evaluable<p_type, float>::value));
            BOOST_CHECK((is_evaluable<p_type, integer>::value));
            BOOST_CHECK((is_evaluable<p_type, int>::value));
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<expo_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_is_evaluable_test)
{
    boost::mpl::for_each<cf_types>(is_evaluable_tester());
    BOOST_CHECK((is_evaluable<polynomial<mock_cf, monomial<int>>, double>::value));
}

struct assignment_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &)
        {
            typedef polynomial<Cf, monomial<Expo>> p_type;
            p_type p1;
            p1 = 1;
            BOOST_CHECK(p1 == 1);
            p1 = integer(10);
            BOOST_CHECK(p1 == integer(10));
            BOOST_CHECK((std::is_assignable<p_type, Cf>::value));
            BOOST_CHECK((std::is_assignable<p_type, p_type>::value));
            BOOST_CHECK((!std::is_assignable<p_type, symbol>::value));
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<expo_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_assignment_test)
{
    boost::mpl::for_each<cf_types>(assignment_tester());
}

BOOST_AUTO_TEST_CASE(polynomial_recursive_test)
{
    typedef polynomial<double, monomial<int>> p_type1;
    typedef polynomial<p_type1, monomial<int>> p_type11;
    typedef polynomial<p_type11, monomial<int>> p_type111;
    p_type1 x("x");
    p_type11 y("y");
    p_type111 z("z");
    BOOST_CHECK((std::is_same<decltype(x + y), p_type11>::value));
    BOOST_CHECK((std::is_same<decltype(y + x), p_type11>::value));
    BOOST_CHECK((std::is_same<decltype(z + y), p_type111>::value));
    BOOST_CHECK((std::is_same<decltype(y + z), p_type111>::value));
    BOOST_CHECK((std::is_same<decltype(z + x), p_type111>::value));
    BOOST_CHECK((std::is_same<decltype(x + z), p_type111>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_degree_test)
{
    typedef polynomial<double, monomial<int>> p_type1;
    typedef polynomial<p_type1, monomial<int>> p_type11;
    typedef polynomial<p_type11, monomial<int>> p_type111;
    BOOST_CHECK(has_degree<p_type1>::value);
    BOOST_CHECK(has_ldegree<p_type1>::value);
    BOOST_CHECK(has_degree<p_type11>::value);
    BOOST_CHECK(has_ldegree<p_type11>::value);
    BOOST_CHECK(has_degree<p_type111>::value);
    BOOST_CHECK(has_ldegree<p_type111>::value);
    p_type1 x("x");
    BOOST_CHECK(math::degree(x) == 1);
    BOOST_CHECK(math::ldegree(x) == 1);
    BOOST_CHECK(math::degree(x * x) == 2);
    BOOST_CHECK(math::ldegree(x * x) == 2);
    BOOST_CHECK(math::degree(x * x, {"y", "z"}) == 0);
    BOOST_CHECK(math::ldegree(x * x, {"y", "z"}) == 0);
    p_type11 y("y");
    p_type111 z("z");
    BOOST_CHECK(math::degree(x * y) == 2);
    BOOST_CHECK(math::degree(x * y * z) == 3);
    BOOST_CHECK(math::ldegree(x * y * z) == 3);
    BOOST_CHECK(math::degree(x * y * z, {"x"}) == 1);
    BOOST_CHECK(math::ldegree(x * y * z, {"x"}) == 1);
    BOOST_CHECK(math::degree(x * y * z, {"y"}) == 1);
    BOOST_CHECK(math::ldegree(x * y * z, {"y"}) == 1);
    BOOST_CHECK(math::degree(x * y * z, {"z"}) == 1);
    BOOST_CHECK(math::ldegree(x * y * z, {"z"}) == 1);
    BOOST_CHECK(math::degree(x * y * z, {"z", "y"}) == 2);
    BOOST_CHECK(math::ldegree(x * y * z, {"z", "y"}) == 2);
    BOOST_CHECK(math::degree(x * y * z, {"z", "x"}) == 2);
    BOOST_CHECK(math::ldegree(x * y * z, {"z", "x"}) == 2);
    BOOST_CHECK(math::degree(x * y * z, {"y", "x"}) == 2);
    BOOST_CHECK(math::ldegree(x * y * z, {"y", "x"}) == 2);
    BOOST_CHECK(math::degree(x * y * z, {"y", "x", "z"}) == 3);
    BOOST_CHECK(math::ldegree(x * y * z, {"y", "x", "z"}) == 3);
    BOOST_CHECK(math::degree(x + y + z) == 1);
    BOOST_CHECK(math::ldegree(x + y + z) == 1);
    BOOST_CHECK(math::degree(x + y + z, {"x"}) == 1);
    BOOST_CHECK(math::ldegree(x + y + z, {"x"}) == 0);
    BOOST_CHECK(math::ldegree(x + y + z, {"x", "y"}) == 0);
    BOOST_CHECK(math::ldegree(x + y + 1, {"x", "y"}) == 0);
    BOOST_CHECK(math::ldegree(x + y + 1, {"x", "y", "t"}) == 0);
    BOOST_CHECK(math::ldegree(x + y + 1) == 0);
}

struct multiplication_tester {
    template <typename Cf, typename std::enable_if<!detail::is_mp_rational<Cf>::value, int>::type = 0>
    void operator()(const Cf &)
    {
        // NOTE: this test is going to be exact in case of coefficients cancellations with double
        // precision coefficients only if the platform has ieee 754 format (integer exactly representable
        // as doubles up to 2 ** 53).
        if (std::is_same<Cf, double>::value
            && (!std::numeric_limits<double>::is_iec559 || std::numeric_limits<double>::radix != 2
                || std::numeric_limits<double>::digits < 53)) {
            return;
        }
        typedef polynomial<Cf, monomial<int>> p_type;
        typedef polynomial_alt<Cf, int> p_type_alt;
        p_type x("x"), y("y"), z("z"), t("t"), u("u");
        // Dense case, default setup.
        auto f = 1 + x + y + z + t;
        auto tmp(f);
        for (int i = 1; i < 10; ++i) {
            f *= tmp;
        }
        auto g = f + 1;
        auto retval = f * g;
        BOOST_CHECK_EQUAL(retval.size(), 10626u);
        auto retval_alt = p_type_alt(f) * p_type_alt(g);
        BOOST_CHECK(retval == p_type{retval_alt});
        // Dense case, force number of threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * g;
            auto tmp_alt = p_type_alt(f) * p_type_alt(g);
            BOOST_CHECK_EQUAL(tmp1.size(), 10626u);
            BOOST_CHECK(tmp1 == retval);
            BOOST_CHECK(tmp1 == p_type{tmp_alt});
        }
        settings::reset_n_threads();
        // Dense case with cancellations, default setup.
        auto h = 1 - x + y + z + t;
        tmp = h;
        for (int i = 1; i < 10; ++i) {
            h *= tmp;
        }
        retval = f * h;
        retval_alt = p_type_alt(f) * p_type_alt(h);
        BOOST_CHECK_EQUAL(retval.size(), 5786u);
        BOOST_CHECK(retval == p_type{retval_alt});
        // Dense case with cancellations, force number of threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * h;
            auto tmp_alt = p_type_alt(f) * p_type_alt(h);
            BOOST_CHECK_EQUAL(tmp1.size(), 5786u);
            BOOST_CHECK(retval == tmp1);
            BOOST_CHECK(tmp_alt == p_type_alt{tmp1});
        }
        settings::reset_n_threads();
        // Sparse case, default.
        f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
        auto tmp_f(f);
        g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
        auto tmp_g(g);
        h = (-u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
        auto tmp_h(h);
        for (int i = 1; i < 8; ++i) {
            f *= tmp_f;
            g *= tmp_g;
            h *= tmp_h;
        }
        retval = f * g;
        BOOST_CHECK_EQUAL(retval.size(), 591235u);
        retval_alt = p_type_alt(f) * p_type_alt(g);
        BOOST_CHECK(retval == p_type{retval_alt});
        // Sparse case, force n threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * g;
            auto tmp_alt = p_type_alt(f) * p_type_alt(g);
            BOOST_CHECK_EQUAL(tmp1.size(), 591235u);
            BOOST_CHECK(retval == tmp1);
            BOOST_CHECK(tmp_alt == p_type_alt{tmp1});
        }
        settings::reset_n_threads();
        // Sparse case with cancellations, default.
        retval = f * h;
        BOOST_CHECK_EQUAL(retval.size(), 591184u);
        retval_alt = p_type_alt(f) * p_type_alt(h);
        BOOST_CHECK(retval_alt == p_type_alt{retval});
        // Sparse case with cancellations, force number of threads.
        for (auto i = 1u; i <= 4u; ++i) {
            settings::set_n_threads(i);
            auto tmp1 = f * h;
            auto tmp_alt = p_type_alt(f) * p_type_alt(h);
            BOOST_CHECK_EQUAL(tmp1.size(), 591184u);
            BOOST_CHECK(tmp1 == retval);
            BOOST_CHECK(tmp1 == p_type{tmp_alt});
        }
    }
    template <typename Cf, typename std::enable_if<detail::is_mp_rational<Cf>::value, int>::type = 0>
    void operator()(const Cf &)
    {
    }
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_test)
{
    boost::mpl::for_each<cf_types>(multiplication_tester());
}

struct integral_combination_tag {
};

namespace piranha
{
template <>
class debug_access<integral_combination_tag>
{
public:
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &)
        {
            // Skip tests for fp values.
            if (std::is_floating_point<Cf>::value) {
                return;
            }
            typedef polynomial<Cf, monomial<Expo>> p_type;
            typedef std::map<std::string, integer> map_type;
            p_type p1;
            BOOST_CHECK((p1.integral_combination() == map_type{}));
            p1 = p_type{"x"};
            BOOST_CHECK((p1.integral_combination() == map_type{{"x", integer(1)}}));
            p1 += 2 * p_type{"y"};
            BOOST_CHECK((p1.integral_combination() == map_type{{"y", integer(2)}, {"x", integer(1)}}));
            p1 = p_type{"x"} + 1;
            BOOST_CHECK_THROW(p1.integral_combination(), std::invalid_argument);
            p1 = p_type{"x"}.pow(2);
            BOOST_CHECK_THROW(p1.integral_combination(), std::invalid_argument);
            p1 = p_type{"x"} * 2 - p_type{"z"} * 3;
            BOOST_CHECK((p1.integral_combination() == map_type{{"x", integer(2)}, {"z", integer(-3)}}));
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<expo_types>(runner<Cf>());
        // Tests specific to rational, double and real.
        typedef polynomial<rational, monomial<int>> p_type;
        typedef std::map<std::string, integer> map_type;
        p_type p1;
        p1 = p_type{"x"} * rational(4, 2) + p_type{"y"} * 4;
        BOOST_CHECK((p1.integral_combination() == map_type{{"x", integer(2)}, {"y", integer(4)}}));
        p1 = p_type{"x"} * rational(4, 3) + p_type{"y"} * 4;
        BOOST_CHECK_THROW(p1.integral_combination(), std::invalid_argument);
        p1 = 3 * (p_type{"x"} * rational(5, 3) - p_type{"y"} * 4);
        BOOST_CHECK((p1.integral_combination() == map_type{{"x", integer(5)}, {"y", integer(-12)}}));
        if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2
            && std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN) {
            typedef polynomial<double, monomial<int>> p_type2;
            p_type2 p2;
            p2 = p_type2{"x"} * 2. + p_type2{"y"} * 4.;
            BOOST_CHECK((p2.integral_combination() == map_type{{"x", integer(2)}, {"y", integer(4)}}));
            p2 = p_type2{"x"} * 2.5 + p_type2{"y"} * 4.;
            BOOST_CHECK_THROW(p2.integral_combination(), std::invalid_argument);
        }
        typedef polynomial<real, monomial<int>> p_type3;
        p_type3 p3;
        p3 = p_type3{"x"} * 2 + p_type3{"y"} * 4;
        BOOST_CHECK((p3.integral_combination() == map_type{{"x", integer(2)}, {"y", integer(4)}}));
        p3 = p_type3{"x"} * real{"2.5"} + p_type3{"y"} * 4.;
        BOOST_CHECK_THROW(p3.integral_combination(), std::invalid_argument);
    }
};
}

typedef debug_access<integral_combination_tag> ic_tester;

BOOST_AUTO_TEST_CASE(polynomial_integral_combination_test)
{
    boost::mpl::for_each<cf_types>(ic_tester());
}

struct pow_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &)
        {
            typedef polynomial<Cf, monomial<Expo>> p_type;
            p_type p{"x"};
            BOOST_CHECK_EQUAL((2 * p).pow(4), p_type{math::pow(Cf(1) * 2, 4)} * p * p * p * p);
            p *= p_type{"y"}.pow(2);
            BOOST_CHECK_EQUAL((3 * p).pow(4), p_type{math::pow(Cf(1) * 3, 4)} * p * p * p * p);
            if (!std::is_unsigned<Expo>::value) {
                BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p.pow(-1)), "x**-1*y**-2");
            }
            BOOST_CHECK_EQUAL(p.pow(0), p_type{math::pow(Cf(1), 0)});
            BOOST_CHECK_EQUAL(p_type{3}.pow(4), math::pow(Cf(3), 4));
            BOOST_CHECK_THROW((p + p_type{"x"}).pow(-1), std::invalid_argument);
            BOOST_CHECK_EQUAL((p + p_type{"x"}).pow(0), Cf(1));
        }
    };
    template <typename Cf>
    void operator()(const Cf &)
    {
        boost::mpl::for_each<expo_types>(runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(polynomial_pow_test)
{
    boost::mpl::for_each<cf_types>(pow_tester());
    typedef polynomial<integer, monomial<int>> p_type1;
    BOOST_CHECK((is_exponentiable<p_type1, integer>::value));
    BOOST_CHECK((is_exponentiable<const p_type1, integer>::value));
    BOOST_CHECK((is_exponentiable<p_type1 &, integer>::value));
    BOOST_CHECK((is_exponentiable<p_type1 &, integer &>::value));
    BOOST_CHECK((!is_exponentiable<p_type1, std::string>::value));
    BOOST_CHECK((!is_exponentiable<p_type1 &, std::string &>::value));
    BOOST_CHECK((is_exponentiable<p_type1, double>::value));
    BOOST_CHECK((std::is_same<decltype(p_type1{"x"}.pow(2.)), polynomial<double, monomial<int>>>::value));
    BOOST_CHECK_EQUAL((p_type1{"x"}.pow(2.)),
                      (polynomial<double, monomial<int>>{"x"} * polynomial<double, monomial<int>>{"x"}));
    typedef polynomial<real, monomial<int>> p_type2;
    BOOST_CHECK((is_exponentiable<p_type2, integer>::value));
    BOOST_CHECK((is_exponentiable<p_type2, real>::value));
    BOOST_CHECK((!is_exponentiable<p_type2, std::string>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_partial_test)
{
    using math::partial;
    using math::pow;
    typedef polynomial<rational, monomial<short>> p_type1;
    p_type1 x{"x"}, y{"y"};
    BOOST_CHECK_EQUAL(partial(x * y, "x"), y);
    BOOST_CHECK_EQUAL(partial(x * y, "y"), x);
    BOOST_CHECK_EQUAL(partial((x * y + x - 3 * pow(y, 2)).pow(10), "y"),
                      10 * (x * y + x - 3 * pow(y, 2)).pow(9) * (x - 6 * y));
    BOOST_CHECK_EQUAL(partial((x * y + x - 3 * pow(y, 2)).pow(10), "z"), 0);
    BOOST_CHECK(is_differentiable<p_type1>::value);
    BOOST_CHECK(has_pbracket<p_type1>::value);
    BOOST_CHECK(has_transformation_is_canonical<p_type1>::value);
    BOOST_CHECK((!is_differentiable<polynomial<mock_cf, monomial<short>>>::value));
    BOOST_CHECK((!has_pbracket<polynomial<mock_cf, monomial<short>>>::value));
    BOOST_CHECK((!has_transformation_is_canonical<polynomial<mock_cf, monomial<short>>>::value));
}
