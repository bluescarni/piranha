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

#include <piranha/poisson_series.hpp>

#define BOOST_TEST_MODULE poisson_series_01_test
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include <mp++/config.hpp>
#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/integer.hpp>
#include <piranha/invert.hpp>
#include <piranha/math.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/series.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

struct foo {
};

using namespace piranha;

using cf_types = std::tuple<double, rational, polynomial<rational, monomial<int>>>;

struct constructor_tester {
    template <typename Cf, enable_if_t<std::is_base_of<detail::polynomial_tag, Cf>::value, int> = 0>
    void poly_ctor_test() const
    {
        typedef poisson_series<Cf> p_type;
        // Construction from symbol name.
        p_type p2{"x"};
        BOOST_CHECK(p2.size() == 1u);
        BOOST_CHECK(p2 == p_type{"x"});
        BOOST_CHECK(p2 != p_type{std::string("y")});
        BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
        BOOST_CHECK((std::is_constructible<p_type, std::string>::value));
        BOOST_CHECK((std::is_constructible<p_type, const char *>::value));
        BOOST_CHECK((!std::is_constructible<p_type, foo>::value));
        BOOST_CHECK((std::is_assignable<p_type, std::string>::value));
        BOOST_CHECK((!std::is_assignable<p_type, foo>::value));
    }
    template <typename Cf, enable_if_t<!std::is_base_of<detail::polynomial_tag, Cf>::value, int> = 0>
    void poly_ctor_test() const
    {
        typedef poisson_series<Cf> p_type;
        if (!std::is_constructible<Cf, std::string>::value) {
            BOOST_CHECK((!std::is_constructible<p_type, std::string>::value));
            BOOST_CHECK((!std::is_constructible<p_type, const char *>::value));
        }
        BOOST_CHECK((!std::is_constructible<p_type, foo>::value));
        BOOST_CHECK((!std::is_assignable<p_type, foo>::value));
        BOOST_CHECK((std::is_assignable<p_type, int>::value));
    }
    template <typename Cf>
    void operator()(const Cf &) const
    {
        typedef poisson_series<Cf> p_type;
        BOOST_CHECK(is_series<p_type>::value);
        // Default construction.
        p_type p1;
        BOOST_CHECK(p1 == 0);
        BOOST_CHECK(p1.empty());
        poly_ctor_test<Cf>();
        // Construction from number-like entities.
        p_type p3{3};
        BOOST_CHECK(p3.size() == 1u);
        BOOST_CHECK(p3 == 3);
        BOOST_CHECK(3 == p3);
        p_type p3a{integer(3)};
        BOOST_CHECK(p3a == p3);
        BOOST_CHECK(p3 == p3a);
        // Construction from poisson series of different type.
        typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
        typedef poisson_series<polynomial<integer, monomial<short>>> p_type2;
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
    }
};

BOOST_AUTO_TEST_CASE(poisson_series_constructors_test)
{
#if defined(MPPP_WITH_MPFR)
    mppp::real_set_default_prec(100);
#endif
    tuple_for_each(cf_types{}, constructor_tester());
}

struct assignment_tester {
    template <typename Cf, enable_if_t<std::is_base_of<detail::polynomial_tag, Cf>::value, int> = 0>
    void poly_assignment_test() const
    {
        typedef poisson_series<Cf> p_type;
        p_type p1;
        p1 = "x";
        BOOST_CHECK(p1 == p_type("x"));
    }
    template <typename Cf, enable_if_t<!std::is_base_of<detail::polynomial_tag, Cf>::value, int> = 0>
    void poly_assignment_test() const
    {
    }
    template <typename Cf>
    void operator()(const Cf &) const
    {
        typedef poisson_series<Cf> p_type;
        p_type p1;
        p1 = 1;
        BOOST_CHECK(p1 == 1);
        p1 = integer(10);
        BOOST_CHECK(p1 == integer(10));
        poly_assignment_test<Cf>();
    }
};

BOOST_AUTO_TEST_CASE(poisson_series_assignment_test)
{
    tuple_for_each(cf_types{}, assignment_tester());
}

BOOST_AUTO_TEST_CASE(poisson_series_stream_test)
{
    typedef poisson_series<integer> p_type1;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type1{}), "0");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type1{1}), "1");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type1{1} - 3), "-2");
    typedef poisson_series<rational> p_type2;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type2{}), "0");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type2{rational(1, 2)}), "1/2");
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type2{real("-0.5", 32)}), "-1/2");
#endif
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type3;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type3{}), "0");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type3{"x"}), "x");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(3, -2) * p_type3{"x"}), "-3/2*x");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(3, -2) * p_type3{"x"}.pow(2)), "-3/2*x**2");
}

BOOST_AUTO_TEST_CASE(poisson_series_sin_cos_test)
{
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    p_type1 p1{"x"};
    BOOST_CHECK((std::is_same<p_type1, decltype(math::sin(p_type1{}))>::value));
    BOOST_CHECK((std::is_same<p_type1, decltype(math::cos(p_type1{}))>::value));
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p1)), "-sin(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p1)), "cos(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.sin()), "sin(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>((-p1).cos()), "cos(x)");
    p1 = 0;
    BOOST_CHECK_EQUAL(math::sin(-p1), 0);
    BOOST_CHECK_EQUAL(math::cos(p1), 1);
    p1 = p_type1{"x"} - 2 * p_type1{"y"};
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p1)), "-sin(x-2*y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p1)), "cos(x-2*y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(3 * p1.sin()), "3*sin(x-2*y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.cos()), "cos(x-2*y)");
    p1 = p_type1{"x"} * p_type1{"y"};
    BOOST_CHECK_THROW(math::sin(p1), std::invalid_argument);
    BOOST_CHECK_THROW(math::cos(p1), std::invalid_argument);
    BOOST_CHECK_THROW(math::sin(p_type1{"x"} + 1), std::invalid_argument);
    BOOST_CHECK_THROW(math::cos(p_type1{"x"} - 1), std::invalid_argument);
    BOOST_CHECK_THROW(math::sin(p_type1{"x"} * rational(1, 2)), std::invalid_argument);
    BOOST_CHECK_THROW(math::cos(p_type1{"x"} * rational(1, 2)), std::invalid_argument);
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type1{"x"} * rational(4, -2))), "-sin(2*x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(-math::cos(p_type1{"x"} * rational(4, 2))), "-cos(2*x)");
#if defined(MPPP_WITH_MPFR)
    typedef poisson_series<polynomial<real, monomial<short>>> p_type2;
    BOOST_CHECK((std::is_same<p_type2, decltype(math::sin(p_type2{}))>::value));
    BOOST_CHECK((std::is_same<p_type2, decltype(math::cos(p_type2{}))>::value));
    BOOST_CHECK_EQUAL(math::sin(p_type2{3}), math::sin(real(3)));
    BOOST_CHECK_EQUAL(math::cos(p_type2{3}), math::cos(real(3)));
    p_type2 p2 = p_type2{"x"} - 2 * p_type2{"y"};
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p2)),
                      "-1.0000000000000000000000000000000*sin(x-2*y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p2)), "1.0000000000000000000000000000000*cos(x-2*y)");
    BOOST_CHECK_THROW(math::sin(p_type2{"x"} * real(rational(1, 2))), std::invalid_argument);
    BOOST_CHECK_THROW(math::cos(p_type2{"x"} * real(rational(1, 2))), std::invalid_argument);
    typedef poisson_series<real> p_type3;
    BOOST_CHECK_EQUAL(math::sin(p_type3{3}), math::sin(real(3)));
    BOOST_CHECK_EQUAL(math::cos(p_type3{3}), math::cos(real(3)));
#endif
    typedef poisson_series<double> p_type4;
    BOOST_CHECK((std::is_same<p_type4, decltype(math::sin(p_type4{}))>::value));
    BOOST_CHECK((std::is_same<p_type4, decltype(math::cos(p_type4{}))>::value));
    BOOST_CHECK_EQUAL(math::sin(p_type4{0}), 0);
    BOOST_CHECK_EQUAL(math::cos(p_type4{0}), std::cos(0));
    BOOST_CHECK_EQUAL(math::cos(p_type4{1}), std::cos(1));
    BOOST_CHECK_EQUAL(math::sin(p_type4{1}), std::sin(1));
    // Type traits checks.
    BOOST_CHECK(is_sine_type<p_type4>::value);
    BOOST_CHECK(has_cosine<p_type4>::value);
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK(is_sine_type<p_type3>::value);
    BOOST_CHECK(has_cosine<p_type3>::value);
#endif
    BOOST_CHECK(is_sine_type<p_type1>::value);
    BOOST_CHECK(has_cosine<p_type1>::value);
    BOOST_CHECK(is_sine_type<poisson_series<rational>>::value);
    BOOST_CHECK(has_cosine<poisson_series<rational>>::value);
    // Check with eps.
    using p_type5 = poisson_series<divisor_series<polynomial<rational, monomial<short>>, divisor<short>>>;
    BOOST_CHECK((std::is_same<p_type5, decltype(math::sin(p_type5{}))>::value));
    BOOST_CHECK((std::is_same<p_type5, decltype(math::cos(p_type5{}))>::value));
    BOOST_CHECK(is_sine_type<p_type5>::value);
    BOOST_CHECK(has_cosine<p_type5>::value);
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p_type5{"x"})), "cos(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p_type5{"x"} + p_type5{"y"})), "cos(x+y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p_type5{"x"} + p_type5{"y"})), "cos(x-y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type5{"x"})), "sin(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type5{"x"} + p_type5{"y"})), "sin(x+y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p_type5{"x"} + p_type5{"y"})), "-sin(x-y)");
    BOOST_CHECK_EQUAL(math::cos(p_type5{0}), 1);
    BOOST_CHECK_EQUAL(math::sin(p_type5{0}), 0);
    using p_type6 = poisson_series<divisor_series<polynomial<double, monomial<short>>, divisor<short>>>;
    BOOST_CHECK_EQUAL(math::cos(p_type6{1.23}), std::cos(1.23));
    BOOST_CHECK_EQUAL(math::sin(p_type6{-4.56}), std::sin(-4.56));
    // Double divisor.
    using p_type7 = poisson_series<
        divisor_series<divisor_series<polynomial<rational, monomial<short>>, divisor<short>>, divisor<short>>>;
    BOOST_CHECK(is_sine_type<p_type7>::value);
    BOOST_CHECK(has_cosine<p_type7>::value);
    BOOST_CHECK((std::is_same<p_type7, decltype(math::sin(p_type7{}))>::value));
    BOOST_CHECK((std::is_same<p_type7, decltype(math::cos(p_type7{}))>::value));
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p_type7{"x"})), "cos(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p_type7{"x"} + p_type7{"y"})), "cos(x+y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p_type7{"x"} + p_type7{"y"})), "cos(x-y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type7{"x"})), "sin(x)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type7{"x"} + p_type7{"y"})), "sin(x+y)");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p_type7{"x"} + p_type7{"y"})), "-sin(x-y)");
    BOOST_CHECK_EQUAL(math::cos(p_type7{0}), 1);
    BOOST_CHECK_EQUAL(math::sin(p_type7{0}), 0);
}

BOOST_AUTO_TEST_CASE(poisson_series_arithmetic_test)
{
    // Just some random arithmetic tests using known trigonometric identities.
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    p_type1 x{"x"}, y{"y"};
    BOOST_CHECK_EQUAL(math::cos(x) * math::cos(y), (math::cos(x - y) + math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(-x) * math::cos(y), (math::cos(x - y) + math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(x) * math::cos(-y), (math::cos(x - y) + math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(-x) * math::cos(-y), (math::cos(x - y) + math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(x) * math::sin(y), (math::cos(x - y) - math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(-x) * math::sin(y), -(math::cos(x - y) - math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(x) * math::sin(-y), -(math::cos(x - y) - math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(-x) * math::sin(-y), (math::cos(x - y) - math::cos(x + y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(x) * math::cos(y), (math::sin(x + y) + math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(-x) * math::cos(y), -(math::sin(x + y) + math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(x) * math::cos(-y), (math::sin(x + y) + math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::sin(-x) * math::cos(-y), -(math::sin(x + y) + math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(x) * math::sin(y), (math::sin(x + y) - math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(-x) * math::sin(y), (math::sin(x + y) - math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(x) * math::sin(-y), -(math::sin(x + y) - math::sin(x - y)) / 2);
    BOOST_CHECK_EQUAL(math::cos(-x) * math::sin(-y), -(math::sin(x + y) - math::sin(x - y)) / 2);
    using math::cos;
    BOOST_CHECK_EQUAL(math::pow(math::sin(x), 5), (10 * math::sin(x) - 5 * math::sin(3 * x) + math::sin(5 * x)) / 16);
    BOOST_CHECK_EQUAL(math::pow(cos(x), 5), (10 * cos(x) + 5 * cos(3 * x) + cos(5 * x)) / 16);
    BOOST_CHECK_EQUAL(math::pow(cos(x), 5) * math::pow(math::sin(x), 5),
                      (10 * math::sin(2 * x) - 5 * math::sin(6 * x) + math::sin(10 * x)) / 512);
    BOOST_CHECK_EQUAL(math::pow(p_type1{rational(1, 2)}, 5), math::pow(rational(1, 2), 5));
#if defined(MPPP_WITH_MPFR)
    // NOTE: these won't work until we specialise safe_cast for real, due
    // to the new monomial pow() requirements.
    typedef poisson_series<polynomial<real, monomial<short>>> p_type2;
    BOOST_CHECK_EQUAL(math::pow(p_type2(real("1.234")), real("-5.678")), math::pow(real("1.234"), real("-5.678")));
    BOOST_CHECK_EQUAL(math::sin(p_type2(real("1.234"))), math::sin(real("1.234")));
    BOOST_CHECK_EQUAL(cos(p_type2(real("1.234"))), cos(real("1.234")));
    typedef poisson_series<real> p_type3;
    BOOST_CHECK_EQUAL(math::sin(p_type3(real("1.234"))), math::sin(real("1.234")));
    BOOST_CHECK_EQUAL(cos(p_type3(real("1.234"))), cos(real("1.234")));
#endif
}

BOOST_AUTO_TEST_CASE(poisson_series_degree_test)
{
    using math::cos;
    {
        typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
        BOOST_CHECK(has_degree<p_type1>::value);
        BOOST_CHECK(has_ldegree<p_type1>::value);
        BOOST_CHECK(math::degree(p_type1{}) == 0);
        BOOST_CHECK(math::degree(p_type1{"x"}) == 1);
        BOOST_CHECK(math::degree(p_type1{"x"} + 1) == 1);
        BOOST_CHECK(math::degree(p_type1{"x"}.pow(2) + 1) == 2);
        BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1) == 2);
        BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1, {"x"}) == 1);
        BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1, {"x", "y"}) == 2);
        BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1, {"z"}) == 0);
        BOOST_CHECK(math::ldegree(p_type1{"x"} + 1) == 0);
        BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + p_type1{"x"}, {"x", "y"}) == 1);
        BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + p_type1{"x"}, {"x"}) == 1);
        BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + p_type1{"x"}, {"y"}) == 0);
        p_type1 x{"x"}, y{"y"};
        BOOST_CHECK(math::degree(math::pow(x, 2) * cos(y) + 1) == 2);
        BOOST_CHECK(math::ldegree(math::pow(x, 2) * cos(y) + 1) == 0);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + 1, {"x"}) == 0);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + 1, {"y"}) == 0);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y, {"y"}) == 1);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y, {"x"}) == 0);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y) == 1);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y, {"x", "y"}) == 1);
        BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + 1, {"x", "y"}) == 0);
        typedef poisson_series<rational> p_type2;
        BOOST_CHECK(!has_degree<p_type2>::value);
        BOOST_CHECK(!has_ldegree<p_type2>::value);
    }
    // Try also with eps.
    {
        using eps = poisson_series<divisor_series<polynomial<rational, monomial<short>>, divisor<short>>>;
        using math::cos;
        using math::degree;
        using math::invert;
        using math::ldegree;
        eps x{"x"}, y{"y"}, z{"z"};
        BOOST_CHECK(has_degree<eps>::value);
        BOOST_CHECK(has_ldegree<eps>::value);
        BOOST_CHECK_EQUAL(degree(x), 1);
        BOOST_CHECK_EQUAL(degree(x * y + z), 2);
        BOOST_CHECK_EQUAL(ldegree(x * y + z), 1);
        // Divisors don't count in the computation of the degree.
        BOOST_CHECK_EQUAL(degree(invert(x)), 0);
        BOOST_CHECK_EQUAL(degree(invert(x) * x + y * x * z), 3);
        BOOST_CHECK_EQUAL(ldegree(invert(x)), 0);
        BOOST_CHECK_EQUAL(ldegree(invert(x) * x + y * x * z), 1);
        BOOST_CHECK_EQUAL(ldegree((invert(x) * x + y * x * z) * cos(x) + cos(y)), 0);
    }
}
