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

#include <piranha/t_substitutable_series.hpp>

#define BOOST_TEST_MODULE t_substitutable_series_test
#include <boost/test/included/unit_test.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mp++/config.hpp>
#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <piranha/config.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/key/key_is_one.hpp>
#include <piranha/math.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/monomial.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/s11n.hpp>
#include <piranha/symbol_utils.hpp>

using namespace piranha;

struct key02 {
    key02() = default;
    key02(const key02 &) = default;
    key02(key02 &&) noexcept;
    key02 &operator=(const key02 &) = default;
    key02 &operator=(key02 &&) noexcept;
    key02(const symbol_fset &);
    bool operator==(const key02 &) const;
    bool operator!=(const key02 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    key02 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, key02>> t_subs(const symbol_idx &, const T &, const U &,
                                                      const symbol_fset &) const;
    void trim_identify(std::vector<char> &, const symbol_fset &) const;
    key02 trim(const std::vector<char> &, const symbol_fset &) const;
};

namespace std
{

template <>
struct hash<key02> {
    std::size_t operator()(const key02 &) const;
};
}

namespace piranha
{
template <>
class key_is_one_impl<key02>
{
public:
    bool operator()(const key02 &, const symbol_fset &) const;
};
}

template <typename Cf, typename Key>
class g_series_type : public t_substitutable_series<series<Cf, Key, g_series_type<Cf, Key>>, g_series_type<Cf, Key>>
{
    typedef t_substitutable_series<series<Cf, Key, g_series_type<Cf, Key>>, g_series_type<Cf, Key>> base;

public:
    g_series_type() = default;
    g_series_type(const g_series_type &) = default;
    g_series_type(g_series_type &&) = default;
    g_series_type &operator=(const g_series_type &) = default;
    g_series_type &operator=(g_series_type &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type, base)
};

BOOST_AUTO_TEST_CASE(t_subs_series_t_subs_test)
{
#if defined(MPPP_WITH_MPFR)
    mppp::real_set_default_prec(100);
#endif
    typedef poisson_series<polynomial<rational, monomial<short>>> p_type1;
    p_type1 x{"x"}, y{"y"};
    BOOST_CHECK((has_t_subs<p_type1, p_type1, p_type1>::value));
    BOOST_CHECK((has_t_subs<p_type1, rational, rational>::value));
    BOOST_CHECK((has_t_subs<p_type1, rational, rational>::value));
    BOOST_CHECK((has_t_subs<p_type1, double, double>::value));
    BOOST_CHECK((!has_t_subs<p_type1, int, double>::value));
    BOOST_CHECK_EQUAL(p_type1{}.t_subs("a", 2, 3), 0);
    BOOST_CHECK_EQUAL(math::t_subs(p_type1{}, "a", 2, 3), 0);
    BOOST_CHECK_EQUAL(p_type1{"x"}.t_subs("a", 2, 3), p_type1{"x"});
    BOOST_CHECK_EQUAL(math::t_subs(p_type1{"x"}, "a", 2, 3), p_type1{"x"});
    BOOST_CHECK_EQUAL(piranha::cos(p_type1{"x"}).t_subs("a", 2, 3), p_type1{"x"}.cos());
    BOOST_CHECK_EQUAL(math::t_subs(piranha::cos(p_type1{"x"}), "a", 2, 3), p_type1{"x"}.cos());
    BOOST_CHECK_EQUAL(math::t_subs(piranha::cos(p_type1{"x"}), "x", 2, 3), 2);
    BOOST_CHECK_EQUAL(math::t_subs(piranha::sin(p_type1{"x"}), "x", 2, 3), 3);
    BOOST_CHECK_EQUAL(math::t_subs(piranha::cos(p_type1{"x"}) + piranha::sin(p_type1{"x"}), "x", 2, 3), 5);
    auto tmp1 = math::t_subs(
        4 * piranha::cos(p_type1{"x"} + p_type1{"y"}) + 3 * piranha::sin(p_type1{"x"} + p_type1{"y"}), "x", 2, 3);
    BOOST_CHECK((std::is_same<decltype(tmp1), p_type1>::value));
    BOOST_CHECK_EQUAL(tmp1, 4 * 2 * piranha::cos(y) - 4 * 3 * piranha::sin(y) + 3 * 3 * piranha::cos(y)
                                + 3 * 2 * piranha::sin(y));
    auto tmp2 = math::t_subs(
        4 * piranha::cos(p_type1{"x"} - p_type1{"y"}) + 3 * piranha::sin(p_type1{"x"} - p_type1{"y"}), "x", 2, 3);
    BOOST_CHECK((std::is_same<decltype(tmp2), p_type1>::value));
    BOOST_CHECK_EQUAL(tmp2, 4 * 2 * piranha::cos(y) + 4 * 3 * piranha::sin(y) + 3 * 3 * piranha::cos(y)
                                - 3 * 2 * piranha::sin(y));
    auto tmp3 = math::t_subs(
        4 * piranha::cos(-p_type1{"x"} - p_type1{"y"}) + 3 * piranha::sin(-p_type1{"x"} - p_type1{"y"}), "x", 2, 3);
    BOOST_CHECK((std::is_same<decltype(tmp3), p_type1>::value));
    BOOST_CHECK_EQUAL(tmp3, 4 * 2 * piranha::cos(y) - 4 * 3 * piranha::sin(y) - 3 * 3 * piranha::cos(y)
                                - 3 * 2 * piranha::sin(y));
    // Some trigonometric identities from Wikipedia.
    p_type1 c{"c"}, s{"s"};
    BOOST_CHECK_EQUAL(piranha::sin(3 * x).t_subs("x", c, s), 3 * c * c * s - s * s * s);
    BOOST_CHECK_EQUAL(piranha::cos(3 * x).t_subs("x", c, s), c * c * c - 3 * s * s * c);
    BOOST_CHECK_EQUAL(
        (math::t_subs((10 * piranha::sin(x) - 5 * piranha::sin(3 * x) + piranha::sin(5 * x)) / 16, "x", c, s))
            .ipow_subs("c", integer(2), 1 - s * s),
        s * s * s * s * s);
    BOOST_CHECK_EQUAL(
        (math::t_subs((10 * piranha::cos(x) + 5 * piranha::cos(3 * x) + piranha::cos(5 * x)) / 16, "x", c, s))
            .ipow_subs("s", integer(2), 1 - c * c),
        c * c * c * c * c);
    BOOST_CHECK_EQUAL(
        (math::t_subs((10 * piranha::sin(2 * x) - 5 * piranha::sin(6 * x) + piranha::sin(10 * x)) / 512, "x", c, s))
            .template subs<p_type1>({{"c", piranha::cos(x)}, {"s", piranha::sin(x)}}),
        piranha::pow(piranha::cos(x), 5) * piranha::pow(piranha::sin(x), 5));
    BOOST_CHECK_EQUAL((piranha::cos(x) * piranha::cos(y)).t_subs("x", c, s), c * piranha::cos(y));
    BOOST_CHECK_EQUAL((piranha::sin(x) * piranha::sin(y)).t_subs("x", c, s), s * piranha::sin(y));
    BOOST_CHECK_EQUAL((piranha::sin(x) * piranha::cos(y)).t_subs("x", c, s), s * piranha::cos(y));
    BOOST_CHECK_EQUAL((piranha::cos(x) * piranha::sin(y)).t_subs("x", c, s), c * piranha::sin(y));
    BOOST_CHECK_EQUAL(4 * piranha::sin(2 * x).t_subs("x", c, s), 8 * s * c);
    BOOST_CHECK_EQUAL(5 * piranha::cos(2 * x).t_subs("x", c, s), 5 * (c * c - s * s));
    BOOST_CHECK_EQUAL((2 * piranha::sin(x + y) * piranha::cos(x - y)).t_subs("x", c, s),
                      2 * c * s + piranha::sin(2 * y));
    BOOST_CHECK_EQUAL(piranha::sin(x + p_type1{"pi2"}).t_subs("pi2", 0, 1), piranha::cos(x));
    BOOST_CHECK_EQUAL(piranha::cos(x + p_type1{"pi2"}).t_subs("pi2", 0, 1), -piranha::sin(x));
    BOOST_CHECK_EQUAL(piranha::sin(x + p_type1{"pi"}).t_subs("pi", -1, 0), -piranha::sin(x));
    BOOST_CHECK_EQUAL(piranha::cos(x + p_type1{"pi"}).t_subs("pi", -1, 0), -piranha::cos(x));
    BOOST_CHECK_EQUAL(piranha::sin(-x + p_type1{"pi"}).t_subs("pi", -1, 0), piranha::sin(x));
    BOOST_CHECK_EQUAL(piranha::cos(-x + p_type1{"pi"}).t_subs("pi", -1, 0), -piranha::cos(x));
    BOOST_CHECK_EQUAL((piranha::cos(-x + p_type1{"pi"}) + piranha::cos(y)).t_subs("pi", -1, 0),
                      -piranha::cos(x) + piranha::cos(y));
    BOOST_CHECK_EQUAL((piranha::cos(-x + p_type1{"pi"}) + piranha::cos(y + p_type1{"pi"})).t_subs("pi", -1, 0),
                      -piranha::cos(x) - piranha::cos(y));
    BOOST_CHECK_EQUAL(piranha::cos(x + p_type1{"2pi"}).t_subs("2pi", 1, 0), piranha::cos(x));
    BOOST_CHECK_EQUAL(piranha::sin(x + p_type1{"2pi"}).t_subs("2pi", 1, 0), piranha::sin(x));
    BOOST_CHECK_EQUAL(piranha::cos(-x + p_type1{"2pi"}).t_subs("2pi", 1, 0), piranha::cos(x));
    BOOST_CHECK_EQUAL(piranha::sin(-x + p_type1{"2pi"}).t_subs("2pi", 1, 0), -piranha::sin(x));
    BOOST_CHECK_EQUAL(math::t_subs(piranha::sin(-x + p_type1{"2pi"}), "2pi", 1, 0), -piranha::sin(x));
#if defined(MPPP_WITH_MPFR)
    // Real and mixed-series subs.
    typedef poisson_series<polynomial<real, monomial<short>>> p_type3;
    BOOST_CHECK((std::is_same<decltype(p_type3{"x"}.t_subs("x", c, s)), p_type3>::value));
    BOOST_CHECK((std::is_same<decltype(math::t_subs(p_type3{"x"}, "x", c, s)), p_type3>::value));
    BOOST_CHECK_EQUAL(p_type3{"x"}.cos().t_subs("x", c, s), c);
    BOOST_CHECK_EQUAL(p_type3{"x"}.cos().t_subs("x", real(.5), real(1.)), real(.5));
    BOOST_CHECK((std::is_same<decltype(x.t_subs("x", p_type3{"c"}, p_type3{"s"})), p_type3>::value));
    BOOST_CHECK_EQUAL(x.t_subs("x", p_type3{"c"}, p_type3{"s"}), p_type3{"x"});
    BOOST_CHECK_EQUAL(piranha::sin(x).t_subs("x", p_type3{"c"}, p_type3{3.}), p_type3{3.});
    BOOST_CHECK_EQUAL(
        piranha::pow(piranha::cos(p_type3{"x"}), 7).t_subs("x", real(.5), real(piranha::pow(real(3), .5)) / 2),
        piranha::pow(real(.5), 7));
    BOOST_CHECK_EQUAL(
        piranha::pow(piranha::sin(p_type3{"x"}), 7).t_subs("x", real(piranha::pow(real(3), .5)) / 2, real(.5)),
        piranha::pow(real(.5), 7));
    BOOST_CHECK_EQUAL(
        math::t_subs(piranha::pow(piranha::sin(p_type3{"x"}), 7), "x", real(piranha::pow(real(3), .5)) / 2, real(.5)),
        piranha::pow(real(.5), 7));
    BOOST_CHECK(math::abs(math::evaluate<real>(
                    ((piranha::pow(piranha::sin(p_type3{"x"}), 5) * piranha::pow(piranha::cos(p_type3{"x"}), 5))
                         .t_subs("x", real(piranha::pow(real(3), .5)) / 2, real(.5))
                     - piranha::pow(real(.5), 5) * piranha::pow(real(piranha::pow(real(3), .5)) / 2, 5))
                        .trim(),
                    {}))
                < 1E-9);
    BOOST_CHECK((has_t_subs<p_type3, p_type3, p_type3>::value));
    BOOST_CHECK((has_t_subs<p_type3, double, double>::value));
    BOOST_CHECK((has_t_subs<p_type3, real, real>::value));
    BOOST_CHECK((has_t_subs<p_type3, double, double>::value));
    BOOST_CHECK((!has_t_subs<p_type3, double, int>::value));
#endif
    // Trig subs in the coefficient.
    typedef polynomial<poisson_series<rational>, monomial<short>> p_type2;
    BOOST_CHECK_EQUAL(p_type2{}.t_subs("x", 1, 2), p_type2{});
    BOOST_CHECK_EQUAL(p_type2{3}.t_subs("x", 1, 2), p_type2{3});
    BOOST_CHECK((std::is_same<decltype(p_type2{}.t_subs("x", 1, 2)), p_type2>::value));
    BOOST_CHECK((std::is_same<decltype(p_type2{}.t_subs("x", rational(1), rational(2))), p_type2>::value));
    BOOST_CHECK((has_t_subs<p_type2, p_type2, p_type2>::value));
    BOOST_CHECK((has_t_subs<p_type2, double, double>::value));
    BOOST_CHECK((has_t_subs<p_type2, double, double>::value));
    BOOST_CHECK((!has_t_subs<p_type2, double, int>::value));
    BOOST_CHECK((key_has_t_subs<key02, int, int>::value));
    BOOST_CHECK((!has_t_subs<g_series_type<double, key02>, double, double>::value));
}

#if defined(PIRANHA_WITH_BOOST_S11N)

BOOST_AUTO_TEST_CASE(t_subs_series_serialization_test)
{
    using stype = poisson_series<polynomial<rational, monomial<short>>>;
    stype x("x"), y("y"), z = x + piranha::cos(x + y), tmp;
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

#endif
