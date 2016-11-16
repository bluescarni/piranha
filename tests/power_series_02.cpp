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

#include "../src/power_series.hpp"

#define BOOST_TEST_MODULE power_series_02_test
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "../src/init.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "../src/real_trigonometric_kronecker_monomial.hpp"
#include "../src/s11n.hpp"
#include "../src/series.hpp"

using namespace piranha;

typedef boost::mpl::vector<double, integer, real> cf_types;
typedef boost::mpl::vector<int, integer> expo_types;

template <typename Cf, typename Expo>
class g_series_type : public power_series<series<Cf, monomial<Expo>, g_series_type<Cf, Expo>>, g_series_type<Cf, Expo>>
{
    using base = power_series<series<Cf, monomial<Expo>, g_series_type<Cf, Expo>>, g_series_type<Cf, Expo>>;

public:
    g_series_type() = default;
    g_series_type(const g_series_type &) = default;
    g_series_type(g_series_type &&) = default;
    g_series_type &operator=(const g_series_type &) = default;
    g_series_type &operator=(g_series_type &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type, base)
};

template <typename Cf>
class g_series_type2 : public power_series<series<Cf, rtk_monomial, g_series_type2<Cf>>, g_series_type2<Cf>>
{
    typedef power_series<series<Cf, rtk_monomial, g_series_type2<Cf>>, g_series_type2<Cf>> base;

public:
    g_series_type2() = default;
    g_series_type2(const g_series_type2 &) = default;
    g_series_type2(g_series_type2 &&) = default;
    g_series_type2 &operator=(const g_series_type2 &) = default;
    g_series_type2 &operator=(g_series_type2 &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type2, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2, base)
};

struct fake_int {
    fake_int();
    explicit fake_int(int);
    fake_int(const fake_int &);
    fake_int(fake_int &&) noexcept;
    fake_int &operator=(const fake_int &);
    fake_int &operator=(fake_int &&) noexcept;
    ~fake_int();
    bool operator==(const fake_int &) const;
    bool operator!=(const fake_int &) const;
    bool operator<(const fake_int &) const;
    fake_int operator+(const fake_int &) const;
    fake_int &operator+=(const fake_int &);
    fake_int operator-(const fake_int &) const;
    fake_int &operator-=(const fake_int &);
    friend std::ostream &operator<<(std::ostream &, const fake_int &);
};

namespace piranha
{

namespace math
{

template <>
struct negate_impl<fake_int> {
    void operator()(fake_int &) const;
};
}
}

namespace std
{

template <>
struct hash<fake_int> {
    typedef size_t result_type;
    typedef fake_int argument_type;
    result_type operator()(const argument_type &) const;
};
}

BOOST_AUTO_TEST_CASE(power_series_test_02)
{
    init();
    // Check the rational degree.
    typedef g_series_type<double, rational> stype0;
    BOOST_CHECK((has_degree<stype0>::value));
    BOOST_CHECK((has_ldegree<stype0>::value));
    BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype0>())), rational>::value));
    BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype0>())), rational>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::degree(std::declval<stype0>(), std::vector<std::string>{})), rational>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::ldegree(std::declval<stype0>(), std::vector<std::string>{})), rational>::value));
    typedef g_series_type<double, int> stype1;
    BOOST_CHECK((has_degree<stype1>::value));
    BOOST_CHECK((has_ldegree<stype1>::value));
    BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype1>())), int>::value));
    BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype1>())), int>::value));
    BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype1>(), std::vector<std::string>{})), int>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::ldegree(std::declval<stype1>(), std::vector<std::string>{})), int>::value));
    typedef g_series_type<stype1, long> stype2;
    BOOST_CHECK((has_degree<stype2>::value));
    BOOST_CHECK((has_ldegree<stype2>::value));
    BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype2>())), long>::value));
    BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype2>())), long>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::degree(std::declval<stype2>(), std::vector<std::string>{})), long>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::ldegree(std::declval<stype2>(), std::vector<std::string>{})), long>::value));
    typedef g_series_type2<double> stype3;
    BOOST_CHECK((!has_degree<stype3>::value));
    BOOST_CHECK((!has_ldegree<stype3>::value));
    typedef g_series_type2<g_series_type<g_series_type<double, int>, integer>> stype4;
    BOOST_CHECK((has_degree<stype4>::value));
    BOOST_CHECK((has_ldegree<stype4>::value));
    BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype4>())), integer>::value));
    BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype4>())), integer>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::degree(std::declval<stype4>(), std::vector<std::string>{})), integer>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::ldegree(std::declval<stype4>(), std::vector<std::string>{})), integer>::value));
    // Check actual instantiations as well.
    std::vector<std::string> ss;
    BOOST_CHECK_EQUAL(math::degree(stype1{}), 0);
    BOOST_CHECK_EQUAL(math::ldegree(stype1{}), 0);
    BOOST_CHECK_EQUAL(math::degree(stype1{}, ss), 0);
    BOOST_CHECK_EQUAL(math::ldegree(stype1{}, ss), 0);
    BOOST_CHECK_EQUAL(math::degree(stype2{}), 0);
    BOOST_CHECK_EQUAL(math::ldegree(stype2{}), 0);
    BOOST_CHECK_EQUAL(math::degree(stype2{}, ss), 0);
    BOOST_CHECK_EQUAL(math::ldegree(stype2{}, ss), 0);
    BOOST_CHECK_EQUAL(math::degree(stype4{}), 0);
    BOOST_CHECK_EQUAL(math::ldegree(stype4{}), 0);
    BOOST_CHECK_EQUAL(math::degree(stype4{}, ss), 0);
    BOOST_CHECK_EQUAL(math::ldegree(stype4{}, ss), 0);
    // Tests with fake int.
    typedef g_series_type<double, fake_int> stype5;
    BOOST_CHECK((has_degree<stype5>::value));
    BOOST_CHECK((has_ldegree<stype5>::value));
    BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype5>())), fake_int>::value));
    BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype5>())), fake_int>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::degree(std::declval<stype5>(), std::vector<std::string>{})), fake_int>::value));
    BOOST_CHECK(
        (std::is_same<decltype(math::ldegree(std::declval<stype5>(), std::vector<std::string>{})), fake_int>::value));
    typedef g_series_type<stype5, int> stype6;
    // This does not have a degree type because fake_int cannot be added to integer.
    BOOST_CHECK((!has_degree<stype6>::value));
    BOOST_CHECK((!has_ldegree<stype6>::value));
}

BOOST_AUTO_TEST_CASE(power_series_serialization_test)
{
    typedef g_series_type<polynomial<rational, monomial<rational>>, rational> stype;
    stype x("x"), y("y"), z = x + y, tmp;
    std::stringstream ss;
    {
        boost::archive::text_oarchive oa(ss);
        oa << z;
    }
    {
        boost::archive::text_iarchive ia(ss);
        ia >> tmp;
    }
    BOOST_CHECK_EQUAL(z, tmp);
}

BOOST_AUTO_TEST_CASE(power_series_truncation_test)
{
    // A test with polynomial, degree only in the key.
    {
        typedef polynomial<double, monomial<rational>> stype0;
        BOOST_CHECK((has_truncate_degree<stype0, int>::value));
        BOOST_CHECK((has_truncate_degree<stype0, rational>::value));
        BOOST_CHECK((has_truncate_degree<stype0, integer>::value));
        BOOST_CHECK((!has_truncate_degree<stype0, std::string>::value));
        stype0 x{"x"}, y{"y"}, z{"z"};
        stype0 s0;
        BOOST_CHECK((std::is_same<stype0, decltype(s0.truncate_degree(5))>::value));
        BOOST_CHECK_EQUAL(s0.truncate_degree(5), s0);
        s0 = x.pow(rational(10, 3));
        BOOST_CHECK_EQUAL(s0.truncate_degree(5), s0);
        BOOST_CHECK_EQUAL(s0.truncate_degree(3 / 2_q), 0);
        // x**5*y+1/2*z**-5*x*y+x*y*z/4
        s0 = x.pow(5) * y + z.pow(-5) / 2 * x * y + x * y * z / 4;
        BOOST_CHECK_EQUAL(s0.truncate_degree(3), z.pow(-5) / 2 * x * y + x * y * z / 4);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, -1), z.pow(-5) / 2 * x * y);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 2, {"x"}), z.pow(-5) / 2 * x * y + x * y * z / 4);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 5, {"x", "y"}), z.pow(-5) / 2 * x * y + x * y * z / 4);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 5, {"y", "x", "y"}), z.pow(-5) / 2 * x * y + x * y * z / 4);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 5, {"z", "x"}), s0);
        // Test with non-existing variable.
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 0, {"a", "b"}), s0);
    }
    {
        // Poisson series, degree only in the coefficient.
        typedef poisson_series<polynomial<rational, monomial<rational>>> st;
        BOOST_CHECK((has_truncate_degree<st, int>::value));
        BOOST_CHECK((has_truncate_degree<st, rational>::value));
        BOOST_CHECK((has_truncate_degree<st, integer>::value));
        BOOST_CHECK((!has_truncate_degree<st, std::string>::value));
        st x("x"), y("y"), z("z"), a("a"), b("b");
        // (x + y**2/4 + 3*x*y*z/7) * cos(a) + (x*y+y*z/3+3*z**2*x/8) * sin(a+b)
        st s0 = (x + y * y / 4 + 3 * z * x * y / 7) * math::cos(a)
                + (x * y + z * y / 3 + 3 * z * z * x / 8) * math::sin(a + b);
        BOOST_CHECK_EQUAL(s0.truncate_degree(2),
                          (x + y * y / 4) * math::cos(a) + (x * y + z * y / 3) * math::sin(a + b));
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1l), x * math::cos(a));
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, -1ll), 0);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1l, {"x"}),
                          (x + y * y / 4 + 3 * z * x * y / 7) * math::cos(a)
                              + (x * y + z * y / 3 + 3 * z * z * x / 8) * math::sin(a + b));
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, char(0), {"x"}),
                          y * y / 4 * math::cos(a) + z * y / 3 * math::sin(a + b));
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, char(1), {"y", "x"}),
                          x * math::cos(a) + (z * y / 3 + 3 * z * z * x / 8) * math::sin(a + b));
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, integer(1), {"z"}),
                          (x + y * y / 4 + 3 * z * x * y / 7) * math::cos(a) + (x * y + z * y / 3) * math::sin(a + b));
        // Test with non-existing variable.
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 0, {"foo", "bar"}), s0);
    }
    {
        // Recursive poly.
        typedef polynomial<rational, monomial<rational>> st0;
        typedef polynomial<st0, monomial<rational>> st1;
        BOOST_CHECK((has_truncate_degree<st1, int>::value));
        BOOST_CHECK((has_truncate_degree<st1, rational>::value));
        BOOST_CHECK((has_truncate_degree<st1, integer>::value));
        BOOST_CHECK((!has_truncate_degree<st1, std::string>::value));
        // (x*y+x**2+x+1/4)*z + (x+y**2+x**2*y)*z**2 + 3
        st0 x{"x"}, y{"y"};
        st1 z{"z"};
        auto s0 = (x * y + x * x + x + 1_q / 4) * z + (x + y * y + x * x * y) * z * z + 3;
        BOOST_CHECK_EQUAL(s0.truncate_degree(1), 1 / 4_q * z + 3);
        BOOST_CHECK_EQUAL(s0.truncate_degree(0), 3);
        BOOST_CHECK_EQUAL(s0.truncate_degree(2), (x + 1_q / 4) * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, -3), 0);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 3_q), (x * y + x * x + x + 1_q / 4) * z + x * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1, {"x"}), (x * y + x + 1_q / 4) * z + (x + y * y) * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1ll, {"x", "y"}), (x + 1_q / 4) * z + x * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1, {"x", "z"}), 1_q / 4 * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 2, {"x", "z"}), (x * y + x + 1_q / 4) * z + y * y * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 3, {"x", "z"}),
                          (x * y + x * x + x + 1_q / 4) * z + (x + y * y) * z * z + 3);
        // Test with non-existing variable.
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 0, {"foo", "bar"}), s0);
    }
    {
        // Recursive poly, integers and rational exponent mixed, same example as above.
        typedef polynomial<rational, monomial<integer>> st0;
        typedef polynomial<st0, monomial<rational>> st1;
        BOOST_CHECK((has_truncate_degree<st1, int>::value));
        BOOST_CHECK((has_truncate_degree<st1, rational>::value));
        BOOST_CHECK((has_truncate_degree<st1, integer>::value));
        BOOST_CHECK((!has_truncate_degree<st1, std::string>::value));
        // (x*y+x**2+x+1/4)*z + (x+y**2+x**2*y)*z**2 + 3
        st0 x{"x"}, y{"y"};
        st1 z{"z"};
        auto s0 = (x * y + x * x + x + 1_q / 4) * z + (x + y * y + x * x * y) * z * z + 3;
        BOOST_CHECK_EQUAL(s0.truncate_degree(1), 1 / 4_q * z + 3);
        BOOST_CHECK_EQUAL(s0.truncate_degree(0), 3);
        BOOST_CHECK_EQUAL(s0.truncate_degree(2), (x + 1_q / 4) * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, -3), 0);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 3_q), (x * y + x * x + x + 1_q / 4) * z + x * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1, {"x"}), (x * y + x + 1_q / 4) * z + (x + y * y) * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1ll, {"x", "y"}), (x + 1_q / 4) * z + x * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1, {"x", "z"}), 1_q / 4 * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 2, {"x", "z"}), (x * y + x + 1_q / 4) * z + y * y * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 3, {"x", "z"}),
                          (x * y + x * x + x + 1_q / 4) * z + (x + y * y) * z * z + 3);
        // Test with non-existing variable.
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 0_q, {"foo", "bar"}), s0);
    }
    {
        // Recursive poly, integers and rational exponent mixed, same example as above but switched.
        typedef polynomial<rational, monomial<rational>> st0;
        typedef polynomial<st0, monomial<integer>> st1;
        BOOST_CHECK((has_truncate_degree<st1, int>::value));
        BOOST_CHECK((has_truncate_degree<st1, rational>::value));
        BOOST_CHECK((has_truncate_degree<st1, integer>::value));
        BOOST_CHECK((!has_truncate_degree<st1, std::string>::value));
        // (x*y+x**2+x+1/4)*z + (x+y**2+x**2*y)*z**2 + 3
        st0 x{"x"}, y{"y"};
        st1 z{"z"};
        auto s0 = (x * y + x * x + x + 1_q / 4) * z + (x + y * y + x * x * y) * z * z + 3;
        BOOST_CHECK_EQUAL(s0.truncate_degree(1), 1 / 4_q * z + 3);
        BOOST_CHECK_EQUAL(s0.truncate_degree(0), 3);
        BOOST_CHECK_EQUAL(s0.truncate_degree(2), (x + 1_q / 4) * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, -3), 0);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 3_q), (x * y + x * x + x + 1_q / 4) * z + x * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1, {"x"}), (x * y + x + 1_q / 4) * z + (x + y * y) * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1ll, {"x", "y"}), (x + 1_q / 4) * z + x * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 1, {"x", "z"}), 1_q / 4 * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 2, {"x", "z"}), (x * y + x + 1_q / 4) * z + y * y * z * z + 3);
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 3, {"x", "z"}),
                          (x * y + x * x + x + 1_q / 4) * z + (x + y * y) * z * z + 3);
        // Test with non-existing variable.
        BOOST_CHECK_EQUAL(math::truncate_degree(s0, 0_z, {"foo", "bar"}), s0);
    }
}

BOOST_AUTO_TEST_CASE(power_series_degree_overflow_test)
{
    using p_type = polynomial<integer, monomial<int>>;
    using pp_type = polynomial<p_type, monomial<int>>;
    p_type x{"x"};
    pp_type y{"y"};
    BOOST_CHECK_THROW((x * y.pow(std::numeric_limits<int>::max())).degree(), std::overflow_error);
    BOOST_CHECK_THROW((x.pow(-1) * y.pow(std::numeric_limits<int>::min())).degree(), std::overflow_error);
    BOOST_CHECK_EQUAL((x * y.pow(std::numeric_limits<int>::min())).degree(), std::numeric_limits<int>::min() + 1);
}

BOOST_AUTO_TEST_CASE(power_series_mixed_degree_test)
{
    using p_type = polynomial<integer, monomial<int>>;
    using pp_type = polynomial<p_type, monomial<integer>>;
    using pp_type2 = polynomial<p_type, monomial<long>>;
    using pp_type3 = polynomial<p_type, monomial<int>>;
    using pp_type4 = polynomial<polynomial<rational, monomial<rational>>, monomial<long long>>;
    p_type x{"x"};
    pp_type y{"y"};
    pp_type2 z{"z"};
    pp_type3 a{"a"};
    pp_type4 b{"b"};
    BOOST_CHECK((std::is_same<decltype(x.degree()), int>::value));
    BOOST_CHECK((std::is_same<decltype(y.degree()), integer>::value));
    BOOST_CHECK((std::is_same<decltype(z.degree()), long>::value));
    BOOST_CHECK((std::is_same<decltype(a.degree()), int>::value));
    BOOST_CHECK((std::is_same<decltype(b.degree()), rational>::value));
}
