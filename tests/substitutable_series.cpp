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

#include <piranha/substitutable_series.hpp>

#define BOOST_TEST_MODULE substitutable_series_test
#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <mp++/config.hpp>

#include <piranha/base_series_multiplier.hpp>
#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/is_key.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/monomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/s11n.hpp>
#include <piranha/series.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/term.hpp>

using namespace piranha;

#if defined(MPPP_WITH_MPFR)

static inline real operator"" _r(const char *s)
{
    return real(s, 100);
}

#endif

template <typename Cf, typename Key>
class g_series_type : public substitutable_series<series<Cf, Key, g_series_type<Cf, Key>>, g_series_type<Cf, Key>>
{
    using base = substitutable_series<series<Cf, Key, g_series_type<Cf, Key>>, g_series_type<Cf, Key>>;

public:
    template <typename Cf2>
    using rebind = g_series_type<Cf2, Key>;
    g_series_type() = default;
    g_series_type(const g_series_type &) = default;
    g_series_type(g_series_type &&) = default;
    explicit g_series_type(const char *name) : base()
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set = symbol_fset{name};
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{typename Key::value_type(1)}));
    }
    g_series_type &operator=(const g_series_type &) = default;
    g_series_type &operator=(g_series_type &&) = default;
    PIRANHA_FORWARDING_CTOR(g_series_type, base)
    PIRANHA_FORWARDING_ASSIGNMENT(g_series_type, base)
};

namespace piranha
{

template <typename Cf, typename Key>
class series_multiplier<g_series_type<Cf, Key>, void> : public base_series_multiplier<g_series_type<Cf, Key>>
{
    using base = base_series_multiplier<g_series_type<Cf, Key>>;
    template <typename T>
    using call_enabler = typename std::enable_if<
        key_is_multipliable<typename T::term_type::cf_type, typename T::term_type::key_type>::value, int>::type;

public:
    using base::base;
    template <typename T = g_series_type<Cf, Key>, call_enabler<T> = 0>
    g_series_type<Cf, Key> operator()() const
    {
        return this->plain_multiplication();
    }
};
}

// An alternative monomial class with no suitable subs() method.
template <class T>
class new_monomial : public monomial<T>
{
    using base = monomial<T>;

public:
    using base::base;
    template <typename... Args>
    new_monomial merge_symbols(Args &&... args) const
    {
        auto ret = static_cast<const base *>(this)->merge_symbols(std::forward<Args>(args)...);
        auto sbe = ret.size_begin_end();
        return new_monomial(std::get<1>(sbe), std::get<2>(sbe));
    }
    template <typename... Args>
    new_monomial trim(Args &&... args) const
    {
        auto ret = static_cast<const base *>(this)->trim(std::forward<Args>(args)...);
        auto sbe = ret.size_begin_end();
        return new_monomial(std::get<1>(sbe), std::get<2>(sbe));
    }
    template <typename Cf>
    static void multiply(std::array<term<Cf, new_monomial>, base::multiply_arity> &res,
                         const term<Cf, new_monomial> &t1, const term<Cf, new_monomial> &t2, const symbol_fset &args)
    {
        term<Cf, new_monomial> &t = res[0u];
        if (unlikely(t1.m_key.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid size of arguments set");
        }
        t.m_cf = t1.m_cf * t2.m_cf;
        t1.m_key.vector_add(t.m_key, t2.m_key);
    }
};

namespace std
{

template <typename T>
struct hash<new_monomial<T>> {
    std::size_t operator()(const new_monomial<T> &m) const
    {
        return m.hash();
    }
};
}

BOOST_AUTO_TEST_CASE(subs_series_subs_test)
{
    // Substitution on key only.
    using stype0 = g_series_type<rational, monomial<int>>;
    // Type trait checks.
    BOOST_CHECK((has_subs<stype0, int>::value));
    BOOST_CHECK((has_subs<stype0, double>::value));
    BOOST_CHECK((has_subs<stype0, integer>::value));
    BOOST_CHECK((has_subs<stype0, rational>::value));
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK((has_subs<stype0, real>::value));
#endif
    BOOST_CHECK((!has_subs<stype0, std::string>::value));
    {
        stype0 x{"x"}, y{"y"}, z{"z"};
        auto tmp = (x + y).subs<int>({{"x", 2}, {"y", 4}});
        BOOST_CHECK_EQUAL(tmp, 6);
        tmp = (x + y).subs<int>({{"x", 2}, {"y", 4}, {"z", 63}});
        BOOST_CHECK_EQUAL(tmp, 6);
        tmp = (x + y).subs<int>({{"x", 2}});
        BOOST_CHECK_EQUAL(tmp, y + 2);
        BOOST_CHECK(tmp.is_identical(math::subs<int>(x + y, {{"x", 2}})));
        BOOST_CHECK(tmp.is_identical(y + 2 + x - x));
        BOOST_CHECK((std::is_same<decltype(tmp), stype0>::value));
        tmp = (x + y).subs<int>({{"z", 2}});
        BOOST_CHECK_EQUAL(tmp, x + y);
        auto tmp2 = (math::pow(x, 2) + y).subs<double>({{"x", 2.}, {"y", 3.}});
        BOOST_CHECK_EQUAL(tmp2, 7.);
        tmp2 = (x + y).subs<double>({{"x", 2.}});
        BOOST_CHECK_EQUAL(tmp2, y + 2.);
        BOOST_CHECK(tmp2.is_identical(math::subs<double>(x + y, {{"x", 2.}})));
        BOOST_CHECK((std::is_same<decltype(tmp2), g_series_type<double, monomial<int>>>::value));
        auto tmp3 = (3 * x + y * y / 7).subs<rational>({{"y", 2 / 5_q}});
        BOOST_CHECK(tmp3.is_identical(math::subs<rational>(3 * x + y * y / 7, {{"y", 2 / 5_q}})));
        BOOST_CHECK((std::is_same<decltype(tmp3), stype0>::value));
        BOOST_CHECK_EQUAL(tmp3, 3 * x + 2 / 5_q * 2 / 5_q / 7);
#if defined(MPPP_WITH_MPFR)
        auto tmp4 = (3 * x + y * y / 7).subs<real>({{"y", 2.123_r}});
        BOOST_CHECK(tmp4.is_identical(math::subs<real>(3 * x + y * y / 7, {{"y", 2.123_r}})));
        BOOST_CHECK((std::is_same<decltype(tmp4), g_series_type<real, monomial<int>>>::value));
        BOOST_CHECK_EQUAL(tmp4, 3 * x + math::pow(2.123_r, 2) / 7);
#endif
        auto tmp5 = (3 * x + y * y / 7).subs<integer>({{"y", -2_z}});
        BOOST_CHECK(tmp5.is_identical(math::subs<integer>(3 * x + y * y / 7, {{"y", -2_z}})));
        BOOST_CHECK((std::is_same<decltype(tmp5), stype0>::value));
        BOOST_CHECK_EQUAL(tmp5, 3 * x + math::pow(-2_z, 2) / 7_q);
        // Substitution with series.
        auto tmp6 = (3 * x + y * y / 7).subs<stype0>({{"y", z * 2}});
        BOOST_CHECK(tmp6.is_identical(math::subs<stype0>(3 * x + y * y / 7, {{"y", z * 2}})));
        BOOST_CHECK((std::is_same<decltype(tmp6), stype0>::value));
        BOOST_CHECK_EQUAL(tmp6, 3 * x + 4 * z * z / 7);
    }
    // Subs on cf only.
    using stype1 = g_series_type<stype0, new_monomial<int>>;
    BOOST_CHECK((is_key<stype1::term_type::key_type>::value));
    BOOST_CHECK((key_is_multipliable<rational, stype1::term_type::key_type>::value));
    BOOST_CHECK((!key_has_subs<stype1::term_type::key_type, rational>::value));
    BOOST_CHECK((has_subs<stype1, int>::value));
    BOOST_CHECK((has_subs<stype1, double>::value));
    BOOST_CHECK((has_subs<stype1, integer>::value));
    BOOST_CHECK((has_subs<stype1, rational>::value));
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK((has_subs<stype1, real>::value));
#endif
    BOOST_CHECK((!has_subs<stype1, std::string>::value));
    {
        stype1 x{stype0{"x"}}, y{stype0{"y"}}, z{stype0{"z"}};
        auto tmp = (x + y).subs<int>({{"x", 2}, {"y", -3}});
        BOOST_CHECK_EQUAL(tmp, -1);
        tmp = (x + y).subs<int>({{"x", 2}});
        BOOST_CHECK_EQUAL(tmp, y + 2);
        BOOST_CHECK(tmp.is_identical(math::subs<int>(x + y, {{"x", 2}})));
        BOOST_CHECK(tmp.is_identical(y + 2 + x - x));
        BOOST_CHECK((std::is_same<decltype(tmp), stype1>::value));
        tmp = (x + y).subs<int>({{"z", 2}});
        BOOST_CHECK_EQUAL(tmp, x + y);
        auto tmp2 = (x + y).subs<double>({{"x", 2.}, {"y", 3.}});
        BOOST_CHECK_EQUAL(tmp2, 5.);
        tmp2 = (x + y).subs<double>({{"x", 2.}});
        BOOST_CHECK_EQUAL(tmp2, y + 2.);
        BOOST_CHECK(tmp2.is_identical(math::subs<double>(x + y, {{"x", 2.}})));
        BOOST_CHECK((std::is_same<decltype(tmp2),
                                  g_series_type<g_series_type<double, monomial<int>>, new_monomial<int>>>::value));
        auto tmp3 = (3 * x + y * y / 7).subs<rational>({{"y", 2 / 5_q}});
        BOOST_CHECK(tmp3.is_identical(math::subs<rational>(3 * x + y * y / 7, {{"y", 2 / 5_q}})));
        BOOST_CHECK((std::is_same<decltype(tmp3), stype1>::value));
        BOOST_CHECK_EQUAL(tmp3, 3 * x + 2 / 5_q * 2 / 5_q / 7);
#if defined(MPPP_WITH_MPFR)
        auto tmp4 = (3 * x + y * y / 7).subs<real>({{"y", 2.123_r}});
        BOOST_CHECK(tmp4.is_identical(math::subs<real>(3 * x + y * y / 7, {{"y", 2.123_r}})));
        BOOST_CHECK((
            std::is_same<decltype(tmp4), g_series_type<g_series_type<real, monomial<int>>, new_monomial<int>>>::value));
        BOOST_CHECK_EQUAL(tmp4, 3 * x + math::pow(2.123_r, 2) / 7);
#endif
        auto tmp5 = (3 * x + y * y / 7).subs<integer>({{"y", -2_z}});
        BOOST_CHECK(tmp5.is_identical(math::subs<integer>(3 * x + y * y / 7, {{"y", -2_z}})));
        BOOST_CHECK((std::is_same<decltype(tmp5), stype1>::value));
        BOOST_CHECK_EQUAL(tmp5, 3 * x + math::pow(-2_z, 2) / 7_q);
        auto tmp6 = (3 * x + y * y / 7).subs<stype1>({{"y", -2 * z}});
        BOOST_CHECK(tmp6.is_identical(math::subs<stype1>(3 * x + y * y / 7, {{"y", -2 * z}})));
        BOOST_CHECK((std::is_same<decltype(tmp6), stype1>::value));
        BOOST_CHECK_EQUAL(tmp6, 3 * x + 4 * z * z / 7);
    }
    // Subs on cf and key.
    using stype2 = g_series_type<stype0, monomial<int>>;
    BOOST_CHECK((is_key<stype2::term_type::key_type>::value));
    BOOST_CHECK((key_is_multipliable<rational, stype2::term_type::key_type>::value));
    BOOST_CHECK((key_has_subs<stype2::term_type::key_type, rational>::value));
    BOOST_CHECK((has_subs<stype2, int>::value));
    BOOST_CHECK((has_subs<stype2, double>::value));
    BOOST_CHECK((has_subs<stype2, integer>::value));
    BOOST_CHECK((has_subs<stype2, rational>::value));
#if defined(MPPP_WITH_MPFR)
    BOOST_CHECK((has_subs<stype2, real>::value));
#endif
    BOOST_CHECK((!has_subs<stype2, std::string>::value));
    {
        // Recursive poly with x and y at the first level, z in the second.
        stype2 x{stype0{"x"}}, y{stype0{"y"}}, z{"z"}, t{"t"};
        auto tmp = ((x + y) * z).subs<int>({{"x", 2}, {"y", 3}, {"z", 4}});
        BOOST_CHECK_EQUAL(tmp, 20);
        tmp = ((x + y) * z).subs<int>({{"x", 2}});
        BOOST_CHECK_EQUAL(tmp, (2 + y) * z);
        BOOST_CHECK(tmp.is_identical(math::subs<int>((x + y) * z, {{"x", 2}})));
        BOOST_CHECK((std::is_same<decltype(tmp), stype2>::value));
        tmp = ((x + y) * z).subs<int>({{"t", 2}});
        BOOST_CHECK_EQUAL(tmp, (x + y) * z);
        auto tmp2 = ((x + y) * z).subs<double>({{"x", 2.}, {"y", -4.}, {"z", 5.}});
        BOOST_CHECK_EQUAL(tmp2, -10.);
        tmp2 = ((x + y) * z).subs<double>({{"x", 2.}});
        BOOST_CHECK_EQUAL(tmp2, (2. + y) * z);
        BOOST_CHECK(tmp2.is_identical(math::subs<double>((x + y) * z, {{"x", 2.}})));
        BOOST_CHECK(
            (std::is_same<decltype(tmp2), g_series_type<g_series_type<double, monomial<int>>, monomial<int>>>::value));
        auto tmp3 = ((3 * x + y * y / 7) * z).subs<rational>({{"z", 2 / 5_q}});
        BOOST_CHECK(tmp3.is_identical(math::subs<rational>((3 * x + y * y / 7) * z, {{"z", 2 / 5_q}})));
        BOOST_CHECK((std::is_same<decltype(tmp3), stype2>::value));
        BOOST_CHECK_EQUAL(tmp3, (3 * x + y * y / 7) * 2 / 5_q);
        auto tmp4 = ((3 * x + y * y / 7) * z).subs<rational>({{"y", 2 / 3_q}, {"z", 4_q}});
        BOOST_CHECK(tmp4.is_identical(math::subs<rational>((3 * x + y * y / 7) * z, {{"y", 2 / 3_q}, {"z", 4_q}})));
        BOOST_CHECK((std::is_same<decltype(tmp4), stype2>::value));
        BOOST_CHECK_EQUAL(tmp4, (3 * x + 2 / 3_q * 2 / 3_q / 7) * 4_z);
#if defined(MPPP_WITH_MPFR)
        auto tmp5 = ((3 * x + y * y / 7) * z).subs<real>({{"y", -2.123_r}});
        BOOST_CHECK(tmp5.is_identical(math::subs<real>((3 * x + y * y / 7) * z, {{"y", -2.123_r}})));
        BOOST_CHECK(
            (std::is_same<decltype(tmp5), g_series_type<g_series_type<real, monomial<int>>, monomial<int>>>::value));
        BOOST_CHECK_EQUAL(tmp5, (3 * x + math::pow(-2.123_r, 2) / 7) * z);
#endif
        auto tmp6 = ((3 * x + y * y / 7) * z).subs<stype2>({{"z", 2 * t}});
        BOOST_CHECK(tmp6.is_identical(math::subs<stype2>((3 * x + y * y / 7) * z, {{"z", 2 * t}})));
        BOOST_CHECK((std::is_same<decltype(tmp6), stype2>::value));
        BOOST_CHECK_EQUAL(tmp6, (3 * x + y * y / 7) * 2 * t);
    }
    {
        // Same variable in both levels.
        stype2 x1{stype0{"x"}}, x2{"x"}, y{stype0{"y"}};
        BOOST_CHECK_EQUAL((x1 * x2 * y * 4 / 3_q + 2 * y).subs<int>({{"x", 4}}), 16 * y * 4 / 3_q + 2 * y);
        BOOST_CHECK_EQUAL((x1 * x2 * y * 4 / 3_q + 2 * y).subs<int>({{"x", 4}, {"y", 5}}), 16 * 5 * 4 / 3_q + 2 * 5);
    }
}

#if defined(PIRANHA_WITH_BOOST_S11N)

BOOST_AUTO_TEST_CASE(subs_series_serialization_test)
{
    using stype = g_series_type<rational, monomial<int>>;
    stype x("x"), y("y"), z = (x + 3 * y + 1).pow(4), tmp;
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
