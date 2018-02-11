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

#define BOOST_TEST_MODULE series_08_test
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>

#include <mp++/config.hpp>
#include <mp++/exceptions.hpp>
#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <piranha/base_series_multiplier.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/safe_cast.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/settings.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using cf_types = std::tuple<double, rational>;
using expo_types = std::tuple<unsigned, integer>;

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
    // Provide fake sin/cos methods with wrong sigs.
    g_series_type sin()
    {
        return g_series_type(42);
    }
    int cos() const
    {
        return -42;
    }
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

struct negate_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type;
            p_type p("x");
            p += 1;
            p += p_type("y");
            BOOST_CHECK_EQUAL(p.size(), unsigned(3));
            p_type q1 = p, q2 = p;
            p.negate();
            BOOST_CHECK_EQUAL(p.size(), unsigned(3));
            p += q1;
            BOOST_CHECK(p.empty());
            math::negate(q2);
            q2 += q1;
            BOOST_CHECK(q2.empty());
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(series_negate_test)
{
#if defined(MPPP_WITH_MPFR)
    mppp::real_set_default_prec(100);
#endif
    tuple_for_each(cf_types{}, negate_tester());
}

struct identity_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            BOOST_CHECK(+p_type1{} == +p_type1{});
            BOOST_CHECK(+p_type1{} == p_type1{});
            BOOST_CHECK(p_type1{} == +p_type1{});
            BOOST_CHECK(p_type1("x") == +p_type1("x"));
            BOOST_CHECK(+p_type1("x") == p_type1("x"));
            BOOST_CHECK(+p_type1("x") == +p_type1("x"));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(series_identity_test)
{
    tuple_for_each(cf_types{}, identity_tester());
}

struct negation_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            BOOST_CHECK(+p_type1{} == -(-(+p_type1{})));
            BOOST_CHECK(-(-(+p_type1{})) == p_type1{});
            BOOST_CHECK(-p_type1("x") == -(+p_type1("x")));
            BOOST_CHECK(-(+p_type1("x")) == -p_type1("x"));
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(series_negation_test)
{
    tuple_for_each(cf_types{}, negation_tester());
}

struct stream_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            // Avoid the stream tests with floating-point, because of messy output.
            if (std::is_same<Cf, double>::value) {
                return;
            }
            typedef g_series_type<Cf, Expo> p_type1;
            typedef g_series_type<p_type1, Expo> p_type11;
            std::ostringstream oss;
            oss << p_type1{};
            BOOST_CHECK(oss.str() == "0");
            oss.str("");
            oss << p_type1{1};
            BOOST_CHECK(oss.str() == "1");
            oss.str("");
            oss << p_type1{-1};
            BOOST_CHECK(oss.str() == "-1");
            oss.str("");
            oss << p_type1{"x"};
            BOOST_CHECK(oss.str() == "x");
            oss.str("");
            oss << (-p_type1{"x"});
            BOOST_CHECK(oss.str() == "-x");
            oss.str("");
            oss << (-p_type1{"x"} * p_type1{"y"});
            BOOST_CHECK(oss.str() == "-x*y");
            oss.str("");
            oss << (-p_type1{"x"} + 1);
            BOOST_CHECK(oss.str() == "1-x" || oss.str() == "-x+1");
            oss.str("");
            oss << p_type11{};
            BOOST_CHECK(oss.str() == "0");
            oss.str("");
            oss << p_type11{"x"};
            BOOST_CHECK(oss.str() == "x");
            oss.str("");
            oss << (-p_type11{"x"});
            BOOST_CHECK(oss.str() == "-x");
            oss.str("");
            oss << (p_type11{1});
            BOOST_CHECK(oss.str() == "1");
            oss.str("");
            oss << (p_type11{-1});
            BOOST_CHECK(oss.str() == "-1");
            oss.str("");
            oss << (p_type11{"x"} * p_type11{"y"});
            BOOST_CHECK(oss.str() == "x*y");
            oss.str("");
            oss << (-p_type11{"x"} * p_type11{"y"});
            BOOST_CHECK(oss.str() == "-x*y");
            oss.str("");
            oss << (-p_type11{"x"} + 1);
            BOOST_CHECK(oss.str() == "1-x" || oss.str() == "-x+1");
            oss.str("");
            oss << (p_type11{"x"} - 1);
            BOOST_CHECK(oss.str() == "x-1" || oss.str() == "-1+x");
            // Test wih less term output.
            typedef polynomial<Cf, monomial<Expo>> poly_type;
            settings::set_max_term_output(3u);
            oss.str("");
            oss << p_type11{};
            BOOST_CHECK(oss.str() == "0");
            oss.str("");
            oss << p_type11{"x"};
            BOOST_CHECK(oss.str() == "x");
            oss.str("");
            oss << (-p_type11{"x"});
            BOOST_CHECK(oss.str() == "-x");
            oss.str("");
            oss << (p_type11{1});
            BOOST_CHECK(oss.str() == "1");
            oss.str("");
            oss << (p_type11{-1});
            BOOST_CHECK(oss.str() == "-1");
            oss.str("");
            oss << (p_type11{"x"} * p_type11{"y"});
            BOOST_CHECK(oss.str() == "x*y");
            oss.str("");
            oss << (-p_type11{"x"} * p_type11{"y"});
            BOOST_CHECK(oss.str() == "-x*y");
            // Test wih small term output.
            settings::set_max_term_output(1u);
            const std::string tmp_out
                = boost::lexical_cast<std::string>(3 * poly_type{"x"} + 1 + poly_type{"x"} * poly_type{"x"}
                                                   + poly_type{"x"} * poly_type{"x"} * poly_type{"x"}),
                tmp_cmp = "...";
            BOOST_CHECK(std::equal(tmp_cmp.begin(), tmp_cmp.end(), tmp_out.rbegin()));
            BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(poly_type{}), "0");
            settings::reset_max_term_output();
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(series_stream_test)
{
    tuple_for_each(cf_types{}, stream_tester());
}

struct table_info_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            p_type1 p;
            using s_type = decltype(p.table_sparsity());
            BOOST_CHECK(p.table_sparsity() == s_type{});
            BOOST_CHECK(p.table_bucket_count() == 0u);
            BOOST_CHECK(p.table_load_factor() == 0.);
            p_type1 q{"x"};
            BOOST_CHECK((q.table_sparsity() == s_type{{1u, 1u}}));
            BOOST_CHECK(q.table_load_factor() != 0.);
            BOOST_CHECK(q.table_bucket_count() != 0u);
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(series_table_info_test)
{
    tuple_for_each(cf_types{}, table_info_tester());
}

struct fake_int_01 {
    fake_int_01();
    explicit fake_int_01(int);
    fake_int_01(const fake_int_01 &);
    fake_int_01(fake_int_01 &&) noexcept;
    fake_int_01 &operator=(const fake_int_01 &);
    fake_int_01 &operator=(fake_int_01 &&) noexcept;
    ~fake_int_01();
};

struct fake_int_02 {
    fake_int_02();
    explicit fake_int_02(int);
    fake_int_02(const fake_int_02 &);
    fake_int_02(fake_int_02 &&) noexcept;
    fake_int_02 &operator=(const fake_int_02 &);
    fake_int_02 &operator=(fake_int_02 &&) noexcept;
    ~fake_int_02();
};

namespace piranha
{

template <typename T, typename U>
class pow_impl<T, U,
               typename std::enable_if<std::is_floating_point<T>::value && std::is_same<U, fake_int_01>::value>::type>
{
public:
    T operator()(const T &, const U &) const;
};

namespace math
{

template <typename T>
class is_zero_impl<T, typename std::enable_if<std::is_same<T, fake_int_01>::value>::type>
{
public:
    bool operator()(const T &) const;
};

template <typename T>
class is_zero_impl<T, typename std::enable_if<std::is_same<T, fake_int_02>::value>::type>
{
public:
    bool operator()(const T &) const;
};
}

template <typename T>
struct safe_cast_impl<integer, T, typename std::enable_if<std::is_same<T, fake_int_01>::value>::type> {
    integer operator()(const T &) const;
};

template <typename T>
struct safe_cast_impl<integer, T, typename std::enable_if<std::is_same<T, fake_int_02>::value>::type> {
    integer operator()(const T &) const;
};
}

struct pow_tester {
    template <typename Cf>
    struct runner {
        template <typename Expo>
        void operator()(const Expo &) const
        {
            typedef g_series_type<Cf, Expo> p_type1;
            p_type1 p1;
            BOOST_CHECK(p1.pow(0) == Cf(1));
            BOOST_CHECK(p1.pow(1) == Cf(0));
            p1 = 2;
            BOOST_CHECK(piranha::pow(p1, 4) == piranha::pow(Cf(2), 4));
            BOOST_CHECK(piranha::pow(p1, -4) == piranha::pow(Cf(2), -4));
            p1 = p_type1("x");
            p1 += 1;
            BOOST_CHECK(piranha::pow(p1, 1) == p1);
            BOOST_CHECK(p1.pow(2u) == p1 * p1);
            BOOST_CHECK(piranha::pow(p1, integer(3)) == p1 * p1 * p1);
            BOOST_CHECK_THROW(p1.pow(-1), std::invalid_argument);
            // Coefficient series.
            typedef g_series_type<p_type1, Expo> p_type11;
            p_type11 p11;
            BOOST_CHECK(p11.pow(0) == Cf(1));
            BOOST_CHECK(p11.pow(1) == Cf(0));
            p11 = 2;
            BOOST_CHECK(piranha::pow(p11, 4) == piranha::pow(p_type1(2), 4));
            BOOST_CHECK(piranha::pow(p11, -4) == piranha::pow(p_type1(2), -4));
            p11 = p_type11("x");
            p11 += 1;
            BOOST_CHECK(piranha::pow(p11, 1) == p11);
            BOOST_CHECK(p11.pow(2u) == p11 * p11);
            BOOST_CHECK(piranha::pow(p11, integer(3)) == p11 * p11 * p11);
        }
    };
    template <typename Cf>
    void operator()(const Cf &) const
    {
        tuple_for_each(expo_types{}, runner<Cf>());
    }
};

BOOST_AUTO_TEST_CASE(series_pow_test)
{
    tuple_for_each(cf_types{}, pow_tester());
    typedef g_series_type<double, int> p_type1;
    if (std::numeric_limits<double>::is_iec559) {
        // Test expo with float-float arguments.
        BOOST_CHECK(p_type1{2.}.pow(0.5) == std::pow(2., 0.5));
        BOOST_CHECK(p_type1{3.}.pow(-0.5) == std::pow(3., -0.5));
        BOOST_CHECK_THROW(piranha::pow(p_type1{"x"} + 1, 0.5), std::invalid_argument);
    }
    // Check division by zero error.
    typedef g_series_type<rational, int> p_type2;
    BOOST_CHECK_THROW(piranha::pow(p_type2{}, -1), mppp::zero_division_error);
#if defined(MPPP_WITH_MPFR)
    // Check the safe_cast mechanism.
    typedef g_series_type<real, int> p_type3;
    auto p = p_type3{"x"} + 1;
    BOOST_CHECK_EQUAL(p.pow(3), p.pow(real{3}));
    BOOST_CHECK_THROW(p.pow(real{-3}), std::invalid_argument);
    BOOST_CHECK_THROW(p.pow(real{"1.5"}), std::invalid_argument);
    if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
        auto pp = p_type1{"x"} + 1;
        BOOST_CHECK_EQUAL(pp.pow(3), pp.pow(3.));
        BOOST_CHECK_THROW(pp.pow(-3.), std::invalid_argument);
        BOOST_CHECK_THROW(pp.pow(1.5), std::invalid_argument);
    }
    BOOST_CHECK((is_exponentiable<p_type1, double>::value));
    BOOST_CHECK((is_exponentiable<const p_type1, double>::value));
    BOOST_CHECK((is_exponentiable<p_type1 &, double>::value));
    BOOST_CHECK((is_exponentiable<p_type1 &, double &>::value));
    BOOST_CHECK((is_exponentiable<const p_type1 &, double &>::value));
    BOOST_CHECK((is_exponentiable<p_type1, integer>::value));
    BOOST_CHECK((!is_exponentiable<p_type1, std::string>::value));
    BOOST_CHECK((!is_exponentiable<p_type1 &, std::string>::value));
    BOOST_CHECK((!is_exponentiable<p_type1 &, std::string &>::value));
    BOOST_CHECK((is_exponentiable<p_type1, fake_int_01>::value));
    BOOST_CHECK((!is_exponentiable<p_type1, fake_int_02>::value));
    // These are a couple of checks for the new pow() code, which is now able to deal with
    // exponentiation creating different types of coefficients.
    BOOST_CHECK((is_exponentiable<g_series_type<short, int>, int>::value));
    BOOST_CHECK((is_exponentiable<g_series_type<int, int>, int>::value));
    BOOST_CHECK((std::is_same<decltype(g_series_type<short, int>{}.pow(3)), g_series_type<integer, int>>::value));
    BOOST_CHECK((std::is_same<decltype(g_series_type<int, int>{}.pow(3)), g_series_type<integer, int>>::value));
    BOOST_CHECK_EQUAL((g_series_type<int, int>{"x"}.pow(2)),
                      (g_series_type<integer, int>{"x"} * g_series_type<integer, int>{"x"}));
    BOOST_CHECK((std::is_same<decltype(g_series_type<int, int>{}.pow(3.)), g_series_type<double, int>>::value));
    BOOST_CHECK_EQUAL((g_series_type<int, int>{"x"}.pow(2.)),
                      (g_series_type<double, int>{"x"} * g_series_type<integer, int>{"x"}));
    BOOST_CHECK((std::is_same<decltype(g_series_type<real, int>{}.pow(3.)), g_series_type<real, int>>::value));
    BOOST_CHECK_EQUAL((g_series_type<real, int>{"x"}.pow(2.)),
                      (g_series_type<real, int>{"x"} * g_series_type<real, int>{"x"}));
    BOOST_CHECK((std::is_same<decltype(g_series_type<rational, int>{}.pow(3_z)), g_series_type<rational, int>>::value));
    BOOST_CHECK_EQUAL((g_series_type<rational, int>{"x"}.pow(2_z)),
                      (g_series_type<rational, int>{"x"} * g_series_type<rational, int>{"x"}));
#endif
    // Some multi-threaded testing.
    p_type1 ret0, ret1;
    std::thread t0([&ret0]() {
        p_type1 x{"x"};
        auto tmp = x.pow(6);
        // Throw in a cache clear for good measure.
        p_type1::clear_pow_cache();
        ret0 = tmp.pow(8);
        p_type1::clear_pow_cache();
    });
    std::thread t1([&ret1]() {
        p_type1 x{"x"};
        auto tmp = x.pow(5);
        p_type1::clear_pow_cache();
        ret1 = tmp.pow(8);
        p_type1::clear_pow_cache();
    });
    t0.join();
    t1.join();
    BOOST_CHECK_EQUAL(ret0, p_type1{"x"}.pow(6).pow(8));
    BOOST_CHECK_EQUAL(ret1, p_type1{"x"}.pow(5).pow(8));
    // Clear the caches.
    p_type1::clear_pow_cache();
    p_type2::clear_pow_cache();
#if defined(MPPP_WITH_MPFR)
    p_type3::clear_pow_cache();
#endif
}
